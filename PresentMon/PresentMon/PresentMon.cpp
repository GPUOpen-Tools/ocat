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

#include <algorithm>
#include <shlwapi.h>

#include "TraceSession.hpp"
#include "PresentMon.hpp"

template <typename Map, typename F>
static void map_erase_if(Map& m, F pred)
{
    typename Map::iterator i = m.begin();
    while ((i = std::find_if(i, m.end(), pred)) != m.end()) {
        m.erase(i++);
    }
}

static void SetConsoleText(const char *text)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    enum { MAX_BUFFER = 16384 };
    char buffer[16384];
    int bufferSize = 0;
    auto write = [&](char ch) {
        if (bufferSize < MAX_BUFFER) {
            buffer[bufferSize++] = ch;
        }
    };

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    int oldBufferSize = int(csbi.dwSize.X * csbi.dwSize.Y);
    if (oldBufferSize > MAX_BUFFER) {
        oldBufferSize = MAX_BUFFER;
    }

    int x = 0;
    while (*text) {
        int repeat = 1;
        char ch = *text;
        if (ch == '\t') {
            ch = ' ';
            repeat = 4;
        }
        else if (ch == '\n') {
            ch = ' ';
            repeat = csbi.dwSize.X - x;
        }
        for (int i = 0; i < repeat; ++i) {
            write(ch);
            if (++x >= csbi.dwSize.X) {
                x = 0;
            }
        }
        text++;
    }

    for (int i = bufferSize; i < oldBufferSize; ++i)
    {
        write(' ');
    }

    COORD origin = { 0,0 };
    DWORD dwCharsWritten;
    WriteConsoleOutputCharacterA(
        hConsole,
        buffer,
        bufferSize,
        origin,
        &dwCharsWritten);

    SetConsoleCursorPosition(hConsole, origin);
}

static bool IsTargetProcess(CommandLineArgs const& args, uint32_t processId, char const* processName)
{
	// -blacklist
	for (auto excludeProcessName : args.mBlackList) {
		if (_stricmp(excludeProcessName.c_str(), processName) == 0) {
			return false;
		}
	}

	// -capture_all
	if (args.mTargetPid == 0 && args.mTargetProcessNames.empty()) {
		return true;
	}

	// -process_id
	if (args.mTargetPid != 0 && args.mTargetPid == processId) {
		return true;
	}

	// -process_name
	for (auto targetProcessName : args.mTargetProcessNames) {
		if (_stricmp(targetProcessName, processName) == 0) {
			return true;
		}
	}

	return false;
}

//  mOutputFilename mHotkeySupport mMultiCsv processName -> FileName
//  PATH.EXT        true           true      PROCESSNAME -> PATH-PROCESSNAME-INDEX.EXT
//  PATH.EXT        false          true      PROCESSNAME -> PATH-PROCESSNAME.EXT
//  PATH.EXT        true           false     any         -> PATH-INDEX.EXT
//  PATH.EXT        false          false     any         -> PATH.EXT
//  nullptr         any            any       nullptr     -> PresentMon-TIME.csv
//  nullptr         any            any       PROCESSNAME -> PresentMon-PROCESSNAME-TIME.csv
//
// If wmr, then append _WMR to name.
static void GenerateOutputFilename(const PresentMonData& pm, const char* processName, bool wmr, char* path)
{
	char ext[_MAX_EXT];

	if (pm.mArgs->mOutputFileName) {
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char name[_MAX_FNAME];
		_splitpath_s(pm.mArgs->mOutputFileName, drive, dir, name, ext);

		int i = _snprintf_s(path, MAX_PATH, _TRUNCATE, "%s%s%s", drive, dir, name);

		if (pm.mArgs->mMultiCsv) {
			i += _snprintf_s(path + i, MAX_PATH - i, _TRUNCATE, "-%s", processName);
		}

		if (pm.mArgs->mHotkeySupport) {
			i += _snprintf_s(path + i, MAX_PATH - i, _TRUNCATE, "-%d", pm.mArgs->mRecordingCount);
		}
	}
	else {
		strcpy_s(ext, ".csv");

		if (processName == nullptr) {
			_snprintf_s(path, MAX_PATH, _TRUNCATE, "PresentMon-%s", pm.mCaptureTimeStr);
		}
		else {
			_snprintf_s(path, MAX_PATH, _TRUNCATE, "PresentMon-%s-%s", processName, pm.mCaptureTimeStr);
		}
	}

	if (wmr) {
		strcat_s(path, MAX_PATH, "_WMR");
	}

	strcat_s(path, MAX_PATH, ext);
}

