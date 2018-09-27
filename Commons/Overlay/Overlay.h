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

#include <windows.h>
#include <string>
#include "Hook.h"
#include "../Utility/ProcessTermination.h"

class Overlay {
public:
  Overlay();

  // Starts the given executable and suspends the process to load the overlay dll
  // if failed the process will be resumed without overlay
  bool StartProcess(const std::wstring& path, std::wstring& cmdArgs, bool enableOverlay);
  void Stop();

  // returns the process name, empty if no process is attached
  const std::wstring& GetProcessName() const { return processName_; }
  const DWORD GetProcessID() const;

private:
  // Returns ERROR_SUCCES if a normal process was started, ERROR_APPCONTAINER_REQUIRED if tried to
  // start uwp app
  DWORD CreateDesktopProcess(const std::wstring& path, std::wstring& cmdArgs, bool enableOverlay);
  void SetProcessInfo(DWORD id);

  PROCESS_INFORMATION processInfo_{};
  DWORD processID_ = 0;
  std::wstring processName_;
  Hook hook_;
  ProcessTermination processTermination_;
};
