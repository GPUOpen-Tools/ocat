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

#include "TraceSession.hpp"
#include "PresentMon.hpp"

#include "MessageLog.h"

#include <psapi.h>
#include <shlwapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "tdh.lib")

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
    auto write = [&](int ch) {
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
        int ch = *text;
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

static void UpdateProcessInfo_Realtime(ProcessInfo& info, uint64_t now, uint32_t thisPid)
{
    if (now - info.mLastRefreshTicks > 1000) {
        info.mLastRefreshTicks = now;
        char path[MAX_PATH] = "<error>";
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, thisPid);
        if (h) {
            GetModuleFileNameExA(h, NULL, path, sizeof(path) - 1);
            std::string name = PathFindFileNameA(path);
            if (name != info.mModuleName) {
                info.mChainMap.clear();
                info.mModuleName = name;
            }
            DWORD dwExitCode = 0;
            info.mProcessExists = true;
            if (GetExitCodeProcess(h, &dwExitCode) && dwExitCode != STILL_ACTIVE) {
                info.mProcessExists = false;
            }
            CloseHandle(h);
        } else {
            info.mChainMap.clear();
            info.mProcessExists = false;
        }
    }
    // remove chains without recent updates
        map_erase_if(info.mChainMap, [now](const std::pair<const uint64_t, SwapChainData>& entry) {
        return entry.second.IsStale(now);
    });
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
	  auto& proc = pm.mProcessMap[p.ProcessId];
    if (!proc.mLastRefreshTicks && !pm.mArgs->mEtlFileName) {
				UpdateProcessInfo_Realtime(proc, now, p.ProcessId);
    }
    if (pm.mArgs->mTargetProcessName && strcmp(pm.mArgs->mTargetProcessName, "*") && _stricmp(pm.mArgs->mTargetProcessName, proc.mModuleName.c_str()) != 0) {
        // process name does not match
			return;
    }
    if (pm.mArgs->mTargetPid && p.ProcessId != pm.mArgs->mTargetPid) {
			return;
    }

    if (pm.mArgs->mTerminateOnProcExit && !proc.mTerminationProcess) {
			  proc.mTerminationProcess = true;
        ++pm.mTerminationProcessCount;
    }

    auto& chain = proc.mChainMap[p.SwapChainAddress];
    chain.AddPresentToSwapChain(p);

		
    if ((p.FinalState == PresentResult::Presented || !pm.mArgs->mExcludeDropped)) {
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
						auto& results = pm.mRecordingResults.OnPresent(p.ProcessId, proc.mModuleName);
						if (results.created) {
							if (!pm.mArgs->mSimple) {
								fprintf(results.outputFile,
									"%s,%d,0x%016llX,%s,%d,%d,%d,%s,%d,%.6lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf\n",
									proc.mModuleName.c_str(), p.ProcessId, p.SwapChainAddress,
									RuntimeToString(p.Runtime), curr.SyncInterval, curr.SupportsTearing,
									curr.PresentFlags, PresentModeToString(curr.PresentMode),
									curr.FinalState != PresentResult::Presented, timeInSeconds, deltaMilliseconds,
									timeSincePreviousDisplayed, timeTakenMilliseconds, deltaReady, deltaDisplayed);
							}
							else {
								fprintf(results.outputFile, "%s,%d,0x%016llX,%s,%d,%d,%d,%.6lf,%.3lf,%.3lf\n",
									proc.mModuleName.c_str(), p.ProcessId, p.SwapChainAddress,
									RuntimeToString(p.Runtime), curr.SyncInterval, curr.PresentFlags,
									curr.FinalState != PresentResult::Presented, timeInSeconds, deltaMilliseconds,
									timeTakenMilliseconds);
							}
							if (pm.mArgs->mSimpleRecording) {
								results.simpleResults.AddPresent(deltaMilliseconds, timeInSeconds);
							}
						}
        }
    }

    chain.UpdateSwapChainInfo(p, now, perfFreq);
}

void PresentMon_Init(const PresentMonArgs& args, PresentMonData& pm)
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

		g_messageLog.Log(MessageLog::LOG_DEBUG, "PresentMonEtw", "Output directory " + std::string(args.mOutputDirectory));
		pm.mRecordingResults.Init(args.mOutputDirectory);
}

