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

#include "CommonIncludes.hpp"
#include "ProcessTraceConsumer.hpp"

#include "..\Recording\RecordingResults.h"
#include "..\Recording\SimpleResults.h"

// Target:           mTargetProcessName mTargetPid mEtlFileName
//  All processes    nullptr            0          nullptr
//  Process by name  process name       0          nullptr
//  Process by ID    nullptr            pid        nullptr
//  ETL file         nullptr            0          path
struct PresentMonArgs {
	const char* mOutputDirectory = nullptr;
	const char* mOutputFileName = nullptr;
	const char* mTargetProcessName = nullptr;
	const char* mEtlFileName = nullptr;
	const char* mTargetExectuable = nullptr;
	int mTargetPid = 0;
	int mDelay = 0;
	int mTimer = 0;
	int mRestartCount = 0;
	bool mScrollLockToggle = false;
	bool mExcludeDropped = false;
	bool mSimple = false;
	bool mTerminateOnProcExit = false;
	bool mHotkeySupport = false;
	unsigned int mHotkey = 0x7A;
	bool mStartExecutable = false;
	bool mSimpleRecording = false;
	bool mCaptureAll = false;
	bool mDetailedRecording = false;
};

struct PresentMonData {
	const PresentMonArgs* mArgs = nullptr;
	uint64_t mStartupQpcTime;
	std::map<uint32_t, ProcessInfo> mProcessMap;
	uint32_t mTerminationProcessCount = 0;
	RecordingResults mRecordingResults;
};

void PresentMonEtw(const PresentMonArgs& args, HWND window);

void PresentMon_Init(const PresentMonArgs& args, PresentMonData& data);
void PresentMon_UpdateNewProcesses(PresentMonData& data, std::map<uint32_t, ProcessInfo>& processes);
void PresentMon_Update(PresentMonData& data, std::vector<std::shared_ptr<PresentEvent>>& presents, uint64_t perfFreq);
void PresentMon_UpdateDeadProcesses(PresentMonData& data, std::vector<uint32_t>& processIds);
void PresentMon_Shutdown(PresentMonData& data, bool log_corrupted);

