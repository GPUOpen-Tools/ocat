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
// The above copyright notice and this permission notice shall be included in
// all
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

#include "ProcessHelper.h"
#include "../Logging/MessageLog.h"
#include "../Utility/StringUtils.h"
#include "../Utility/SmartHandle.h"

#include <psapi.h>
#include <assert.h>
#include <vector>

DWORD GetProcessIDFromName(const std::string& name)
{
  std::vector<DWORD> processes(1024);
  DWORD processArraySize = 0;

  const DWORD bufferSize = static_cast<DWORD>(processes.size() * sizeof(DWORD));

  if (!EnumProcesses(processes.data(), bufferSize, &processArraySize))
  {
    g_messageLog.LogError("ProcessHelper",
      "GetProcessIDFromName - Error while enumerating processes ",
      GetLastError());
    return 0;
  }

  const auto processName = ConvertUTF8StringToUTF16String(name);
  for (const auto pID : processes)
  {
    if (pID != 0 && (GetProcessNameFromID(pID).compare(processName) == 0))
    {
      g_messageLog.LogInfo("ProcessHelper",
        L"GetProcessIDFromName - Found process " + processName + L"PID" + std::to_wstring(pID));
      return pID;
    }
  }

  return 0; // Process not found --> Let caller handle that case.
}

HANDLE GetProcessHandleFromID(DWORD id, DWORD access)
{
  HANDLE handle = OpenProcess(access, FALSE, id);
  if (!handle)
  {
    g_messageLog.LogError("ProcessHelper",
      "GetProcessHandleFromID - Failed getting process hProcess of id " + std::to_string(id),
      GetLastError());
    return NULL;
  }
  return handle;
}

HWND g_hwnd = NULL;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  DWORD processID;
  GetWindowThreadProcessId(hwnd, &processID);
  if (processID == lParam)
  {
    g_hwnd = hwnd;
    return false;
  }
  return true;
}

HWND GetWindowHandleFromProcessID(DWORD id)
{
  g_hwnd = NULL;
  EnumWindows(EnumWindowsProc, id);
  return g_hwnd;
}

std::wstring GetProcessNameFromID(DWORD id)
{
  const HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, id);
  if (processHandle)
  {
    return GetProcessNameFromHandle(processHandle);
  }
  else
  {
    auto error = GetLastError();
    if (error != ERROR_ACCESS_DENIED)
    {
      g_messageLog.LogWarning("ProcessHelper",
        "GetProcessNameFromID - Failed opening process with id " + std::to_string(id), error);
    }
    return L"";
  }
}

std::wstring GetProcessNameFromHandle(HANDLE handle)
{
  std::wstring buffer;
  HMODULE module;
  DWORD moduleArraySize = 0;
  DWORD nameLength = 0;
  DWORD bufferSize = 1024;
  if (EnumProcessModules(handle, &module, sizeof(module), &moduleArraySize))
  {
    do
    {
      assert(bufferSize <= (std::numeric_limits<DWORD>::max)());
      buffer.resize(bufferSize);
      nameLength = GetModuleBaseName(handle, module, &buffer[0], static_cast<DWORD>(buffer.size()));
      if (!nameLength)
      {
        g_messageLog.LogWarning("ProcessHelper",
          "GetAbsolutePath - Unable to get name.", GetLastError());
        return L"";
      }
      bufferSize *= 2;
    }
    // if same size, name is likely truncated, double buffer size
    while (nameLength == buffer.size());
  }
  else
  {
    g_messageLog.LogWarning("ProcessHelper",
      "GetAbsolutePath - Unable to enumerate process hModules.", GetLastError());
  }
  return buffer.substr(0, nameLength);
}

std::wstring GetCurrentProcessDirectory()
{
  const auto bufferLength = GetCurrentDirectory(0, NULL);
  std::wstring buffer;
  buffer.resize(bufferLength);
  if (GetCurrentDirectory(bufferLength, &buffer[0]))
  {
    // remove terminating null
    return buffer.substr(0, buffer.size() - 1) + L"\\";
  }
  return L"";
}

std::wstring GetAbsolutePath(const std::wstring& relativePath)
{
  size_t bufferSize = 1024;
  std::wstring absolutePath;
  TCHAR* filePart = nullptr;

  DWORD pathLength = 0;
  do
  {
    assert(bufferSize <= (std::numeric_limits<DWORD>::max)());
    absolutePath.resize(bufferSize);
    pathLength = GetFullPathName(relativePath.c_str(), static_cast<DWORD>(absolutePath.size()),
      &absolutePath[0], &filePart);
    if (!pathLength) {
      g_messageLog.LogWarning("ProcessHelper",
        "GetAbsolutePath - GetFullPathName failed.", GetLastError());
      return L"";
    }
    bufferSize = pathLength;
  }
  // if buffer is too small the returned path length is the required size
  // including terminating null character
  while (pathLength > absolutePath.size());

  // if successfull the pathLength is the length without the terminating null
  // character
  absolutePath.resize(pathLength);
  return absolutePath;
}