void PresentMon_UpdateNewProcesses(PresentMonData& pm, std::map<uint32_t, ProcessInfo>& newProcesses)
{
    for (auto processPair : newProcesses) {
        pm.mProcessMap[processPair.first] = processPair.second;
    }
}

void PresentMon_UpdateDeadProcesses(PresentMonData& pm, std::vector<uint32_t>& deadProcesses)
{
    for (auto pid : deadProcesses) {
        pm.mProcessMap.erase(pid);
    }
}

void PresentMon_Update(PresentMonData& pm, std::vector<std::shared_ptr<PresentEvent>>& presents, uint64_t perfFreq)
{
    std::string display;
    uint64_t now = GetTickCount64();

    // store the new presents into processes
    for (auto& p : presents)
    {
        AddPresent(pm, *p, now, perfFreq);
    }

    // update all processes
    for (auto& proc : pm.mProcessMap)
    {
        if (!pm.mArgs->mEtlFileName) {
            UpdateProcessInfo_Realtime(proc.second, now, proc.first);
        }

        if (proc.second.mTerminationProcess && !proc.second.mProcessExists) {
            --pm.mTerminationProcessCount;
            if (pm.mTerminationProcessCount == 0) {
                QuitPresentMon();
            }
            proc.second.mTerminationProcess = false;
        }

        if (proc.second.mModuleName.empty() ||
            proc.second.mChainMap.empty())
        {
            // don't display empty processes
            continue;
        }

        char str[256] = {};
        _snprintf_s(str, _TRUNCATE, "%s[%d]:\n", proc.second.mModuleName.c_str(),proc.first);
        display += str;
        for (auto& chain : proc.second.mChainMap)
        {
            double fps = chain.second.ComputeFps(perfFreq);

            _snprintf_s(str, _TRUNCATE, "\t%016llX (%s): SyncInterval %d | Flags %d | %.2lf ms/frame (%.1lf fps, ",
                chain.first,
                RuntimeToString(chain.second.mRuntime),
                chain.second.mLastSyncInterval,
                chain.second.mLastFlags,
                1000.0/fps,
                fps);
            display += str;

            if (!pm.mArgs->mSimple) {
                _snprintf_s(str, _TRUNCATE, "%.1lf displayed fps, ", chain.second.ComputeDisplayedFps(perfFreq));
                display += str;
            }

            _snprintf_s(str, _TRUNCATE, "%.2lf ms CPU", chain.second.ComputeCpuFrameTime(perfFreq) * 1000.0);
            display += str;

            if (!pm.mArgs->mSimple) {
                _snprintf_s(str, _TRUNCATE, ", %.2lf ms latency) (%s",
                    1000.0 * chain.second.ComputeLatency(perfFreq),
                    PresentModeToString(chain.second.mLastPresentMode));
                display += str;

                if (chain.second.mLastPresentMode == PresentMode::Hardware_Composed_Independent_Flip) {
                    _snprintf_s(str, _TRUNCATE, ": Plane %d", chain.second.mLastPlane);
                    display += str;
                }
            }

            _snprintf_s(str, _TRUNCATE, ")%s\n",
                (now - chain.second.mLastUpdateTicks) > 1000 ? " [STALE]" : "");
            display += str;
        }
    }

    // refresh the console
    SetConsoleText(display.c_str());
}

void PresentMon_Shutdown(PresentMonData& pm, bool log_corrupted)
{
		pm.mRecordingResults.OnShutdown(log_corrupted);
    pm.mProcessMap.clear();

    SetConsoleText("");
}

static bool g_FileComplete = false;
static void EtwProcessingThread(TraceSession *session)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    session->Process();

    // Guarantees that the PM thread does one more loop to pick up any last events before calling QuitPresentMon()
    g_FileComplete = true;
}

