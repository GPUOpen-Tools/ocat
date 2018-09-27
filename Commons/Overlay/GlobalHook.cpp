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

#include "GlobalHook.h"
#include <tlhelp32.h>
#include "../Utility/Constants.h"
#include "../Utility/FileDirectory.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "../Utility/StringUtils.h"

void GlobalHook::Activate()
{
  g_messageLog.LogInfo("GlobalHook", " Starting global hooks");
  StartGlobalHookProcess(
    globalHookProcess32_,
    g_fileDirectory.GetDirectory(DirectoryType::Bin) + g_globalHookProcess32);
  StartGlobalHookProcess(
    globalHookProcess64_,
    g_fileDirectory.GetDirectory(DirectoryType::Bin) + g_globalHookProcess64);
}

void GlobalHook::Deactivate()
{
  g_messageLog.LogInfo("GlobalHook", " Stopping global hooks");
  StopGlobalHookProcess(globalHookProcess64_);
  StopGlobalHookProcess(globalHookProcess32_);
}

void GlobalHook::CleanupOldHooks()
{
  const auto pID32 = GetProcessIDFromName(ConvertUTF16StringToUTF8String(g_globalHookProcess32));
  const auto pID64 = GetProcessIDFromName(ConvertUTF16StringToUTF8String(g_globalHookProcess64));
  // Nothing ot cleanup when processes are not found
  if (pID32 == 0 && pID64 == 0) {
    return;
  }
  else {
    HANDLE threadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (threadSnapshot == INVALID_HANDLE_VALUE) {
      g_messageLog.LogWarning("GlobalHook", "Create thread snapshot failed",
        GetLastError());
      return;
    }
    else {
      THREADENTRY32 thread{};
      thread.dwSize = sizeof(thread);
      if (Thread32First(threadSnapshot, &thread)) {
        do {
          if (thread.dwSize >=
            FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(thread.th32OwnerProcessID)) {
            if (thread.th32ThreadID != 0 &&
              (thread.th32OwnerProcessID == pID32 || thread.th32OwnerProcessID == pID64)) {
              SendQuitMessage(thread.th32ThreadID);
            }
          }
        } while (Thread32Next(threadSnapshot, &thread));
      }
    }
  }
}

bool GlobalHook::StartGlobalHookProcess(PROCESS_INFORMATION& procInfo, const std::wstring& hookExe)
{
  STARTUPINFO startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  if (!CreateProcess(hookExe.c_str(), NULL, NULL, NULL, TRUE,
    NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startupInfo,
    &procInfo)) {
    g_messageLog.LogError("GlobalHook", " Creating global hook process failed ",
      GetLastError());
    procInfo = {};
    return false;
  }
  return true;
}

void GlobalHook::StopGlobalHookProcess(PROCESS_INFORMATION& procInfo)
{
  if (procInfo.hProcess && procInfo.hThread) {
    SendQuitMessage(procInfo.dwThreadId);
    CloseHandle(procInfo.hThread);
    CloseHandle(procInfo.hProcess);
    procInfo = {};
  }
}

void GlobalHook::SendQuitMessage(DWORD threadID)
{
  if (!PostThreadMessage(threadID, WM_QUIT, 0, 0)) {
    g_messageLog.LogError("GlobalHook", " Stopping global hook process failed",
      GetLastError());
  }
}