static void CreateOutputFiles(PresentMonData& pm, const char* processName, FILE** outputFile)
{
	// Open output file and print CSV header
	char outputFilePath[MAX_PATH];
	GenerateOutputFilename(pm, processName, false, outputFilePath);
	fopen_s(outputFile, outputFilePath, "w");
	if (*outputFile) {
		fprintf(*outputFile, "Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,PresentFlags");
		if (pm.mArgs->mVerbosity > Verbosity::Simple)
		{
			fprintf(*outputFile, ",AllowsTearing,PresentMode");
		}
		if (pm.mArgs->mVerbosity >= Verbosity::Verbose)
		{
			fprintf(*outputFile, ",WasBatched,DwmNotified");
		}
		fprintf(*outputFile, ",Dropped,TimeInSeconds,MsBetweenPresents");
		if (pm.mArgs->mVerbosity > Verbosity::Simple)
		{
			fprintf(*outputFile, ",MsBetweenDisplayChange");
		}
		fprintf(*outputFile, ",MsInPresentAPI");
		if (pm.mArgs->mVerbosity > Verbosity::Simple)
		{
			fprintf(*outputFile, ",MsUntilRenderComplete,MsUntilDisplayed");
		}
		fprintf(*outputFile, "\n");
	}
}

static void TerminateProcess(PresentMonData& pm, ProcessInfo const& proc)
{
	if (!proc.mTargetProcess) {
		return;
	}

	// Save the output files in case the process is re-started
	if (pm.mArgs->mMultiCsv) {
		pm.mProcessOutputFiles.emplace(proc.mModuleName, proc.mOutputFile);
	}

    // Quit if this is the last process tracked for -terminate_on_proc_exit
    if (pm.mArgs->mTerminateOnProcExit) {
        pm.mTerminationProcessCount -= 1;
        if (pm.mTerminationProcessCount == 0) {
            PostStopRecording();
            PostQuitProcess();
        }
    }
}

static void StopProcess(PresentMonData& pm, std::map<uint32_t, ProcessInfo>::iterator it)
{
	TerminateProcess(pm, it->second);
	pm.mProcessMap.erase(it);
}

static void StopProcess(PresentMonData& pm, uint32_t processId)
{
	auto it = pm.mProcessMap.find(processId);
	if (it != pm.mProcessMap.end()) {
		StopProcess(pm, it);
	}
}

static ProcessInfo* StartNewProcess(PresentMonData& pm, ProcessInfo* proc, uint32_t processId, std::string const& imageFileName, uint64_t now)
{
	proc->mModuleName = imageFileName;
	proc->mOutputFile = nullptr;
	proc->mLastRefreshTicks = now;
	proc->mTargetProcess = IsTargetProcess(*pm.mArgs, processId, imageFileName.c_str());

    if (!proc->mTargetProcess) {
        return nullptr;
    }

	// Create output files now if we're creating one per process or if we're
	// waiting to know the single target process name specified by PID.
	if (pm.mArgs->mMultiCsv) {
		auto it = pm.mProcessOutputFiles.find(imageFileName);
		if (it == pm.mProcessOutputFiles.end()) {
			CreateOutputFiles(pm, imageFileName.c_str(), &proc->mOutputFile);
		}
		else {
			proc->mOutputFile = it->second;
			pm.mProcessOutputFiles.erase(it);
		}
	}
	else if (pm.mArgs->mOutputFile && pm.mOutputFile == nullptr) {
		CreateOutputFiles(pm, imageFileName.c_str(), &pm.mOutputFile);
	}


    // Include process in -terminate_on_proc_exit count
    if (pm.mArgs->mTerminateOnProcExit) {
        pm.mTerminationProcessCount += 1;
    }

	return proc;
}

