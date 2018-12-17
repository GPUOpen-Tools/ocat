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

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>

// Searches for process with name in all current processes
// Returns 0 if nothing was found
DWORD GetProcessIDFromName(const std::string& name);
HANDLE GetProcessHandleFromID(DWORD id, DWORD access);

HWND GetWindowHandleFromProcessID(DWORD id);

// Returns <unknown> if name of process for this id could not be retrieved or
// unable to access because system process
std::wstring GetProcessNameFromID(DWORD id);
std::wstring GetProcessNameFromHandle(HANDLE handle);

std::wstring GetCurrentProcessDirectory();
std::wstring GetAbsolutePath(const std::wstring& relativePath);

enum class ProcessArchitecture { x86, x64, undefined };

ProcessArchitecture GetProcessArchitecture(DWORD processID);

std::wstring GetWindowTitle(HWND window);
std::wstring GetWindowClassName(HWND window);

std::string GetSystemErrorMessage(DWORD errorCode);
std::wstring GetSystemErrorMessageW(DWORD errorCode);

HWND FindOcatWindowHandle();

LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue);

std::vector<std::wstring> GetLoadedModuleNames();
