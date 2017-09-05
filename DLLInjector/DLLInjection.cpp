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

#include <Windows.h>
#include <string>
#include <TlHelp32.h>

#include "Logging\MessageLog.h"
#include "Utility\ProcessHelper.h"
#include "Utility\StringUtils.h"

DLLInjection::Arguments::Arguments() : processID{0} {}
DLLInjection::Resources::Resources() : processHandle{NULL}, remoteDLLAddress{nullptr} {}
DLLInjection::Resources::~Resources()
{
  if (processHandle) {
    if (remoteDLLAddress) {
      VirtualFreeEx(processHandle, remoteDLLAddress, 0, MEM_RELEASE);
      remoteDLLAddress = nullptr;
    }
    CloseHandle(processHandle);
    processHandle = NULL;
  }
}

// #include <windows.h>
// #include <stdio.h>
// #include <tchar.h>
#include <psapi.h>

void DLLInjection::UpdateProcessName()
{
  auto processName = GetProcessNameFromHandle(resources_.processHandle);
  if (processName.size() > 0) {
    processName_ = ConvertUTF16StringToUTF8String(processName);
    return;
  }

  // Default to unknown.
  processName_ = "<unknown>";
}

std::string DLLInjection::GetProcessInfo()
{
  return std::to_string(arguments_.processID)
    + " (" + processName_ + ")";
}

DLLInjection::DLLInjection(const Arguments & args) : arguments_(args)
{
  // Empty
}

bool DLLInjection::InjectDLL()
{
  if (arguments_.dllPath.empty()) {
    g_messageLog.LogError("DLLInjector", 
      "Empty dll path");
    return false;
  }

  g_messageLog.LogInfo("DLLInjector",
    "Starting dll injection for " 
    + std::to_string(arguments_.processID));

  if (!GetProcessHandle()) {
    return false;
  }

  if (!GetRemoteDLLAddress()) {
    return false;
  }

  if (!ExecuteLoadLibrary()) {
    return false;
  }

  g_messageLog.LogInfo("DLLInjector",
                   "DLL injected into process " + GetProcessInfo());
  return true;
}

bool DLLInjection::FreeDLL()
{
  g_messageLog.LogInfo("DLLInjector",
    "Starting free dll for " + std::to_string(arguments_.processID));

  if (!GetProcessHandle()) {
    return false;
  }

  void* dllModule = GetRemoteDLLModule();
  if (!dllModule) {
    return false;
  }

  if (!ExecuteFreeLibrary(dllModule)) {
    return false;
  }

  g_messageLog.LogInfo("DLLInjector",
    "DLL freed in process " + GetProcessInfo());

  return true;
}

bool DLLInjection::GetProcessHandle()
{
  auto processHandle = OpenProcess(
    PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION 
    | PROCESS_VM_WRITE | PROCESS_VM_READ,
    FALSE, arguments_.processID);
  if (!processHandle) {
    g_messageLog.LogError("DLLInjector",
        "Unable to open process " + std::to_string(arguments_.processID) 
      + " for injection ",
        GetLastError());
    return false;
  }

  resources_.processHandle = processHandle;
  UpdateProcessName();

  g_messageLog.LogInfo("DLLInjector",
    "Acquired process handle for "
    + GetProcessInfo());

  return true;
}

bool DLLInjection::GetRemoteDLLAddress()
{
  // Because libAbsolutePath is used as c-string representation add size for the null terminator
  dllPathSize_ = (arguments_.dllPath.size() + 1) * sizeof(arguments_.dllPath[0]);
  void* remoteDLLAddress =
      VirtualAllocEx(resources_.processHandle, NULL, dllPathSize_, MEM_COMMIT, PAGE_READWRITE);
  if (!remoteDLLAddress) {
    g_messageLog.LogError("DLLInjector", 
      "Unable to allocate memory for dll in " + GetProcessInfo(),
      GetLastError());
    return false;
  }

  g_messageLog.LogInfo("DLLInjector",
    "Acquired remote DLL address for "
    + GetProcessInfo());

  resources_.remoteDLLAddress = remoteDLLAddress;
  return true;
}

bool DLLInjection::ExecuteLoadLibrary()
{
  if (!WriteProcessMemory(resources_.processHandle, resources_.remoteDLLAddress,
                          arguments_.dllPath.data(), dllPathSize_, NULL)) {
    g_messageLog.LogError("DLLInjector", 
      "WriteProcessMemory failed for " + GetProcessInfo(),
      GetLastError());
    return false;
  }
  return ExecuteRemoteThread("LoadLibraryW", resources_.remoteDLLAddress);
}

void* DLLInjection::GetRemoteDLLModule()
{
  bool result = false;
  void* dllModule = nullptr;
  auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, arguments_.processID);
  if (snapShot)
  {
    MODULEENTRY32 moduleEntry{};
    moduleEntry.dwSize = sizeof(moduleEntry);
    auto newModule = Module32First(snapShot, &moduleEntry);
    while (newModule)
    {
      if (arguments_.dllName.compare(moduleEntry.szModule) == 0)
      {
        dllModule = moduleEntry.modBaseAddr;
        g_messageLog.LogInfo("DLLInjector",
          "Found remote DLL module for "
          + GetProcessInfo());

        result = true;
        break;
      }

      newModule = Module32Next(snapShot, &moduleEntry);
    }
    if (!newModule)
    {
      g_messageLog.LogError("DLLInjector", 
        "Module not found for " + GetProcessInfo(), GetLastError());
      result = false;
    }
  }
  else
  {
    g_messageLog.LogError("DLLInjector", 
      "CreateToolhelp32Snapshot failed for " + GetProcessInfo(),
      GetLastError());
    result = false;
  }

  CloseHandle(snapShot);
  return dllModule;
}

bool DLLInjection::ExecuteFreeLibrary(void* dllModule)
{
  return ExecuteRemoteThread("FreeLibrary", dllModule);
}

bool DLLInjection::ExecuteRemoteThread(const std::string& functionName, void* functionArguments)
{
  const auto threadRoutine = reinterpret_cast<PTHREAD_START_ROUTINE>(
    GetProcAddress(GetModuleHandle(TEXT("Kernel32")), functionName.c_str()));
  if (!threadRoutine) {
    g_messageLog.LogError("DLLInjector",
      "GetProcAddress failed for " + functionName);
    return false;
  }

  HANDLE remoteThread = CreateRemoteThread(resources_.processHandle, NULL, 0, threadRoutine,
    functionArguments, 0, NULL);
  if (!remoteThread) {
    g_messageLog.LogError("DLLInjector", "CreateRemoteThread failed ",
      GetLastError());
    return false;
  }

  WaitForSingleObject(remoteThread, INFINITE);
  DWORD exitCode = 0;
  if (!GetExitCodeThread(remoteThread, &exitCode))
  {
    g_messageLog.LogError("DLLInjector", "GetExitCodeThread failed ",
      GetLastError());
  }
  if(!exitCode)
  {
    g_messageLog.LogError("DLLInjector", "Remote thread failed");
  }
  CloseHandle(remoteThread);
  return true;
}