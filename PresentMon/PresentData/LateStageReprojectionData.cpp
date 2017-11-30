/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "LateStageReprojectionData.hpp"

#include <algorithm>

enum {
    MAX_HISTORY_TIME = 3000,
    LSR_TIMEOUT_THRESHOLD_TICKS = 10000, // 10 sec
    MAX_LSRS_IN_DEQUE = 120 * (MAX_HISTORY_TIME / 1000)
};

void LateStageReprojectionData::PruneDeque(std::deque<LateStageReprojectionEvent> &lsrHistory, uint64_t perfFreq, uint32_t msTimeDiff, uint32_t maxHistLen) {
    while (!lsrHistory.empty() &&
        (lsrHistory.size() > maxHistLen ||
        ((double)(lsrHistory.back().QpcTime - lsrHistory.front().QpcTime) / perfFreq) * 1000 > msTimeDiff)) {
        lsrHistory.pop_front();
    }
}

void LateStageReprojectionData::AddLateStageReprojection(LateStageReprojectionEvent& p)
{
    if (LateStageReprojectionPresented(p.FinalState))
    {
        assert(p.MissedVsyncCount == 0);
        mDisplayedLSRHistory.push_back(p);
    }
    else if(LateStageReprojectionMissed(p.FinalState))
    {
        assert(p.MissedVsyncCount >= 1);
        mLifetimeLsrMissedFrames += p.MissedVsyncCount;
    }

    if (p.NewSourceLatched)
    {
        mSourceHistory.push_back(p);
    }
    else
    {
        mLifetimeAppMissedFrames++;
    }

    if (!mLSRHistory.empty())
    {
        assert(mLSRHistory.back().QpcTime <= p.QpcTime);
    }
    mLSRHistory.push_back(p);
}

void LateStageReprojectionData::UpdateLateStageReprojectionInfo(uint64_t now, uint64_t perfFreq)
{
    PruneDeque(mSourceHistory, perfFreq, MAX_HISTORY_TIME, MAX_LSRS_IN_DEQUE);
    PruneDeque(mDisplayedLSRHistory, perfFreq, MAX_HISTORY_TIME, MAX_LSRS_IN_DEQUE);
    PruneDeque(mLSRHistory, perfFreq, MAX_HISTORY_TIME, MAX_LSRS_IN_DEQUE);

    mLastUpdateTicks = now;
}

double LateStageReprojectionData::ComputeHistoryTime(const std::deque<LateStageReprojectionEvent>& lsrHistory, uint64_t qpcFreq)
{
    if (lsrHistory.size() < 2) {
        return 0.0;
    }

    auto start = lsrHistory.front().QpcTime;
    auto end = lsrHistory.back().QpcTime;
    return double(end - start) / qpcFreq;
}

size_t LateStageReprojectionData::ComputeHistorySize()
{
    if (mLSRHistory.size() < 2) {
        return 0;
    }

    return mLSRHistory.size();
}

double LateStageReprojectionData::ComputeHistoryTime(uint64_t qpcFreq)
{
    return ComputeHistoryTime(mLSRHistory, qpcFreq);
}

double LateStageReprojectionData::ComputeFps(const std::deque<LateStageReprojectionEvent>& lsrHistory, uint64_t qpcFreq)
{
    if (lsrHistory.size() < 2) {
        return 0.0;
    }
    auto start = lsrHistory.front().QpcTime;
    auto end = lsrHistory.back().QpcTime;
    auto count = lsrHistory.size() - 1;

    double deltaT = double(end - start) / qpcFreq;
    return count / deltaT;
}

double LateStageReprojectionData::ComputeSourceFps(uint64_t qpcFreq)
{
    return ComputeFps(mSourceHistory, qpcFreq);
}

double LateStageReprojectionData::ComputeDisplayedFps(uint64_t qpcFreq)
{
    return ComputeFps(mDisplayedLSRHistory, qpcFreq);
}

double LateStageReprojectionData::ComputeFps(uint64_t qpcFreq)
{
    return ComputeFps(mLSRHistory, qpcFreq);
}

