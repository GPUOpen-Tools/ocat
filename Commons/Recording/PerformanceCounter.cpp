//
// Copyright(c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "PerformanceCounter.hpp"

#include "../Logging/MessageLog.h"

#include <algorithm>

constexpr float percentile = 0.99f;

namespace GameOverlay {
using Clock = std::chrono::high_resolution_clock;
using MilliSeconds = std::chrono::duration<double, std::milli>;
using Seconds = std::chrono::duration<double>;

const MilliSeconds PerformanceCounter::refreshRate_{1000.0};

PerformanceCounter::PerformanceCounter() : frameTimes_(1024)
{
  lastFrame_ = Clock::now();
  recordingStart_ = Clock::now();
  deltaTime_ = MilliSeconds::zero();
}

const PerformanceCounter::FrameInfo& PerformanceCounter::NextFrame()
{
  const auto currFrame = Clock::now();
  const MilliSeconds frameDelta = currFrame - lastFrame_;
  deltaTime_ += frameDelta;
  frameTimes_.push_back(static_cast<float>(frameDelta.count()));

  currentFrameCount_++;
  totalFrameCount_++;
  if (deltaTime_ >= refreshRate_) {
    currentFrameInfo_.fps = static_cast<int32_t>(currentFrameCount_);
    currentFrameInfo_.ms = static_cast<float>(deltaTime_.count() / currentFrameCount_);

    currentFrameCount_ = 0;
    deltaTime_ -= refreshRate_;
  }

  lastFrame_ = currFrame;

  return currentFrameInfo_;
}

const PerformanceCounter::CaptureResults& PerformanceCounter::GetLastCaptureResults() const
{
  return prevCaptureResults_;
}

void PerformanceCounter::Start()
{
  recordingStart_ = Clock::now();
  totalFrameCount_ = 0;
  frameTimes_.clear();
}

void PerformanceCounter::Stop()
{
  const MilliSeconds durationMS = lastFrame_ - recordingStart_;
  prevCaptureResults_.averageFPS =
      static_cast<float>(totalFrameCount_ / (durationMS.count() / 1000.0f));
  prevCaptureResults_.averageMS = static_cast<float>(durationMS.count() / totalFrameCount_);

  std::sort(frameTimes_.begin(), frameTimes_.end(), std::less<double>());
  const auto rank = static_cast<int>(percentile * frameTimes_.size());
  prevCaptureResults_.frameTimePercentile = static_cast<float>(frameTimes_[rank]);
}
}
