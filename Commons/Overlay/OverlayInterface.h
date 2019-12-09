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

#include "Overlay.h"
#include "GlobalHook.h"

#include <vector>

class OverlayInterface
{
public:
  OverlayInterface();
  ~OverlayInterface();

  bool Init(HWND hwnd);

  void StartProcess(const std::wstring& executable, std::wstring& cmdArgs);
  void StartGlobal();
  void StopCapture(std::vector<int> overlayThreads);
  void FreeInjectedDlls(std::vector<int> injectedProcesses);
  bool ProcessFinished();

private:
  bool SetMessageFilter();

  static const char enableOverlayEnv_[];
  Overlay overlay_;
  GlobalHook globalHook_;
  ProcessTermination processTermination_;
};