static ProcessInfo* StartProcess(PresentMonData& pm, uint32_t processId, std::string const& imageFileName, uint64_t now)
{
    auto it = pm.mProcessMap.find(processId);
    if (it != pm.mProcessMap.end()) {
        StopProcess(pm, it);
    }

    auto proc = &pm.mProcessMap.emplace(processId, ProcessInfo()).first->second;
    return StartNewProcess(pm, proc, processId, imageFileName, now);
}

static ProcessInfo* StartProcessIfNew(PresentMonData& pm, uint32_t processId, uint64_t now)
{
    auto it = pm.mProcessMap.find(processId);
    if (it != pm.mProcessMap.end()) {
        auto proc = &it->second;
        return proc->mTargetProcess ? proc : nullptr;
    }

    std::string imageFileName("<error>");
    if (!pm.mArgs->mEtlFileName) {
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (h) {
            char path[MAX_PATH] = "<error>";
            char* name = path;
            DWORD numChars = sizeof(path);
            if (QueryFullProcessImageNameA(h, 0, path, &numChars) == TRUE) {
                name = PathFindFileNameA(path);
            }
            imageFileName = name;
            CloseHandle(h);
        }
    }

    auto proc = &pm.mProcessMap.emplace(processId, ProcessInfo()).first->second;
    return StartNewProcess(pm, proc, processId, imageFileName, now);
}

static bool UpdateProcessInfo_Realtime(PresentMonData& pm, ProcessInfo& info, uint64_t now, uint32_t thisPid)
{
	// Check periodically if the process has exited
    if (now - info.mLastRefreshTicks > 1000) {
        info.mLastRefreshTicks = now;

		auto running = false;
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, thisPid);
        if (h) {
            char path[MAX_PATH] = "<error>";
            char* name = path;
            DWORD numChars = sizeof(path);
            if (QueryFullProcessImageNameA(h, 0, path, &numChars) == TRUE) {
                name = PathFindFileNameA(path);
            }
            if (info.mModuleName.compare(name) != 0) {
				// Image name changed, which means that our process exited and another
                // one started with the same PID.
                TerminateProcess(pm, info);
                StartNewProcess(pm, &info, thisPid, name, now);
            }

            DWORD dwExitCode = 0;
            if (GetExitCodeProcess(h, &dwExitCode) == TRUE && dwExitCode == STILL_ACTIVE) {
               running = true;
            }

            CloseHandle(h);
        }

		if (!running) {
			return false;
		}
    }

    // remove chains without recent updates
    map_erase_if(info.mChainMap, [now](const std::pair<const uint64_t, SwapChainData>& entry) {
        return entry.second.IsStale(now);
    });

	return true;
}

const char* PresentModeToString(PresentMode mode)
{
    switch (mode) {
    case PresentMode::Hardware_Legacy_Flip: return "Hardware: Legacy Flip";
    case PresentMode::Hardware_Legacy_Copy_To_Front_Buffer: return "Hardware: Legacy Copy to front buffer";
    case PresentMode::Hardware_Direct_Flip: return "Hardware: Direct Flip";
    case PresentMode::Hardware_Independent_Flip: return "Hardware: Independent Flip";
    case PresentMode::Composed_Flip: return "Composed: Flip";
    case PresentMode::Composed_Copy_GPU_GDI: return "Composed: Copy with GPU GDI";
    case PresentMode::Composed_Copy_CPU_GDI: return "Composed: Copy with CPU GDI";
    case PresentMode::Composed_Composition_Atlas: return "Composed: Composition Atlas";
    case PresentMode::Hardware_Composed_Independent_Flip: return "Hardware Composed: Independent Flip";
    default: return "Other";
    }
}

const char* RuntimeToString(Runtime rt)
{
    switch (rt) {
    case Runtime::DXGI: return "DXGI";
    case Runtime::D3D9: return "D3D9";
    default: return "Other";
    }
}

const char* FinalStateToDroppedString(PresentResult res)
{
    switch (res) {
    case PresentResult::Presented: return "0";
    case PresentResult::Error: return "Error";
    default: return "1";
    }
}

