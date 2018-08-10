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
#include "Utility\StringUtils.h"

template <typename Map, typename F>
static void map_erase_if(Map& m, F pred)
{
  typename Map::iterator i = m.begin();
  while ((i = std::find_if(i, m.end(), pred)) != m.end()) {
    m.erase(i++);
  }
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

tm GetTime()
{
  time_t time_now = time(NULL);
  struct tm tm;
  localtime_s(&tm, &time_now);
  return tm;
}

enum class ProcessType
{
  DXGIProcess,
  WMRProcess,
  SteamVRProcess,
  OculusVRProcess
};

//  mOutputFilename mHotkeySupport mMultiCsv processName -> FileName
//  PATH.EXT        true           true      PROCESSNAME -> PATH-PROCESSNAME-INDEX.EXT
//  PATH.EXT        false          true      PROCESSNAME -> PATH-PROCESSNAME.EXT
//  PATH.EXT        true           false     any         -> PATH-INDEX.EXT
//  PATH.EXT        false          false     any         -> PATH.EXT
//  nullptr         any            any       nullptr     -> PresentMon-TIME.csv
//  nullptr         any            any       PROCESSNAME -> PresentMon-PROCESSNAME-TIME.csv
//
// If wmr, then append _WMR to name.
static void GenerateOutputFilename(const PresentMonData& pm, const char* processName, ProcessType type, char* path, std::string& fileName)
{
  const auto tm = GetTime();

  char ext[_MAX_EXT];

  char file[MAX_PATH];

  if (pm.mArgs->mOutputFileName) {
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char name[_MAX_FNAME];
    _splitpath_s(pm.mArgs->mOutputFileName, drive, dir, name, ext);

    _snprintf_s(path, MAX_PATH, _TRUNCATE, "%s%s", drive, dir);

    int i = _snprintf_s(file, MAX_PATH, _TRUNCATE, "%s", name);

    if (pm.mArgs->mMultiCsv) {
      i += _snprintf_s(file + i, MAX_PATH - i, _TRUNCATE, "-%s", processName);
    }

    if (pm.mArgs->mHotkeySupport) {
      i += _snprintf_s(file + i, MAX_PATH - i, _TRUNCATE, "-%d", pm.mArgs->mRecordingCount);
    }

    i += _snprintf_s(file + i, MAX_PATH - i, _TRUNCATE, "-%4d-%02d-%02dT%02d%02d%02d",
      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
      tm.tm_hour, tm.tm_min, tm.tm_sec);
  }
  else {
    strcpy_s(ext, ".csv");

    if (processName == nullptr) {
      _snprintf_s(file, MAX_PATH, _TRUNCATE, "PresentMon-%4d-%02d-%02dT%02d%02d%02ds",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    else {
      _snprintf_s(file, MAX_PATH, _TRUNCATE, "PresentMon-%s-%4d-%02d-%02dT%02d%02d%02d", processName,
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
  }

  switch (type)
  {
  case ProcessType::WMRProcess:
  {
    strcat_s(file, MAX_PATH, "_WMR");
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    strcat_s(file, MAX_PATH, "_SteamVR");
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    strcat_s(file, MAX_PATH, "_OculusVR");
    break;
  }
  }

  strcat_s(file, MAX_PATH, ext);
  fileName = file;
  strcat_s(path, MAX_PATH, file);
}

static void CreateDXGIOutputFile(PresentMonData& pm, const char* processName, FILE** outputFile, std::string& fileName)
{
  // Open output file and print CSV header
  char outputFilePath[MAX_PATH];
  GenerateOutputFilename(pm, processName, ProcessType::DXGIProcess, outputFilePath, fileName);
  _wfopen_s(outputFile, ConvertUTF8StringToUTF16String(outputFilePath).c_str(), L"w");
  if (*outputFile) {
    fprintf(*outputFile, "Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,PresentFlags");
    if (pm.mDXGIVerbosity > Verbosity::Simple)
    {
      fprintf(*outputFile, ",AllowsTearing,PresentMode");
    }
    if (pm.mDXGIVerbosity >= Verbosity::Verbose)
    {
      fprintf(*outputFile, ",WasBatched,DwmNotified");
    }
    fprintf(*outputFile, ",Dropped,TimeInSeconds,MsBetweenPresents");
    if (pm.mDXGIVerbosity > Verbosity::Simple)
    {
      fprintf(*outputFile, ",MsBetweenDisplayChange");
    }
    fprintf(*outputFile, ",MsInPresentAPI");
    if (pm.mDXGIVerbosity > Verbosity::Simple)
    {
      fprintf(*outputFile, ",MsUntilRenderComplete,MsUntilDisplayed");
    }
    fprintf(*outputFile, "\n");
  }
}
static void CreateLSROutputFile(PresentMonData& pm, const char* processName, FILE** lsrOutputFile, std::string& fileName)
{
  // Open output file and print CSV header
  char outputFilePath[MAX_PATH];
  GenerateOutputFilename(pm, processName, ProcessType::WMRProcess, outputFilePath, fileName);
  _wfopen_s(lsrOutputFile, ConvertUTF8StringToUTF16String(outputFilePath).c_str(), L"w");
  if (*lsrOutputFile) {
    fprintf(*lsrOutputFile, "Application,ProcessID,DwmProcessID");
    if (pm.mLSRVerbosity >= Verbosity::Verbose)
    {
      fprintf(*lsrOutputFile, ",HolographicFrameID");
    }
    fprintf(*lsrOutputFile, ",TimeInSeconds");
    if (pm.mLSRVerbosity > Verbosity::Simple)
    {
      fprintf(*lsrOutputFile, ",MsBetweenAppPresents,MsAppPresentToLsr");
    }
    fprintf(*lsrOutputFile, ",MsBetweenLsrs,AppMissed,LsrMissed");
    if (pm.mLSRVerbosity >= Verbosity::Verbose)
    {
      fprintf(*lsrOutputFile, ",MsSourceReleaseFromRenderingToLsrAcquire,MsAppCpuRenderFrame");
    }
    fprintf(*lsrOutputFile, ",MsAppPoseLatency");
    if (pm.mLSRVerbosity >= Verbosity::Verbose)
    {
      fprintf(*lsrOutputFile, ",MsAppMisprediction,MsLsrCpuRenderFrame");
    }
    fprintf(*lsrOutputFile, ",MsLsrPoseLatency,MsActualLsrPoseLatency,MsTimeUntilVsync,MsLsrThreadWakeupToGpuEnd,MsLsrThreadWakeupError");
    if (pm.mLSRVerbosity >= Verbosity::Verbose)
    {
      fprintf(*lsrOutputFile, ",MsLsrThreadWakeupToCpuRenderFrameStart,MsCpuRenderFrameStartToHeadPoseCallbackStart,MsGetHeadPose,MsHeadPoseCallbackStopToInputLatch,MsInputLatchToGpuSubmission");
    }
    fprintf(*lsrOutputFile, ",MsLsrPreemption,MsLsrExecution,MsCopyPreemption,MsCopyExecution,MsGpuEndToVsync");
    fprintf(*lsrOutputFile, ",AppRenderStart,AppRenderEnd,ReprojectionStart");
    fprintf(*lsrOutputFile, ",ReprojectionEnd,VSync");
    fprintf(*lsrOutputFile, "\n");
  }
}
static void CreateSteamVROutputFile(PresentMonData& pm, const char* processName, FILE** steamvrOutputFile, std::string& fileName)
{
  // Open output file and print CSV header
  char outputFilePath[MAX_PATH];
  GenerateOutputFilename(pm, processName, ProcessType::SteamVRProcess, outputFilePath, fileName);
  _wfopen_s(steamvrOutputFile, ConvertUTF8StringToUTF16String(outputFilePath).c_str(), L"w");
  if (*steamvrOutputFile) {
    fprintf(*steamvrOutputFile, "Application,ProcessID");
    fprintf(*steamvrOutputFile, ",MsBetweenAppPresents,MsBetweenReprojections");
    fprintf(*steamvrOutputFile, ",AppRenderStart,AppRenderEnd");
    fprintf(*steamvrOutputFile, ",ReprojectionStart,ReprojectionEnd,VSync");
    fprintf(*steamvrOutputFile, ",AppMissed,WarpMissed");
    fprintf(*steamvrOutputFile, "\n");
  }
}
static void CreateOculusVROutputFile(PresentMonData& pm, const char* processName, FILE** oculusvrOutputFile, std::string& fileName)
{
  // Open output file and print CSV header
  char outputFilePath[MAX_PATH];
  GenerateOutputFilename(pm, processName, ProcessType::OculusVRProcess, outputFilePath, fileName);
  _wfopen_s(oculusvrOutputFile, ConvertUTF8StringToUTF16String(outputFilePath).c_str(), L"w");
  if (*oculusvrOutputFile) {
    fprintf(*oculusvrOutputFile, "Application,ProcessID");
    fprintf(*oculusvrOutputFile, ",MsBetweenAppPresents,MsBetweenReprojections");
    fprintf(*oculusvrOutputFile, ",AppRenderStart,AppRenderEnd");
    fprintf(*oculusvrOutputFile, ",ReprojectionStart,ReprojectionEnd,VSync");
    fprintf(*oculusvrOutputFile, ",AppMissed,WarpMissed");
    fprintf(*oculusvrOutputFile, "\n");
  }
}

static void TerminateProcess(PresentMonData& pm, ProcessInfo const& proc, ProcessType type)
{
  if (!proc.mTargetProcess) {
    return;
  }

  switch (type)
  {
  case ProcessType::DXGIProcess:
  {
    // Save the output file in case the process is re-started
    pm.mDXGIProcessOutputFile.emplace(proc.mModuleName, proc.mOutputFile);
    break;
  }
  case ProcessType::WMRProcess:
  {
    // Save the output file in case the process is re-started
    pm.mWMRProcessOutputFile.emplace(proc.mModuleName, proc.mOutputFile);
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    // Save the output file in case the process is re-started
    pm.mSteamVRProcessOutputFile.emplace(proc.mModuleName, proc.mOutputFile);
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    // Save the output file in case the process is re-started
    pm.mOculusVRProcessOutputFile.emplace(proc.mModuleName, proc.mOutputFile);
    break;
  }
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


static void StopProcess(PresentMonData& pm, ProcessType type, std::map<uint32_t, ProcessInfo>::iterator it)
{
  TerminateProcess(pm, it->second, type);
  switch (type)
  {
  case ProcessType::DXGIProcess:
  {
    pm.mDXGIProcessMap.erase(it);
    break;
  }
  case ProcessType::WMRProcess:
  {
    pm.mWMRProcessMap.erase(it);
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    pm.mSteamVRProcessMap.erase(it);
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    pm.mOculusVRProcessMap.erase(it);
    break;
  }
  }
}

static void StopProcess(PresentMonData& pm, uint32_t processId)
{
  // called from NTProcesses -> they are DXGI?
  auto it = pm.mDXGIProcessMap.find(processId);
  if (it != pm.mDXGIProcessMap.end()) {
    StopProcess(pm, ProcessType::DXGIProcess, it);
  }
}

static ProcessInfo* StartNewProcess(PresentMonData& pm, ProcessType type, ProcessInfo* proc, uint32_t processId, std::string const& imageFileName, uint64_t now)
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

  switch (type)
  {
  case ProcessType::DXGIProcess:
  {
    auto it = pm.mDXGIProcessOutputFile.find(imageFileName);
    if (it == pm.mDXGIProcessOutputFile.end()) {
      CreateDXGIOutputFile(pm, imageFileName.c_str(), &proc->mOutputFile, proc->mFileName);
    }
    else {
      proc->mOutputFile = it->second;
      pm.mDXGIProcessOutputFile.erase(it);
    }
    break;
  }
  case ProcessType::WMRProcess:
  {
    auto it = pm.mWMRProcessOutputFile.find(imageFileName);
    if (it == pm.mWMRProcessOutputFile.end()) {
      CreateLSROutputFile(pm, imageFileName.c_str(), &proc->mOutputFile, proc->mFileName);
    }
    else {
      proc->mOutputFile = it->second;
      pm.mWMRProcessOutputFile.erase(it);
    }
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    auto it = pm.mSteamVRProcessOutputFile.find(imageFileName);
    if (it == pm.mSteamVRProcessOutputFile.end()) {
      CreateSteamVROutputFile(pm, imageFileName.c_str(), &proc->mOutputFile, proc->mFileName);
    }
    else {
      proc->mOutputFile = it->second;
      pm.mSteamVRProcessOutputFile.erase(it);
    }
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    auto it = pm.mOculusVRProcessOutputFile.find(imageFileName);
    if (it == pm.mOculusVRProcessOutputFile.end()) {
      CreateOculusVROutputFile(pm, imageFileName.c_str(), &proc->mOutputFile, proc->mFileName);
    }
    else {
      proc->mOutputFile = it->second;
      pm.mOculusVRProcessOutputFile.erase(it);
    }
    break;
  }
  }

  // Include process in -terminate_on_proc_exit count
  if (pm.mArgs->mTerminateOnProcExit) {
    pm.mTerminationProcessCount += 1;
  }

  return proc;
}

static ProcessInfo* StartProcess(PresentMonData& pm, uint32_t processId, ProcessType type, std::string const& imageFileName, uint64_t now)
{
  ProcessInfo* proc = nullptr;
  switch (type)
  {
  case ProcessType::DXGIProcess:
  {
    auto it = pm.mDXGIProcessMap.find(processId);
    if (it != pm.mDXGIProcessMap.end()) {
      StopProcess(pm, type, it);
    }
    proc = &pm.mDXGIProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::WMRProcess:
  {
    auto it = pm.mWMRProcessMap.find(processId);
    if (it != pm.mWMRProcessMap.end()) {
      StopProcess(pm, type, it);
    }
    proc = &pm.mWMRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    auto it = pm.mSteamVRProcessMap.find(processId);
    if (it != pm.mSteamVRProcessMap.end()) {
      StopProcess(pm, type, it);
    }
    proc = &pm.mSteamVRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    auto it = pm.mOculusVRProcessMap.find(processId);
    if (it != pm.mOculusVRProcessMap.end()) {
      StopProcess(pm, type, it);
    }
    proc = &pm.mOculusVRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  }

  return StartNewProcess(pm, type, proc, processId, imageFileName, now);
}

static ProcessInfo* StartProcessIfNew(PresentMonData& pm, uint32_t processId, ProcessType type, uint64_t now)
{
  ProcessInfo* proc = nullptr;
  switch (type)
  {
  case ProcessType::DXGIProcess:
  {
    auto it = pm.mDXGIProcessMap.find(processId);
    if (it != pm.mDXGIProcessMap.end()) {
      auto proc = &it->second;
      return proc->mTargetProcess ? proc : nullptr;
    }
    proc = &pm.mDXGIProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::WMRProcess:
  {
    auto it = pm.mWMRProcessMap.find(processId);
    if (it != pm.mWMRProcessMap.end()) {
      auto proc = &it->second;
      return proc->mTargetProcess ? proc : nullptr;
    }
    proc = &pm.mWMRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::SteamVRProcess:
  {
    auto it = pm.mSteamVRProcessMap.find(processId);
    if (it != pm.mSteamVRProcessMap.end()) {
      auto proc = &it->second;
      return proc->mTargetProcess ? proc : nullptr;
    }
    proc = &pm.mSteamVRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
  case ProcessType::OculusVRProcess:
  {
    auto it = pm.mOculusVRProcessMap.find(processId);
    if (it != pm.mOculusVRProcessMap.end()) {
      auto proc = &it->second;
      return proc->mTargetProcess ? proc : nullptr;
    }
    proc = &pm.mOculusVRProcessMap.emplace(processId, ProcessInfo()).first->second;
    break;
  }
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
  return StartNewProcess(pm, type, proc, processId, imageFileName, now);
}

static bool UpdateProcessInfo_Realtime(PresentMonData& pm, ProcessType type, ProcessInfo& info, uint64_t now, uint32_t thisPid)
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

      TerminateProcess(pm, info, type);
      StartNewProcess(pm, type, &info, thisPid, name, now);
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

void AddLateStageReprojection(PresentMonData& pm, LateStageReprojectionEvent& p, uint64_t now, uint64_t perfFreq)
{
  const uint32_t appProcessId = p.GetAppProcessId();
  auto proc = StartProcessIfNew(pm, appProcessId, ProcessType::WMRProcess, now);
  if (proc == nullptr) {
    return; // process is not a target
  }

  if ((pm.mLSRVerbosity > Verbosity::Simple) && (appProcessId == 0)) {
    return; // Incomplete event data
  }

  pm.mLateStageReprojectionData.AddLateStageReprojection(p);

  auto file = proc->mOutputFile;
  if (file && (p.FinalState == LateStageReprojectionResult::Presented || !pm.mArgs->mExcludeDropped)) {
    auto len = pm.mLateStageReprojectionData.mLSRHistory.size();
    if (len > 1) {
      auto& curr = pm.mLateStageReprojectionData.mLSRHistory[len - 1];
      auto& prev = pm.mLateStageReprojectionData.mLSRHistory[len - 2];
      const double deltaMilliseconds = 1000 * double(curr.QpcTime - prev.QpcTime) / perfFreq;
      const double timeInSeconds = (double)(int64_t)(p.QpcTime - pm.mStartupQpcTime) / perfFreq;

      fprintf(file, "%s,%d,%d", proc->mModuleName.c_str(), curr.GetAppProcessId(), curr.ProcessId);
      if (pm.mLSRVerbosity >= Verbosity::Verbose)
      {
        fprintf(file, ",%d", curr.GetAppFrameId());
      }
      fprintf(file, ",%.6lf", timeInSeconds);
      if (pm.mLSRVerbosity > Verbosity::Simple)
      {
      double appPresentDeltaMilliseconds = 0.0;
      double appPresentToLsrMilliseconds = 0.0;
      if (curr.IsValidAppFrame())
        {
          const uint64_t currAppPresentTime = curr.GetAppPresentTime();
          appPresentToLsrMilliseconds = 1000 * double(curr.QpcTime - currAppPresentTime) / perfFreq;
          if (prev.IsValidAppFrame() && (curr.GetAppProcessId() == prev.GetAppProcessId()))
          {
            const uint64_t prevAppPresentTime = prev.GetAppPresentTime();
            appPresentDeltaMilliseconds = 1000 * double(currAppPresentTime - prevAppPresentTime) / perfFreq;
          }
        }
        fprintf(file, ",%.6lf,%.6lf", appPresentDeltaMilliseconds, appPresentToLsrMilliseconds);
      }
      fprintf(file, ",%.6lf,%d,%d", deltaMilliseconds, !curr.NewSourceLatched, curr.MissedVsyncCount);
      if (pm.mLSRVerbosity >= Verbosity::Verbose)
      {
        fprintf(file, ",%.6lf,%.6lf", 1000 * double(curr.Source.GetReleaseFromRenderingToAcquireForPresentationTime()) / perfFreq, 1000 * double(curr.GetAppCpuRenderFrameTime()) / perfFreq);
      }
      fprintf(file, ",%.6lf", curr.AppPredictionLatencyMs);
      if (pm.mLSRVerbosity >= Verbosity::Verbose)
      {
        fprintf(file, ",%.6lf,%.6lf", curr.AppMispredictionMs, curr.GetLsrCpuRenderFrameMs());
      }
      fprintf(file, ",%.6lf,%.6lf,%.6lf,%.6lf,%.6lf",
        curr.LsrPredictionLatencyMs,
        curr.GetLsrMotionToPhotonLatencyMs(),
        curr.TimeUntilVsyncMs,
        curr.GetLsrThreadWakeupStartLatchToGpuEndMs(),
        curr.TotalWakeupErrorMs);
      if (pm.mLSRVerbosity >= Verbosity::Verbose)
      {
        fprintf(file, ",%.6lf,%.6lf,%.6lf,%.6lf,%.6lf",
          curr.ThreadWakeupStartLatchToCpuRenderFrameStartInMs,
          curr.CpuRenderFrameStartToHeadPoseCallbackStartInMs,
          curr.HeadPoseCallbackStartToHeadPoseCallbackStopInMs,
          curr.HeadPoseCallbackStopToInputLatchInMs,
          curr.InputLatchToGpuSubmissionInMs);
      }
      fprintf(file, ",%.6lf,%.6lf,%.6lf,%.6lf,%.6lf",
        curr.GpuSubmissionToGpuStartInMs,
        curr.GpuStartToGpuStopInMs,
        curr.GpuStopToCopyStartInMs,
        curr.CopyStartToCopyStopInMs,
        curr.CopyStopToVsyncInMs);

      const double appStartTime = (double)(curr.GetAppStartTime() - pm.mStartupQpcTime) / perfFreq;
      const double appEndTime = (double)(curr.GetAppPresentTime() - pm.mStartupQpcTime) / perfFreq;
      fprintf(file, ",%.6lf,%.6lf", appStartTime, appEndTime);
      const double compEndTime = ((double)(curr.QpcTime - pm.mStartupQpcTime) / perfFreq) + (curr.GetLsrThreadWakeupStartLatchToGpuEndMs() * 0.001);
      fprintf(file, ",%.6lf,%.6lf", timeInSeconds, compEndTime);
      const double VSync = ((double)(curr.VSyncIndicator - pm.mStartupQpcTime) / perfFreq) + (curr.TimeUntilVsyncMs * 0.001);
      fprintf(file, ",%.6lf", VSync);
      fprintf(file, "\n");

      PresentFrameInfo frameInfo;

      if (!curr.NewSourceLatched) {
        if (curr.MissedVsyncCount) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARP;
        }
      }
      else {
        if (curr.MissedVsyncCount) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARP;
        }
      }

      if (pm.mArgs->mPresentCallback)
      {
        pm.mArgs->mPresentCallback(proc->mFileName, proc->mModuleName, CompositorInfo::WMR, timeInSeconds, deltaMilliseconds, frameInfo);
      }
    }
  }

  pm.mLateStageReprojectionData.UpdateLateStageReprojectionInfo(now, perfFreq);
}

enum {
  MAX_HISTORY_TIME = 2000,
  CHAIN_TIMEOUT_THRESHOLD_TICKS = 10000, // 10 sec
  MAX_PRESENTS_IN_DEQUE = 60 * (MAX_HISTORY_TIME / 1000)
};

void AddSteamVREvent(PresentMonData& pm, SteamVREvent& p, uint64_t now, uint64_t perfFreq)
{
  const uint32_t appProcessId = p.ProcessId;

  auto proc = StartProcessIfNew(pm, appProcessId, ProcessType::SteamVRProcess, now);
  if (proc == nullptr) {
    return; // process is not a target
  }

  pm.mSVRData.AddCompositorPresent(p);

  auto file = proc->mOutputFile;
  if (file) {
    auto len = pm.mSVRData.mPresentHistory.size();
    if (len > 1) {
      auto& curr = pm.mSVRData.mPresentHistory[len - 1];
      auto& prev = pm.mSVRData.mPresentHistory[len - 2];
      double deltaMillisecondsApp = 0;
      if (curr.AppRenderStart && (prev.AppRenderStart || pm.mSVRData.mLastAppFrameStart)) {
      if (prev.AppRenderStart) {
        deltaMillisecondsApp = 1000 * double(curr.AppRenderStart - prev.AppRenderStart) / perfFreq;
      }
      else {
        deltaMillisecondsApp = 1000 * double(curr.AppRenderStart - pm.mSVRData.mLastAppFrameStart) / perfFreq;
      }
      pm.mSVRData.mLastAppFrameStart = curr.AppRenderStart;
      }

    double deltaMillisecondsReprojection = 0;
    if (curr.ReprojectionStart && prev.ReprojectionEnd) {
      deltaMillisecondsReprojection = 1000 * double(curr.ReprojectionStart - prev.ReprojectionStart) / perfFreq;
    }

    double appRenderStart = 0;
    if (p.AppRenderStart) {
      appRenderStart = (double)(int64_t)(p.AppRenderStart - pm.mStartupQpcTime) / perfFreq;
    }

      const double appRenderEnd = (double)(int64_t)(p.AppRenderEnd - pm.mStartupQpcTime) / perfFreq;
      const double reprojectionStart = (double)(int64_t)(p.ReprojectionStart - pm.mStartupQpcTime) / perfFreq;
      const double reprojectionEnd = (double)(int64_t)(p.ReprojectionEnd - pm.mStartupQpcTime) / perfFreq;
      const double VSync = ((double)(int64_t)(p.TimeStampSinceLastVSync - pm.mStartupQpcTime) / perfFreq)
        - (p.MsSinceLastVSync * 0.001);

      PresentFrameInfo frameInfo;

      if (p.AppMiss) {
        if (p.WarpMiss) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARP;
        }
      }
      else {
        if (p.WarpMiss) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARP;
        }
      }

      if (pm.mArgs->mPresentCallback)
      {
        pm.mArgs->mPresentCallback(proc->mFileName, proc->mModuleName, CompositorInfo::SteamVR, appRenderStart, deltaMillisecondsApp, frameInfo);
      }

      fprintf(file, "%s,%d", proc->mModuleName.c_str(), appProcessId);
      fprintf(file, ",%.6lf,%.6lf", deltaMillisecondsApp, deltaMillisecondsReprojection);
      fprintf(file, ",%.6lf", appRenderStart);
      fprintf(file, ",%.6lf", p.AppRenderEnd ? appRenderEnd : 0);
      fprintf(file, ",%.6lf", p.ReprojectionStart ? reprojectionStart : 0);
      fprintf(file, ",%.6lf", p.ReprojectionEnd ? reprojectionEnd : 0);
      fprintf(file, ",%.6lf", VSync);
      fprintf(file, ",%d,%d", p.AppMiss, p.WarpMiss);
      fprintf(file, "\n");
    }
  }

  pm.mSVRData.PruneDeque(perfFreq, MAX_HISTORY_TIME, MAX_PRESENTS_IN_DEQUE);
}

void AddOculusVREvent(PresentMonData& pm, OculusVREvent& p, uint64_t now, uint64_t perfFreq)
{
  const uint32_t appProcessId = p.ProcessId;
  auto proc = StartProcessIfNew(pm, appProcessId, ProcessType::OculusVRProcess, now);
  if (proc == nullptr) {
    return; // process is not a target
  }

  pm.mOVRData.AddCompositorPresent(p);

  auto file = proc->mOutputFile;
  if (file) {
    auto len = pm.mOVRData.mPresentHistory.size();
    if (len > 1) {
      auto& curr = pm.mOVRData.mPresentHistory[len - 1];
      auto& prev = pm.mOVRData.mPresentHistory[len - 2];
    double deltaMillisecondsApp = 0;
    if (curr.AppRenderStart && (prev.AppRenderStart || pm.mOVRData.mLastAppFrameStart)) {
      if (prev.AppRenderStart) {
        deltaMillisecondsApp = 1000 * double(curr.AppRenderStart - prev.AppRenderStart) / perfFreq;
      }
      else {
        deltaMillisecondsApp = 1000 * double(curr.AppRenderStart - pm.mOVRData.mLastAppFrameStart) / perfFreq;
      }
      pm.mOVRData.mLastAppFrameStart = curr.AppRenderStart;
    }
    // ...
    double deltaMillisecondsReprojection = 0;
    if (curr.ReprojectionStart && prev.ReprojectionEnd) {
      deltaMillisecondsReprojection = 1000 * double(curr.ReprojectionStart - prev.ReprojectionStart) / perfFreq;
    }

    double appRenderStart = 0;
    if (p.AppRenderStart) {
      appRenderStart = (double)(int64_t)(p.AppRenderStart - pm.mStartupQpcTime) / perfFreq;
    }

      const double appRenderEnd = (double)(int64_t)(p.AppRenderEnd - pm.mStartupQpcTime) / perfFreq;
      const double reprojectionStart = (double)(int64_t)(p.ReprojectionStart - pm.mStartupQpcTime) / perfFreq;
      const double reprojectionEnd = (double)(int64_t)(p.ReprojectionEnd - pm.mStartupQpcTime) / perfFreq;

      // guess VSync based on EndSpinWait Event
      const double VSync = ((double)(int64_t)(p.EndSpinWait - pm.mStartupQpcTime) / perfFreq) + 0.0085;

      PresentFrameInfo frameInfo;

      if (p.AppMiss) {
        if (p.WarpMiss) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARP;
        }
      }
      else {
        if (p.WarpMiss) {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARPMISS;
        }
        else {
          frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARP;
        }
      }

      if (pm.mArgs->mPresentCallback)
      {
        pm.mArgs->mPresentCallback(proc->mFileName, proc->mModuleName, CompositorInfo::OculusVR, appRenderStart, deltaMillisecondsApp, frameInfo);
      }

    fprintf(file, "%s,%d", proc->mModuleName.c_str(), appProcessId);
    fprintf(file, ",%.6lf,%.6lf", deltaMillisecondsApp, deltaMillisecondsReprojection);
    fprintf(file, ",%.6lf", appRenderStart);
    fprintf(file, ",%.6lf", p.AppRenderEnd ? appRenderEnd : 0);
    fprintf(file, ",%.6lf", p.ReprojectionStart ? reprojectionStart : 0);
    fprintf(file, ",%.6lf", p.ReprojectionEnd ? reprojectionEnd : 0);
    fprintf(file, ",%.6lf", VSync);
    fprintf(file, ",%d,%d", p.AppMiss, p.WarpMiss);
    fprintf(file, "\n");
    }
  }
  pm.mOVRData.PruneDeque(perfFreq, MAX_HISTORY_TIME, MAX_PRESENTS_IN_DEQUE);
}

void AddPresent(PresentMonData& pm, PresentEvent& p, uint64_t now, uint64_t perfFreq)
{
  const uint32_t appProcessId = p.ProcessId;
  auto proc = StartProcessIfNew(pm, appProcessId, ProcessType::DXGIProcess, now);
  if (proc == nullptr) {
    return; // process is not a target
  }

  auto& chain = proc->mChainMap[p.SwapChainAddress];
  chain.AddPresentToSwapChain(p);

  auto file = proc->mOutputFile;
  if (file && (p.FinalState == PresentResult::Presented || !pm.mArgs->mExcludeDropped)) {
    auto len = chain.mPresentHistory.size();
    auto displayedLen = chain.mDisplayedPresentHistory.size();
    if (len > 1) {
    auto& curr = chain.mPresentHistory[len - 1];
    auto& prev = chain.mPresentHistory[len - 2];
    const double deltaMilliseconds = 1000 * double(curr.QpcTime - prev.QpcTime) / perfFreq;
    const double deltaReady = curr.ReadyTime == 0 ? 0.0 : (1000 * double(curr.ReadyTime - curr.QpcTime) / perfFreq);
    const double deltaDisplayed = curr.FinalState == PresentResult::Presented ? (1000 * double(curr.ScreenTime - curr.QpcTime) / perfFreq) : 0.0;
    const double timeTakenMilliseconds = 1000 * double(curr.TimeTaken) / perfFreq;

    double timeSincePreviousDisplayed = 0.0;
    if (curr.FinalState == PresentResult::Presented && displayedLen > 1) {
      assert(chain.mDisplayedPresentHistory[displayedLen - 1].QpcTime == curr.QpcTime);
      auto& prevDisplayed = chain.mDisplayedPresentHistory[displayedLen - 2];
      timeSincePreviousDisplayed = 1000 * double(curr.ScreenTime - prevDisplayed.ScreenTime) / perfFreq;
    }

    const double timeInSeconds = (double)(int64_t)(p.QpcTime - pm.mStartupQpcTime) / perfFreq;

    PresentFrameInfo frameInfo;

    // always set to WARP (so we don't count any warp misses here)
    if (curr.FinalState == PresentResult::Presented) {
        frameInfo = PresentFrameInfo::COMPOSITOR_APP_WARP;
    } else {
        frameInfo = PresentFrameInfo::COMPOSITOR_APPMISS_WARP;
    }

    if (pm.mArgs->mPresentCallback)
    {
      pm.mArgs->mPresentCallback(proc->mFileName, proc->mModuleName, CompositorInfo::DWM, timeInSeconds, deltaMilliseconds, frameInfo);
    }

    fprintf(file, "%s,%d,0x%016llX,%s,%d,%d",
      proc->mModuleName.c_str(), appProcessId, p.SwapChainAddress, RuntimeToString(p.Runtime), curr.SyncInterval, curr.PresentFlags);
    if (pm.mDXGIVerbosity > Verbosity::Simple)
    {
      fprintf(file, ",%d,%s", curr.SupportsTearing, PresentModeToString(curr.PresentMode));
    }
    if (pm.mDXGIVerbosity >= Verbosity::Verbose)
    {
      fprintf(file, ",%d,%d", curr.WasBatched, curr.DwmNotified);
    }
    fprintf(file, ",%s,%.6lf,%.3lf", FinalStateToDroppedString(curr.FinalState), timeInSeconds, deltaMilliseconds);
    if (pm.mDXGIVerbosity > Verbosity::Simple)
    {
      fprintf(file, ",%.3lf", timeSincePreviousDisplayed);
    }
    fprintf(file, ",%.3lf", timeTakenMilliseconds);
    if (pm.mDXGIVerbosity > Verbosity::Simple)
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

  // in case we have an unset verbosity option, set verbosity to value of commandline argument
  if (pm.mDXGIVerbosity == Verbosity::Default) pm.mDXGIVerbosity = args.mVerbosity;
  if (pm.mLSRVerbosity == Verbosity::Default) pm.mLSRVerbosity = args.mVerbosity;
  if (pm.mSVRVerbosity == Verbosity::Default) pm.mSVRVerbosity = args.mVerbosity;
  if (pm.mOVRVerbosity == Verbosity::Default) pm.mOVRVerbosity = args.mVerbosity;

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
}

void PresentMon_Update(PresentMonData& pm, std::vector<std::shared_ptr<PresentEvent>>& presents, 
  std::vector<std::shared_ptr<LateStageReprojectionEvent>>& lsrs,
  std::vector<std::shared_ptr<SteamVREvent>>& svrevents,
  std::vector<std::shared_ptr<OculusVREvent>>& ovrevents,
  uint64_t now, uint64_t perfFreq)
{
  // store the new presents into processes
  for (auto& p : presents)
  {
    AddPresent(pm, *p, now, perfFreq);
  }

  // store the new lsrs
  for (auto& p : lsrs)
  {
    AddLateStageReprojection(pm, *p, now, perfFreq);
  }

  // store the new SteamVR events
  for (auto& p : svrevents)
  {
    AddSteamVREvent(pm, *p, now, perfFreq);
  }

  // store the new OculusVR events
  for (auto& p : ovrevents)
  {
    AddOculusVREvent(pm, *p, now, perfFreq);
  }

  // Update realtime process info
  if (!pm.mArgs->mEtlFileName) {
    std::vector<std::map<uint32_t, ProcessInfo>::iterator> removeDXGI;
    for (auto ii = pm.mDXGIProcessMap.begin(), ie = pm.mDXGIProcessMap.end(); ii != ie; ++ii) {
      if (!UpdateProcessInfo_Realtime(pm, ProcessType::DXGIProcess, ii->second, now, ii->first)) {
        removeDXGI.emplace_back(ii);
      }
    }
    for (auto ii : removeDXGI) {
      StopProcess(pm, ProcessType::DXGIProcess, ii);
    }

    std::vector<std::map<uint32_t, ProcessInfo>::iterator> removeWMR;
    for (auto ii = pm.mWMRProcessMap.begin(), ie = pm.mWMRProcessMap.end(); ii != ie; ++ii) {
      if (!UpdateProcessInfo_Realtime(pm, ProcessType::WMRProcess, ii->second, now, ii->first)) {
        removeWMR.emplace_back(ii);
      }
    }
    for (auto ii : removeWMR) {
      StopProcess(pm, ProcessType::WMRProcess, ii);
    }

    std::vector<std::map<uint32_t, ProcessInfo>::iterator> removeSteamVR;
    for (auto ii = pm.mSteamVRProcessMap.begin(), ie = pm.mSteamVRProcessMap.end(); ii != ie; ++ii) {
      if (!UpdateProcessInfo_Realtime(pm, ProcessType::SteamVRProcess, ii->second, now, ii->first)) {
        removeSteamVR.emplace_back(ii);
      }
    }
    for (auto ii : removeSteamVR) {
      StopProcess(pm, ProcessType::SteamVRProcess, ii);
    }

  std::vector<std::map<uint32_t, ProcessInfo>::iterator> removeOculusVR;
  for (auto ii = pm.mOculusVRProcessMap.begin(), ie = pm.mOculusVRProcessMap.end(); ii != ie; ++ii) {
    if (!UpdateProcessInfo_Realtime(pm, ProcessType::OculusVRProcess, ii->second, now, ii->first)) {
      removeOculusVR.emplace_back(ii);
    }
  }
  for (auto ii : removeOculusVR) {
    StopProcess(pm, ProcessType::OculusVRProcess, ii);
  }
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
  CloseFile(pm.mLsrOutputFile, totalEventsLost, totalBuffersLost);
  pm.mOutputFile = nullptr;
  pm.mLsrOutputFile = nullptr;

  for (auto& p : pm.mDXGIProcessMap) {
    auto proc = &p.second;
    CloseFile(proc->mOutputFile, totalEventsLost, totalBuffersLost);
  }

  for (auto& p : pm.mWMRProcessMap) {
    auto proc = &p.second;
    CloseFile(proc->mOutputFile, totalEventsLost, totalBuffersLost);
  }

  for (auto& p : pm.mSteamVRProcessMap) {
    auto proc = &p.second;
    CloseFile(proc->mOutputFile, totalEventsLost, totalBuffersLost);
  }

  for (auto& p : pm.mOculusVRProcessMap) {
    auto proc = &p.second;
    CloseFile(proc->mOutputFile, totalEventsLost, totalBuffersLost);
  }

  for (auto& p : pm.mDXGIProcessOutputFile) {
    CloseFile(p.second, totalEventsLost, totalBuffersLost);
  }
  for (auto& p : pm.mWMRProcessOutputFile) {
    CloseFile(p.second, totalEventsLost, totalBuffersLost);
  }
  for (auto& p : pm.mSteamVRProcessOutputFile) {
    CloseFile(p.second, totalEventsLost, totalBuffersLost);
  }
  for (auto& p : pm.mOculusVRProcessOutputFile) {
    CloseFile(p.second, totalEventsLost, totalBuffersLost);
  }

  pm.mDXGIProcessMap.clear();
  pm.mDXGIProcessOutputFile.clear();
  pm.mWMRProcessMap.clear();
  pm.mWMRProcessOutputFile.clear();
  pm.mSteamVRProcessMap.clear();
  pm.mSteamVRProcessOutputFile.clear();
  pm.mOculusVRProcessMap.clear();
  pm.mOculusVRProcessOutputFile.clear();
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

void ProcessProviderConfig(ProviderConfig& config, std::string provider, const CommandLineArgs& args) {
  // Set default behavior in case no user data is specified
  config.recordingDetail = args.mVerbosity;
  config.enabled = true;
  auto it = args.mProviders.find(provider);
  if (it != args.mProviders.end()) {
    if (it->second.recordingDetail != Verbosity::Default) {
      // user did set a specific recording detail
      config.recordingDetail = it->second.recordingDetail;
    }
    config.enabled = it->second.enabled;
  }
}

void EtwConsumingThread(const CommandLineArgs& args)
{
  Sleep(args.mDelay * 1000);
  if (EtwThreadsShouldQuit()) {
    return;
  }

  PresentMonData data;
  TraceSession session;

  ProviderConfig config;
  // SteamVR
  ProcessProviderConfig(config, "SteamVR", args);
  data.mSVRVerbosity = config.recordingDetail;
  SteamVRTraceConsumer svrConsumer(config.recordingDetail == Verbosity::Simple);
  if (config.enabled) {
    session.AddProviderAndHandler(STEAMVR_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0, 0, (EventHandlerFn)&HandleSteamVREvent, &svrConsumer);
  }

  // Oculus LibOVR
  ProcessProviderConfig(config, "OculusVR", args);
  data.mOVRVerbosity = config.recordingDetail;
  OculusVRTraceConsumer ovrConsumer(config.recordingDetail == Verbosity::Simple);
  if (config.enabled) {
    session.AddProviderAndHandler(OCULUSVR_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0, 0, (EventHandlerFn)&HandleOculusVREvent, &ovrConsumer);
  }

  //WMR
  ProcessProviderConfig(config, "WMR", args);
  data.mLSRVerbosity = config.recordingDetail;
  MRTraceConsumer mrConsumer(config.recordingDetail == Verbosity::Simple);
  if (config.enabled) {
    session.AddProviderAndHandler(DHD_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0x1C00000, 0, (EventHandlerFn)&HandleDHDEvent, &mrConsumer);
    if (config.recordingDetail != Verbosity::Simple) {
      session.AddProviderAndHandler(SPECTRUMCONTINUOUS_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0x800000, 0, (EventHandlerFn)&HandleSpectrumContinuousEvent, &mrConsumer);
    }
  }

  // DXG
  ProcessProviderConfig(config, "DXGI", args);
  data.mDXGIVerbosity = config.recordingDetail;
  PMTraceConsumer pmConsumer(config.recordingDetail == Verbosity::Simple);
  if (config.enabled) {
    session.AddProviderAndHandler(DXGI_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn)&HandleDXGIEvent, &pmConsumer);
    session.AddProviderAndHandler(D3D9_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0, 0, (EventHandlerFn)&HandleD3D9Event, &pmConsumer);
    if (config.recordingDetail != Verbosity::Simple) {
      session.AddProviderAndHandler(DXGKRNL_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 1, 0, (EventHandlerFn)&HandleDXGKEvent, &pmConsumer);
      session.AddProviderAndHandler(WIN32K_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 0x1000, 0, (EventHandlerFn)&HandleWin32kEvent, &pmConsumer);
      session.AddProviderAndHandler(DWM_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0, 0, (EventHandlerFn)&HandleDWMEvent, &pmConsumer);
      session.AddProviderAndHandler(Win7::DWM_PROVIDER_GUID, TRACE_LEVEL_VERBOSE, 0, 0, (EventHandlerFn)&HandleDWMEvent, &pmConsumer);
      session.AddProvider(Win7::DXGKRNL_PROVIDER_GUID, TRACE_LEVEL_INFORMATION, 1, 0);
    }
    session.AddHandler(NT_PROCESS_EVENT_GUID, (EventHandlerFn)&HandleNTProcessEvent, &pmConsumer);
    session.AddHandler(Win7::DXGKBLT_GUID, (EventHandlerFn)&Win7::HandleDxgkBlt, &pmConsumer);
    session.AddHandler(Win7::DXGKFLIP_GUID, (EventHandlerFn)&Win7::HandleDxgkFlip, &pmConsumer);
    session.AddHandler(Win7::DXGKPRESENTHISTORY_GUID, (EventHandlerFn)&Win7::HandleDxgkPresentHistory, &pmConsumer);
    session.AddHandler(Win7::DXGKQUEUEPACKET_GUID, (EventHandlerFn)&Win7::HandleDxgkQueuePacket, &pmConsumer);
    session.AddHandler(Win7::DXGKVSYNCDPC_GUID, (EventHandlerFn)&Win7::HandleDxgkVSyncDPC, &pmConsumer);
    session.AddHandler(Win7::DXGKMMIOFLIP_GUID, (EventHandlerFn)&Win7::HandleDxgkMMIOFlip, &pmConsumer);
  }
  if (!(args.mEtlFileName == nullptr
    ? session.InitializeRealtime("PresentMon", &EtwThreadsShouldQuit)
    : session.InitializeEtlFile(args.mEtlFileName, &EtwThreadsShouldQuit))) {
    return;
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
      std::vector<std::shared_ptr<LateStageReprojectionEvent>> lsrs;
      std::vector<std::shared_ptr<SteamVREvent>> svrevents;
      std::vector<std::shared_ptr<OculusVREvent>> ovrevents;
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
        lsrs.clear();
        svrevents.clear();
        ovrevents.clear();
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
            StartProcess(data, ntProcessEvent.ProcessId, ProcessType::DXGIProcess, ntProcessEvent.ImageFileName, now);
          }
        }

        pmConsumer.DequeuePresents(presents);
        mrConsumer.DequeueLSRs(lsrs);
        svrConsumer.DequeueEvents(svrevents);
        ovrConsumer.DequeueEvents(ovrevents);
        if (args.mScrollLockToggle && (GetKeyState(VK_SCROLL) & 1) == 0) {
          presents.clear();
          lsrs.clear();
          svrevents.clear();
          ovrevents.clear();
        }

        auto doneProcessingEvents = g_EtwProcessingThreadProcessing ? false : true;
        PresentMon_Update(data, presents, lsrs, svrevents, ovrevents, now, session.frequency_);

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