ProcessArchitecture GetProcessArchitecture(DWORD processID)
{
  const auto wow64FunctionAdress = GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
  if (!wow64FunctionAdress)
  {
    g_messageLog.LogError("ProcessHelper",
      "GetProcessArchitecture - IsWow64Process function not found in kernel32.");
    return ProcessArchitecture::undefined;
  }

  BOOL wow64Process = true;
  Win32Handle processHandle = GetProcessHandleFromID(processID, PROCESS_QUERY_INFORMATION);
  if (!processHandle.Get())
  {
    return ProcessArchitecture::undefined;
  }
  if (!IsWow64Process(processHandle.Get(), &wow64Process))
  {
    g_messageLog.LogError("ProcessHelper",
      "GetProcessArchitecture - IsWow64Process failed.",
      GetLastError());
    return ProcessArchitecture::undefined;
  }

  return wow64Process == TRUE ? ProcessArchitecture::x86 : ProcessArchitecture::x64;
}

std::wstring GetWindowTitle(HWND window)
{
  std::wstring windowTitle;
  int bufferSize = 1024;
  DWORD titleLength = 0;
  do
  {
    if (bufferSize <= (std::numeric_limits<int>::max)())
    {
      windowTitle.resize(bufferSize);
      titleLength = GetWindowText(window, &windowTitle[0], bufferSize);
      if (!titleLength) {
        g_messageLog.LogError("ProcessHelper",
          "GetWindowTitle - GetWindowText failed.", GetLastError());
        return L"";
      }
      bufferSize *= 2;
    }
  } while (titleLength == windowTitle.size());
  windowTitle.resize(titleLength);
  return windowTitle;
}

std::wstring GetWindowClassName(HWND window)
{
  std::wstring windowClassName;
  int bufferSize = 1024;
  DWORD classLength = 0;
  do
  {
    if (bufferSize <= (std::numeric_limits<int>::max)())
    {
      windowClassName.resize(bufferSize);
      classLength = GetClassName(window, &windowClassName[0], bufferSize);
      if (!classLength)
      {
        g_messageLog.LogError("ProcessHelper",
          "GetWindowClassName - GetClassName failed.", GetLastError());
        return L"";
      }
      bufferSize *= 2;
    }
  } while (classLength == windowClassName.size());
  windowClassName.resize(classLength);
  return windowClassName;
}

std::wstring GetSystemErrorMessageW(DWORD errorCode)
{
  // Get the corresponding system error code as string.
  LPWSTR errorMessageBuffer = nullptr;
  size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessageBuffer, 0, NULL);
  std::wstring systemErrorMessage(errorMessageBuffer, size);
  LocalFree(errorMessageBuffer); //Free the buffer.

  if (systemErrorMessage.size() > 2) // Avoid unsigned overflow.
  {
    systemErrorMessage.resize(systemErrorMessage.size() - 2); // Last chars contain new lines.
  }
  return systemErrorMessage;
}

std::string GetSystemErrorMessage(DWORD errorCode)
{
  return ConvertUTF16StringToUTF8String(GetSystemErrorMessageW(errorCode));
}


HWND FindOcatWindowHandle()
{
  const auto windowHandle = FindWindow(NULL, L"OCAT");
  if (!windowHandle)
  {
    // In case of getting this error message: Did the title of the OCAT window change?
    g_messageLog.LogError("ProcessHelper",
      "Could not find OCAT window hProcess.", GetLastError());
    return NULL;
  }
  return windowHandle;
}

// https://stackoverflow.com/a/35717
LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
  strValue = strDefaultValue;
  WCHAR szBuffer[512];
  DWORD dwBufferSize = sizeof(szBuffer);
  ULONG nError;
  nError = RegQueryValueEx(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
  if (ERROR_SUCCESS == nError)
  {
    strValue = szBuffer;
  }
  return nError;
}

// https://msdn.microsoft.com/en-us/library/ms682621(VS.85).aspx
std::vector<std::wstring> GetLoadedModuleNames()
{
  std::vector<std::wstring> moduleNames;
  HMODULE hModules[1024];
  DWORD cbNeeded;
  Win32Handle hProcess = GetCurrentProcess();
  if (EnumProcessModules(hProcess.Get(), hModules, sizeof(hModules), &cbNeeded))
  {
    for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
    {
      TCHAR szModName[MAX_PATH];
      // Get the full path to the module's file.
      if (GetModuleFileNameEx(hProcess.Get(), hModules[i], szModName,
        sizeof(szModName) / sizeof(TCHAR)))
      {
        moduleNames.push_back(szModName);
      }
    }
  }
  return moduleNames;
}
