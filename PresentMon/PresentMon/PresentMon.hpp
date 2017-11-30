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

#pragma once

#include <map>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "CommandLine.hpp"
#include "..\PresentData\SwapChainData.hpp"
#include "..\PresentData\LateStageReprojectionData.hpp"
#include "..\PresentData\MixedRealityTraceConsumer.hpp"

struct ProcessInfo {
    std::string mModuleName;
    std::map<uint64_t, SwapChainData> mChainMap;
	uint64_t mLastRefreshTicks; // GetTickCount64
	FILE *mOutputFile;          // Used if -multi_csv
	FILE *mLsrOutputFile;       // Used if -multi_csv
	bool mTargetProcess;
};


struct PresentMonData {
	char mCaptureTimeStr[18] = "";
    const CommandLineArgs *mArgs = nullptr;
    uint64_t mStartupQpcTime = 0;
    FILE *mOutputFile = nullptr;
	FILE *mLsrOutputFile = nullptr;
    std::map<uint32_t, ProcessInfo> mProcessMap;
	std::map<std::string, std::pair<FILE*, FILE*> > mProcessOutputFiles;
	LateStageReprojectionData mLateStageReprojectionData;
    uint32_t mTerminationProcessCount = 0;
};

void EtwConsumingThread(const CommandLineArgs& args);

void PresentMon_Init(const CommandLineArgs& args, PresentMonData& data);
void PresentMon_Update(PresentMonData& pm, 
	std::vector<std::shared_ptr<PresentEvent>>& presents, 
	std::vector<std::shared_ptr<LateStageReprojectionEvent>>& lsrs, 
	uint64_t now, uint64_t perfFreq);
void PresentMon_Shutdown(PresentMonData& pm, uint32_t totalEventsLost, 
	uint32_t totalBuffersLost);

bool EtwThreadsShouldQuit();
void PostStopRecording();
void PostQuitProcess();