void AddPresent(PresentMonData& pm, PresentEvent& p, uint64_t now, uint64_t perfFreq)
{
	const uint32_t appProcessId = p.ProcessId;
	auto proc = StartProcessIfNew(pm, appProcessId, now);
	if (proc == nullptr) {
		return; // process is not a target
	}

    //for (auto& entry : pm.mArgs->mBlackList) {
    //  if (_stricmp(entry.c_str(), proc->mModuleName.c_str()) == 0) {
    //    // process is on blacklist
    //    return;
    //  }
    //}

    auto& chain = proc->mChainMap[p.SwapChainAddress];
    chain.AddPresentToSwapChain(p);

	auto file = pm.mArgs->mMultiCsv ? proc->mOutputFile : pm.mOutputFile;
	if (file && (p.FinalState == PresentResult::Presented || !pm.mArgs->mExcludeDropped)) {
		auto len = chain.mPresentHistory.size();
		auto displayedLen = chain.mDisplayedPresentHistory.size();
		if (len > 1) {
			auto& curr = chain.mPresentHistory[len - 1];
			auto& prev = chain.mPresentHistory[len - 2];
			double deltaMilliseconds = 1000 * double(curr.QpcTime - prev.QpcTime) / perfFreq;
			double deltaReady = curr.ReadyTime == 0 ? 0.0 : (1000 * double(curr.ReadyTime - curr.QpcTime) / perfFreq);
			double deltaDisplayed = curr.FinalState == PresentResult::Presented ? (1000 * double(curr.ScreenTime - curr.QpcTime) / perfFreq) : 0.0;
			double timeTakenMilliseconds = 1000 * double(curr.TimeTaken) / perfFreq;

			double timeSincePreviousDisplayed = 0.0;
			if (curr.FinalState == PresentResult::Presented && displayedLen > 1) {
				assert(chain.mDisplayedPresentHistory[displayedLen - 1].QpcTime == curr.QpcTime);
				auto& prevDisplayed = chain.mDisplayedPresentHistory[displayedLen - 2];
				timeSincePreviousDisplayed = 1000 * double(curr.ScreenTime - prevDisplayed.ScreenTime) / perfFreq;
			}

			double timeInSeconds = (double)(int64_t)(p.QpcTime - pm.mStartupQpcTime) / perfFreq;

			if (pm.mArgs->mPresentCallback)
			{
				pm.mArgs->mPresentCallback(proc->mModuleName, timeInSeconds, deltaMilliseconds);
			}

			fprintf(file, "%s,%d,0x%016llX,%s,%d,%d",
				proc->mModuleName.c_str(), appProcessId, p.SwapChainAddress, RuntimeToString(p.Runtime), curr.SyncInterval, curr.PresentFlags);
			if (pm.mArgs->mVerbosity > Verbosity::Simple)
			{
				fprintf(file, ",%d,%s", curr.SupportsTearing, PresentModeToString(curr.PresentMode));
			}
			if (pm.mArgs->mVerbosity >= Verbosity::Verbose)
			{
				fprintf(file, ",%d,%d", curr.WasBatched, curr.DwmNotified);
			}
			fprintf(file, ",%s,%.6lf,%.3lf", FinalStateToDroppedString(curr.FinalState), timeInSeconds, deltaMilliseconds);
			if (pm.mArgs->mVerbosity > Verbosity::Simple)
			{
				fprintf(file, ",%.3lf", timeSincePreviousDisplayed);
			}
			fprintf(file, ",%.3lf", timeTakenMilliseconds);
			if (pm.mArgs->mVerbosity > Verbosity::Simple)
			{
				fprintf(file, ",%.3lf,%.3lf", deltaReady, deltaDisplayed);
			}
			fprintf(file, "\n");
		}
	}

	chain.UpdateSwapChainInfo(p, now, perfFreq);
}

