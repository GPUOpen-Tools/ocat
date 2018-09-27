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

#include "DLLInjection.h"

#include "../Utility/Constants.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "OverlayMessage.h"

#include <vector>

namespace {

HANDLE GetProcessHandle(DWORD processID)
{
  const auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
  if (!processHandle) {
    g_messageLog.LogError("DLL Injection",
                     " Unable to open process to query architecture");
    return NULL;
  }
  return processHandle;
}

bool StartDLLInjector(std::wstring& commandLine)
{
  g_messageLog.LogVerbose("StartDLLInjector", L"With commandline: " + commandLine);

  STARTUPINFO startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  PROCESS_INFORMATION processInfo{};
  if (!CreateProcess(NULL, &commandLine[0], NULL, NULL, TRUE,
                     NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startupInfo,
                     &processInfo)) {
    g_messageLog.LogError("DLL Injection", L"CreateProcess failed " + commandLine,
                     GetLastError());
    return false;
  }

  WaitForSingleObject(processInfo.hProcess, INFINITE);
  CloseHandle(processInfo.hThread);
  CloseHandle(processInfo.hProcess);

  g_messageLog.LogInfo("StartDLLInjector", L"Successfully injected " + commandLine);
  return true;
}

const std::wstring GetDLLPath(const std::wstring& dllDirectory,
                              ProcessArchitecture processArchitecture)
{
  const auto& libName =
      (processArchitecture == ProcessArchitecture::x86) ? g_libraryName32 : g_libraryName64;

  const std::wstring dllPath =
      (dllDirectory.empty()) ? GetAbsolutePath(libName) : dllDirectory + libName;
  return dllPath;
}

const std::wstring GetCommandLineArgs(const std::wstring& dllPath, const std::wstring& dllDirectory,
                                      DWORD processID, ProcessArchitecture processArchitecture,
                                      bool free)
{
  std::wstring commandArgs = L" -l \"" + dllPath + L"\" -p " + std::to_wstring(processID);
  if (free) {
    commandArgs += L" -free";
  }

  std::wstring commandLine =
      L"\"" + dllDirectory +
      ((processArchitecture == ProcessArchitecture::x86) ? g_injectorProcess32
                                                         : g_injectorProcess64) +
      L"\"" + commandArgs;
  return commandLine;
}
}

bool InjectDLL(DWORD processID, const std::wstring& dllDirectory)
{
  g_messageLog.LogInfo("DLL Injection",
                   " start injection " + std::to_string(processID));
  auto processArchitecture = GetProcessArchitecture(processID);
  if (processArchitecture == ProcessArchitecture::undefined) {
    return false;
  }

  const auto dllPath = GetDLLPath(dllDirectory, processArchitecture);
  auto commandLine =
      GetCommandLineArgs(dllPath, dllDirectory, processID, processArchitecture, false);

  return StartDLLInjector(commandLine);
}

bool FreeDLL(DWORD processID, const std::wstring& dllDirectory)
{
  printf("Free dll %d\n", processID);
  g_messageLog.LogInfo("DLL Injection",
                   " free dll in " + std::to_string(processID));
  auto processArchitecture = GetProcessArchitecture(processID);
  if (processArchitecture == ProcessArchitecture::undefined) {
    return false;
  }

  const auto dllPath = GetDLLPath(dllDirectory, processArchitecture);
  auto commandLine =
      GetCommandLineArgs(dllPath, dllDirectory, processID, processArchitecture, true);

  return StartDLLInjector(commandLine);
  return true;
}

bool FreeDLLOverlay(DWORD threadID)
{
  return OverlayMessage::PostOverlayMessage(threadID, OverlayMessageType::FreeLibrary, 0);
}