LateStageReprojectionRuntimeStats LateStageReprojectionData::ComputeRuntimeStats(uint64_t qpcFreq)
{
    LateStageReprojectionRuntimeStats stats = {};
    if (mLSRHistory.size() < 2) {
        return stats;
    }

    uint64_t totalAppSourceReleaseToLsrAcquireTime = 0;
    uint64_t totalAppSourceCpuRenderTime = 0;
    const size_t count = mLSRHistory.size();
    for (size_t i = 0; i < count; i++)
    {
        LateStageReprojectionEvent& current = mLSRHistory[i];

        stats.mGpuPreemptionInMs.AddValue(current.GpuSubmissionToGpuStartInMs);
        stats.mGpuExecutionInMs.AddValue(current.GpuStartToGpuStopInMs);
        stats.mCopyPreemptionInMs.AddValue(current.GpuStopToCopyStartInMs);
        stats.mCopyExecutionInMs.AddValue(current.CopyStartToCopyStopInMs);

        const double lsrInputLatchToVsyncInMs =
            current.InputLatchToGpuSubmissionInMs +
            current.GpuSubmissionToGpuStartInMs +
            current.GpuStartToGpuStopInMs +
            current.GpuStopToCopyStartInMs +
            current.CopyStartToCopyStopInMs +
            current.CopyStopToVsyncInMs;
        stats.mLsrInputLatchToVsyncInMs.AddValue(lsrInputLatchToVsyncInMs);

        // Stats just with averages
        totalAppSourceReleaseToLsrAcquireTime += current.Source.GetReleaseFromRenderingToAcquireForPresentationTime();
        totalAppSourceCpuRenderTime += current.GetAppCpuRenderFrameTime();
        stats.mLsrCpuRenderTimeInMs +=
            current.CpuRenderFrameStartToHeadPoseCallbackStartInMs +
            current.HeadPoseCallbackStartToHeadPoseCallbackStopInMs +
            current.HeadPoseCallbackStopToInputLatchInMs +
            current.InputLatchToGpuSubmissionInMs;

        stats.mGpuEndToVsyncInMs += current.CopyStopToVsyncInMs;
        stats.mVsyncToPhotonsMiddleInMs += (current.TimeUntilPhotonsMiddleMs - current.TimeUntilVsyncMs);
        stats.mLsrPoseLatencyInMs += current.LsrPredictionLatencyMs;
        stats.mAppPoseLatencyInMs += current.AppPredictionLatencyMs;

        if (!current.NewSourceLatched) {
            stats.mAppMissedFrames++;
        }

        if (LateStageReprojectionMissed(current.FinalState)) {
            stats.mLsrMissedFrames += current.MissedVsyncCount;
            if (current.MissedVsyncCount > 1) {
                // We always expect a count of at least 1, but if we missed multiple vsyncs during a single LSR period we need to account for that.
                stats.mLsrConsecutiveMissedFrames += (current.MissedVsyncCount - 1);
            }
            if (i > 0 && LateStageReprojectionMissed((mLSRHistory[i - 1].FinalState))) {
                stats.mLsrConsecutiveMissedFrames++;
            }
        }
    }

    stats.mAppProcessId = mLSRHistory[count - 1].GetAppProcessId();
    stats.mLsrProcessId = mLSRHistory[count - 1].ProcessId;

    stats.mAppSourceCpuRenderTimeInMs = 1000 * double(totalAppSourceCpuRenderTime) / qpcFreq;
    stats.mAppSourceReleaseToLsrAcquireInMs = 1000 * double(totalAppSourceReleaseToLsrAcquireTime) / qpcFreq;

    stats.mAppSourceReleaseToLsrAcquireInMs /= count;
    stats.mAppSourceCpuRenderTimeInMs /= count;
    stats.mLsrCpuRenderTimeInMs /= count;
    stats.mGpuEndToVsyncInMs /= count;
    stats.mVsyncToPhotonsMiddleInMs /= count;
    stats.mLsrPoseLatencyInMs /= count;
    stats.mAppPoseLatencyInMs /= count;

    return stats;
}

bool LateStageReprojectionData::IsStale(uint64_t now) const
{
    return now - mLastUpdateTicks > LSR_TIMEOUT_THRESHOLD_TICKS;
}