void PresentMonEtw(const PresentMonArgs& args, HWND window)
{
    Sleep(args.mDelay * 1000);
    if (g_StopRecording) {
        return;
    }

    g_FileComplete = false;

    // Convert ETL Filename to wchar_t if it exists, and construct a
    // TraceSession.
    wchar_t* wEtlFileName = nullptr;
    if (args.mEtlFileName != nullptr) {
        auto size = strlen(args.mEtlFileName) + 1;
        wEtlFileName = new wchar_t [size];
        mbstowcs_s(&size, wEtlFileName, size, args.mEtlFileName, size - 1);
    }

    TraceSession session(L"PresentMon", wEtlFileName);

    delete[] wEtlFileName;
    wEtlFileName = nullptr;


    PMTraceConsumer pmConsumer(args.mSimple);
    ProcessTraceConsumer procConsumer = {};

    MultiTraceConsumer mtConsumer = {};
    mtConsumer.AddTraceConsumer(&procConsumer);
    mtConsumer.AddTraceConsumer(&pmConsumer);

    if (!args.mEtlFileName && !session.Start()) {
        if (session.Status() == ERROR_ALREADY_EXISTS) {
            if (!session.Stop() || !session.Start()) {
								g_messageLog.Log(MessageLog::LOG_ERROR, "PresentMonEtw", "Creating session failed");
                printf("ETW session error. Quitting.\n");
                exit(0);
            }
        }
    }

    session.EnableProvider(DXGI_PROVIDER_GUID, TRACE_LEVEL_INFORMATION);
    session.EnableProvider(D3D9_PROVIDER_GUID, TRACE_LEVEL_INFORMATION);
    if (!args.mSimple)
    {
        session.EnableProvider(DXGKRNL_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 1);
        session.EnableProvider(WIN32K_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0x1000);
        session.EnableProvider(DWM_PROVIDER_GUID, TRACE_LEVEL_VERBOSE);
    }

    session.OpenTrace(&mtConsumer);
    uint32_t eventsLost, buffersLost;

    {
        // Launch the ETW producer thread
        std::thread etwThread(EtwProcessingThread, &session);

        // Consume / Update based on the ETW output
        {
            PresentMonData data;

            PresentMon_Init(args, data);
            uint64_t start_time = GetTickCount64();

            std::vector<std::shared_ptr<PresentEvent>> presents;
            std::map<uint32_t, ProcessInfo> newProcesses;
            std::vector<uint32_t> deadProcesses;

            bool log_corrupted = false;
						bool timeSignaled = false;

            while (!g_StopRecording)
            {
                presents.clear();
                newProcesses.clear();
                deadProcesses.clear();

                // If we are reading events from ETL file set start time to match time stamp of first event
                if (data.mArgs->mEtlFileName && data.mStartupQpcTime == 0)
                {
                    data.mStartupQpcTime = mtConsumer.mTraceStartTime;
                }

                if (args.mEtlFileName) {
                    procConsumer.GetProcessEvents(newProcesses, deadProcesses);
                    PresentMon_UpdateNewProcesses(data, newProcesses);
                }

                pmConsumer.DequeuePresents(presents);
                if (args.mScrollLockToggle && (GetKeyState(VK_SCROLL) & 1) == 0) {
                    presents.clear();
                }


                PresentMon_Update(data, presents, session.PerfFreq());
                if (session.AnythingLost(eventsLost, buffersLost)) {
                    printf("Lost %u events, %u buffers.", eventsLost, buffersLost);
										g_messageLog.Log(MessageLog::LOG_WARNING, "PresentMonEtw",
											"Lost " + std::to_string(eventsLost) + " events, " +
											std::to_string(buffersLost) + " buffers.");
                    // FIXME: How do we set a threshold here?
                    if (eventsLost > 100) {
                        log_corrupted = true;
                        g_FileComplete = true;
                    }
                }

                if (args.mEtlFileName) {
                    PresentMon_UpdateDeadProcesses(data, deadProcesses);
                }


								if (args.mTimer > 0 && GetTickCount64() - start_time > args.mTimer * 1000) {
									if (!timeSignaled) {
										PostMessage(window, WM_APP + 2, 0, 0);
										timeSignaled = true;
									}
								}
								else
								{
									if (g_FileComplete) {
										g_StopRecording = true;
									}
								}

                Sleep(100);
            }

            PresentMon_Shutdown(data, log_corrupted);
        }

        etwThread.join();
    }

    session.CloseTrace();
    session.DisableProvider(DXGI_PROVIDER_GUID);
    session.DisableProvider(D3D9_PROVIDER_GUID);
    if (!args.mSimple)
    {
        session.DisableProvider(DXGKRNL_PROVIDER_GUID);
        session.DisableProvider(WIN32K_PROVIDER_GUID);
        session.DisableProvider(DWM_PROVIDER_GUID);
    }
    session.Stop();
}