void PresentMon_Init(const CommandLineArgs& args, PresentMonData& pm)
{
	pm.mArgs = &args;

	if (!args.mEtlFileName)
	{
		QueryPerformanceCounter((PLARGE_INTEGER)&pm.mStartupQpcTime);
	}
	else
	{
		// Reading events from ETL file so current QPC value is irrelevant. Update this 
		// later from first event in the file.
		pm.mStartupQpcTime = 0;
	}

	// Generate capture date string in ISO 8601 format
	{
		struct tm tm;
		time_t time_now = time(NULL);
		localtime_s(&tm, &time_now);
		_snprintf_s(pm.mCaptureTimeStr, _TRUNCATE, "%4d-%02d-%02dT%02d%02d%02d",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	// Create output files now if we're not creating one per process and we
	// don't need to wait for the single process name specified by PID.
	if (args.mOutputFile && !args.mMultiCsv && !(args.mTargetPid != 0 && args.mTargetProcessNames.empty())) {
		CreateOutputFiles(pm, args.mTargetPid == 0 && args.mTargetProcessNames.size() == 1 ? args.mTargetProcessNames[0] : nullptr, &pm.mOutputFile);
	}
}

void PresentMon_Update(PresentMonData& pm, std::vector<std::shared_ptr<PresentEvent>>& presents, uint64_t now, uint64_t perfFreq)
{
	// store the new presents into processes
	for (auto& p : presents)
	{
		AddPresent(pm, *p, now, perfFreq);
	}

	// Update realtime process info
	if (!pm.mArgs->mEtlFileName) {
		std::vector<std::map<uint32_t, ProcessInfo>::iterator> remove;
		for (auto ii = pm.mProcessMap.begin(), ie = pm.mProcessMap.end(); ii != ie; ++ii) {
			if (!UpdateProcessInfo_Realtime(pm, ii->second, now, ii->first)) {
				remove.emplace_back(ii);
			}
		}
		for (auto ii : remove) {
			StopProcess(pm, ii);
		}
	}

	// Display information to console
	if (!pm.mArgs->mSimpleConsole) {
		std::string display;

		// ProcessInfo
		for (auto const& p : pm.mProcessMap) {
			auto processId = p.first;
			auto const& proc = p.second;

			// Don't display non-specified or empty processes
			if (!proc.mTargetProcess ||
				proc.mModuleName.empty() ||
				proc.mChainMap.empty()) {
				continue;
			}

			char str[256] = {};
			_snprintf_s(str, _TRUNCATE, "\n%s[%d]:\n", proc.mModuleName.c_str(), processId);
			display += str;

			for (auto& chain : proc.mChainMap)
			{
				double fps = chain.second.ComputeFps(perfFreq);

				_snprintf_s(str, _TRUNCATE, "\t%016llX (%s): SyncInterval %d | Flags %d | %.2lf ms/frame (%.1lf fps, ",
					chain.first,
					RuntimeToString(chain.second.mRuntime),
					chain.second.mLastSyncInterval,
					chain.second.mLastFlags,
					1000.0 / fps,
					fps);
				display += str;

				if (pm.mArgs->mVerbosity > Verbosity::Simple) {
					_snprintf_s(str, _TRUNCATE, "%.1lf displayed fps, ", chain.second.ComputeDisplayedFps(perfFreq));
					display += str;
				}

				_snprintf_s(str, _TRUNCATE, "%.2lf ms CPU", chain.second.ComputeCpuFrameTime(perfFreq) * 1000.0);
				display += str;

				if (pm.mArgs->mVerbosity > Verbosity::Simple) {
					_snprintf_s(str, _TRUNCATE, ", %.2lf ms latency) (%s",
						1000.0 * chain.second.ComputeLatency(perfFreq),
						PresentModeToString(chain.second.mLastPresentMode));
					display += str;

					if (chain.second.mLastPresentMode == PresentMode::Hardware_Composed_Independent_Flip) {
						_snprintf_s(str, _TRUNCATE, ": Plane %d", chain.second.mLastPlane);
						display += str;
					}

					if ((chain.second.mLastPresentMode == PresentMode::Hardware_Composed_Independent_Flip ||
						chain.second.mLastPresentMode == PresentMode::Hardware_Independent_Flip) &&
						pm.mArgs->mVerbosity >= Verbosity::Verbose &&
						chain.second.mDwmNotified) {
						_snprintf_s(str, _TRUNCATE, ", DWM notified");
						display += str;
					}

					if (pm.mArgs->mVerbosity >= Verbosity::Verbose &&
						chain.second.mHasBeenBatched) {
						_snprintf_s(str, _TRUNCATE, ", batched");
						display += str;
					}
				}

				_snprintf_s(str, _TRUNCATE, ")%s\n",
					(now - chain.second.mLastUpdateTicks) > 1000 ? " [STALE]" : "");
				display += str;
			}
		}

		SetConsoleText(display.c_str());
	}
}

void CloseFile(FILE* fp, uint32_t totalEventsLost, uint32_t totalBuffersLost)
{
	if (fp == nullptr) {
		return;
	}

	if (totalEventsLost > 0) {
		fprintf(fp, "warning: %u events were lost; collected data may be unreliable.\n", totalEventsLost);
	}
	if (totalBuffersLost > 0) {
		fprintf(fp, "warning: %u buffers were lost; collected data may be unreliable.\n", totalBuffersLost);
	}

	fclose(fp);
}

void PresentMon_Shutdown(PresentMonData& pm, uint32_t totalEventsLost, uint32_t totalBuffersLost)
{
	CloseFile(pm.mOutputFile, totalEventsLost, totalBuffersLost);
	pm.mOutputFile = nullptr;

	for (auto& p : pm.mProcessMap) {
		auto proc = &p.second;
		CloseFile(proc->mOutputFile, totalEventsLost, totalBuffersLost);
	}

	for (auto& p : pm.mProcessOutputFiles) {
		CloseFile(p.second, totalEventsLost, totalBuffersLost);
	}

	pm.mProcessMap.clear();
	pm.mProcessOutputFiles.clear();

	if (pm.mArgs->mSimpleConsole == false) {
		SetConsoleText("");
	}
}

static bool g_EtwProcessingThreadProcessing = false;
static void EtwProcessingThread(TraceSession *session)
{
    assert(g_EtwProcessingThreadProcessing == true);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    auto status = ProcessTrace(&session->traceHandle_, 1, NULL, NULL);
    (void) status; // check: _status == ERROR_SUCCESS;

    // Notify EtwConsumingThread that processing is complete
    g_EtwProcessingThreadProcessing = false;
}

void EtwConsumingThread(const CommandLineArgs& args)
{
    Sleep(args.mDelay * 1000);
    if (EtwThreadsShouldQuit()) {
        return;
    }

    PresentMonData data;
    PMTraceConsumer pmConsumer(args.mVerbosity == Verbosity::Simple);

    TraceSession session;

    session.AddProviderAndHandler(DXGI_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn) &HandleDXGIEvent, &pmConsumer);
    session.AddProviderAndHandler(D3D9_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn) &HandleD3D9Event, &pmConsumer);
    if (args.mVerbosity != Verbosity::Simple) {
        session.AddProviderAndHandler(DXGKRNL_PROVIDER_GUID,   TRACE_LEVEL_INFORMATION, 1,      0, (EventHandlerFn) &HandleDXGKEvent,   &pmConsumer);
        session.AddProviderAndHandler(WIN32K_PROVIDER_GUID,    TRACE_LEVEL_INFORMATION, 0x1000, 0, (EventHandlerFn) &HandleWin32kEvent, &pmConsumer);
        session.AddProviderAndHandler(DWM_PROVIDER_GUID,       TRACE_LEVEL_VERBOSE,     0,      0, (EventHandlerFn) &HandleDWMEvent,    &pmConsumer);
        session.AddProviderAndHandler(Win7::DWM_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0, 0, (EventHandlerFn) &HandleDWMEvent, &pmConsumer);
        session.AddProvider(Win7::DXGKRNL_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 1, 0);
    }
    session.AddHandler(NT_PROCESS_EVENT_GUID,         (EventHandlerFn) &HandleNTProcessEvent,           &pmConsumer);
    session.AddHandler(Win7::DXGKBLT_GUID,            (EventHandlerFn) &Win7::HandleDxgkBlt,            &pmConsumer);
    session.AddHandler(Win7::DXGKFLIP_GUID,           (EventHandlerFn) &Win7::HandleDxgkFlip,           &pmConsumer);
    session.AddHandler(Win7::DXGKPRESENTHISTORY_GUID, (EventHandlerFn) &Win7::HandleDxgkPresentHistory, &pmConsumer);
    session.AddHandler(Win7::DXGKQUEUEPACKET_GUID,    (EventHandlerFn) &Win7::HandleDxgkQueuePacket,    &pmConsumer);
    session.AddHandler(Win7::DXGKVSYNCDPC_GUID,       (EventHandlerFn) &Win7::HandleDxgkVSyncDPC,       &pmConsumer);
    session.AddHandler(Win7::DXGKMMIOFLIP_GUID,       (EventHandlerFn) &Win7::HandleDxgkMMIOFlip,       &pmConsumer);

    if (!(args.mEtlFileName == nullptr
        ? session.InitializeRealtime("PresentMon", &EtwThreadsShouldQuit)
        : session.InitializeEtlFile(args.mEtlFileName, &EtwThreadsShouldQuit))) {
        return;
    }

    if (args.mSimpleConsole) {
        printf("Started recording.\n");
    }
    if (args.mScrollLockIndicator) {
        EnableScrollLock(true);
    }

    {
        // Launch the ETW producer thread
        g_EtwProcessingThreadProcessing = true;
        std::thread etwProcessingThread(EtwProcessingThread, &session);

        // Consume / Update based on the ETW output
        {

            PresentMon_Init(args, data);
            auto timerRunning = args.mTimer > 0;
            auto timerEnd = GetTickCount64() + args.mTimer * 1000;

            std::vector<std::shared_ptr<PresentEvent>> presents;
            std::vector<NTProcessEvent> ntProcessEvents;

			uint32_t totalEventsLost = 0;
			uint32_t totalBuffersLost = 0;
            for (;;) {
#if _DEBUG
                if (args.mSimpleConsole) {
                    printf(".");
                }
#endif

                presents.clear();
                ntProcessEvents.clear();

                // If we are reading events from ETL file set start time to match time stamp of first event
                if (data.mArgs->mEtlFileName && data.mStartupQpcTime == 0)
                {
                    data.mStartupQpcTime = session.startTime_;
                }

				uint64_t now = GetTickCount64();

				// Dequeue any captured NTProcess events; if ImageFileName is
				// empty then the process stopped, otherwise it started.
				pmConsumer.DequeueProcessEvents(ntProcessEvents);
				for (auto ntProcessEvent : ntProcessEvents) {
					if (!ntProcessEvent.ImageFileName.empty()) {
						StartProcess(data, ntProcessEvent.ProcessId, ntProcessEvent.ImageFileName, now);
					}
				}

				pmConsumer.DequeuePresents(presents);

				if (args.mScrollLockToggle && (GetKeyState(VK_SCROLL) & 1) == 0) {
					presents.clear();
				}

				auto doneProcessingEvents = g_EtwProcessingThreadProcessing ? false : true;
				PresentMon_Update(data, presents, now, session.frequency_);

				for (auto ntProcessEvent : ntProcessEvents) {
					if (ntProcessEvent.ImageFileName.empty()) {
						StopProcess(data, ntProcessEvent.ProcessId);
					}
				}

                uint32_t eventsLost = 0;
                uint32_t buffersLost = 0;
                if (session.CheckLostReports(&eventsLost, &buffersLost)) {
                    printf("Lost %u events, %u buffers.", eventsLost, buffersLost);
					totalEventsLost += eventsLost;
					totalBuffersLost += buffersLost;

					// FIXME: How do we set a threshold here?
					if (eventsLost > 100) {
						PostStopRecording();
						PostQuitProcess();
					}
				}

                if (timerRunning) {
                    if (GetTickCount64() >= timerEnd) {
                        PostStopRecording();
                        if (args.mTerminateAfterTimer) {
                            PostQuitProcess();
                        }
                        timerRunning = false;
                    }
                }

                if (doneProcessingEvents) {
                    assert(EtwThreadsShouldQuit() || args.mEtlFileName);
                    if (!EtwThreadsShouldQuit()) {
                        PostStopRecording();
                        PostQuitProcess();
                    }
                    break;
                }

                Sleep(100);
            }

            PresentMon_Shutdown(data, totalEventsLost, totalBuffersLost);
        }

        assert(etwProcessingThread.joinable());
        assert(!g_EtwProcessingThreadProcessing);
        etwProcessingThread.join();
    }

    session.Finalize();

    if (args.mScrollLockIndicator) {
        EnableScrollLock(false);
    }
    if (args.mSimpleConsole) {
        printf("Stopping recording.\n");
    }
}