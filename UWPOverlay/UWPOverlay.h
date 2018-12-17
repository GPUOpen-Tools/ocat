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

#pragma once

#include <ShObjIdl.h>
#include <windows.h>
#include <atlcomcli.h>
#include <string>

class UWPOverlay {
 public:
  // Starts the uwp executable suspended by attaching a debugger which injects the GameOverlay dll
  // Returns the process ID or zero if unsuccessfull
  static unsigned long StartProcess(const wchar_t* path);

 private:
  // Creates a application activation manager and starts the executable
  // Returns the process ID or zero if unsuccessfull
  static DWORD StartUWPProcess(const std::wstring& appId);

  // Attaches a debug exe to the uwp process and starts it in a suspended state
  // The executable is responsible for injecting the GameOverlay dll and resuming the uwp process
  static bool EnableDebugging(const std::wstring& packageID, IPackageDebugSettings* debugSettings);
  // Removes the debugg exe from the uwp process
  static void DisableDebugging(const std::wstring& packageID, IPackageDebugSettings* debugSettings);

  // UWP helper functions
  static std::wstring GetPackageIdFromPath(const std::wstring& input);
  static std::wstring GetAppIdFromPackageId(const std::wstring& packageId);

  static const std::wstring uwpDebugExe_;
};

extern "C" __declspec(dllexport) DWORD StartUWPProcessWithOverlay(const wchar_t* path)
{
  return UWPOverlay::StartProcess(path);
}