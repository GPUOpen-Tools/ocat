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
#include "MessageLog.h"

#include <Psapi.h>
#include <assert.h>
#include <codecvt>
#include <vector>

std::wstring ConvertUTF8StringToUTF16String(const std::string& input)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring result = converter.from_bytes(input);
  return result;
}

std::string ConvertUTF16StringToUTF8String(const std::wstring& input)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::string result = converter.to_bytes(input);
  return result;
}

DWORD GetProcessIDFromName(const std::string& name)
{
  std::vector<DWORD> processes(1024);
  DWORD processArraySize = 0;

  const DWORD bufferSize = static_cast<DWORD>(processes.size() * sizeof(DWORD));

  if (!EnumProcesses(processes.data(), bufferSize, &processArraySize)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "GetProcessIDFromName", "Enum Process error ",
                     GetLastError());
    return 0;
  }

  const auto processName = ConvertUTF8StringToUTF16String(name);
  for (const auto pID : processes) {
    if (pID != 0 && (GetProcessNameFromID(pID).compare(processName) == 0)) {
      wprintf(L"Found process %s PID %u\n", processName.c_str(), pID);
      return pID;
    }
  }

  g_messageLog.Log(MessageLog::LOG_ERROR, "GetProcessIDFromName", "Process not found");
  return 0;
}

HANDLE GetProcessHandleFromID(DWORD id, DWORD access)
{
  HANDLE handle = OpenProcess(access, FALSE, id);
  if (!handle) {
    printf("Failed getting process handle %ld", GetLastError());
  }
  return handle;
}

HWND g_hwnd = NULL;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  DWORD processID;
  GetWindowThreadProcessId(hwnd, &processID);
  if (processID == lParam) {
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
  if (processHandle) {
    return GetProcessNameFromHandle(processHandle);
  }
  else {
    printf("OpenProcess failed %lu\n", GetLastError());
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
  if (EnumProcessModules(handle, &module, sizeof(module), &moduleArraySize)) {
    do {
      assert(bufferSize <= (std::numeric_limits<DWORD>::max)());
      buffer.resize(bufferSize);
      nameLength = GetModuleBaseName(handle, module, &buffer[0], static_cast<DWORD>(buffer.size()));
      if (!nameLength) {
        printf("Unable to get name %lu", GetLastError());
        return L"";
      }
      bufferSize *= 2;
    }
    // if same size, name is likely truncated, double buffer size
    while (nameLength == buffer.size());
  }
  else {
    printf("Unable to Enum Process Modules %lu\n", GetLastError());
  }

  return buffer.substr(0, nameLength);
}

std::wstring GetDirFromPathSlashes(const std::wstring& path)
{
  const auto pathEnd = path.find_last_of('\\');
  if (pathEnd == std::string::npos) {
    printf("Failed finding end of path\n");
    return L"";
  }

  return path.substr(0, pathEnd + 1);
}

std::wstring GetDirFomPathSlashesRemoved(const std::wstring& path)
{
  const size_t directoryEnd = path.find_last_of('\\');
  std::wstring directory;
  if (std::string::npos != directoryEnd) {
    directory = path.substr(0, directoryEnd);
  }
  else {
    MessageBox(NULL, L"Invalid file path ", NULL, MB_OK);
  }
  return directory;
}

std::wstring GetCurrentProcessDirectory()
{
  const auto bufferLength = GetCurrentDirectory(0, NULL);

  std::wstring buffer;
  buffer.resize(bufferLength);
  if (GetCurrentDirectory(bufferLength, &buffer[0])) {
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
  do {
    assert(bufferSize <= (std::numeric_limits<DWORD>::max)());
    absolutePath.resize(bufferSize);
    pathLength = GetFullPathName(relativePath.c_str(), static_cast<DWORD>(absolutePath.size()),
                                 &absolutePath[0], &filePart);
    if (!pathLength) {
      printf("GetFullPathName failed %lu\n", GetLastError());
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
  if (!wow64FunctionAdress) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "Check Process Architecture",
                     "IsWow64Process function not found in kernel32");
    return ProcessArchitecture::undefined;
  }

  BOOL wow64Process = true;
  const auto processHandle = GetProcessHandleFromID(processID, PROCESS_QUERY_INFORMATION);
  if (!processHandle) {
    return ProcessArchitecture::undefined;
  }
  if (!IsWow64Process(processHandle, &wow64Process)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "Get Process Architecture", "IsWow64Process failed",
                     GetLastError());
    CloseHandle(processHandle);
    return ProcessArchitecture::undefined;
  }
  CloseHandle(processHandle);

  return wow64Process == TRUE ? ProcessArchitecture::x86 : ProcessArchitecture::x64;
}

std::wstring GetWindowTitle(HWND window)
{
  std::wstring windowTitle;
  int bufferSize = 1024;
  DWORD titleLength = 0;
  do {
    if (bufferSize <= (std::numeric_limits<int>::max)()) {
      windowTitle.resize(bufferSize);
      titleLength = GetWindowText(window, &windowTitle[0], bufferSize);
      if (!titleLength) {
        g_messageLog.Log(MessageLog::LOG_ERROR, "GetWindowTitle", "GetWindowText failed",
                         GetLastError());
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
  do {
    if (bufferSize <= (std::numeric_limits<int>::max)()) {
      windowClassName.resize(bufferSize);
      classLength = GetClassName(window, &windowClassName[0], bufferSize);
      if (!classLength) {
        g_messageLog.Log(MessageLog::LOG_ERROR, "GetWindowClassName", "GetClassName failed",
                         GetLastError());
        return L"";
      }
      bufferSize *= 2;
    }
  } while (classLength == windowClassName.size());
  windowClassName.resize(classLength);
  return windowClassName;
}

std::string GetSystemErrorMessage(DWORD errorCode)
{
  // Get the corresponding system error code as string.
  LPSTR errorMessageBuffer = nullptr;
  size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessageBuffer, 0, NULL);
  std::string systemErrorMessage(errorMessageBuffer, size);
  LocalFree(errorMessageBuffer); //Free the buffer.

  if (systemErrorMessage.size() > 2) {// Avoid unsigned overflow.
    systemErrorMessage.resize(systemErrorMessage.size() - 2); // Last chars contain new lines.
  }
  return systemErrorMessage;
}

std::wstring GetSystemErrorMessageW(DWORD errorCode)
{
  return ConvertUTF8StringToUTF16String(GetSystemErrorMessage(errorCode));
}

#define VERBOSE_DEBUG

void OutputDebug(const std::wstring & message, DWORD errorCode)
{
  UNREFERENCED_PARAMETER(message);
#ifdef VERBOSE_DEBUG
  std::wstring output = L"OCAT: " + message;
  if (errorCode != 0) {
    output += L" - Error: " + GetSystemErrorMessageW(errorCode);
  }

  // Brute force solution for debugging.
  const std::wstring processName = GetProcessNameFromHandle(GetCurrentProcess());
  OutputDebugString((output +  L" - Process: " + processName).c_str());
#endif
}

void OutputDebug(const std::string & message, DWORD errorCode)
{
  OutputDebug(ConvertUTF8StringToUTF16String(message), errorCode);
}