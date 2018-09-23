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

#include "OverlayInterface.h"
#include "../Logging/MessageLog.h"
#include "../Overlay/DLLInjection.h"
#include "../Utility/FileDirectory.h"
#include "OverlayMessage.h"

bool g_ProcessFinished;
bool g_CaptureAll;
HWND g_FrontendHwnd;


OverlayInterface::OverlayInterface()
{
  // Nothing to do.
}

OverlayInterface::~OverlayInterface()
{
  globalHook_.Deactivate();
  globalHook_.CleanupOldHooks();
  processTermination_.UnRegister();
}

bool OverlayInterface::Init(HWND hwnd)
{
  if (!g_fileDirectory.Initialize())
  {
    return false;
  }

  g_FrontendHwnd = hwnd;
  globalHook_.CleanupOldHooks();
  SetMessageFilter();
  return true;
}

void OverlayInterface::StartProcess(const std::wstring& executable, std::wstring& cmdArgs)
{
  g_messageLog.LogInfo("OverlayInterface", "Start single process");
  g_ProcessFinished = false;
  g_CaptureAll = false;
  if (overlay_.StartProcess(executable, cmdArgs, true))
  {
    processTermination_.Register(overlay_.GetProcessID());
  }
}

void OverlayInterface::StartGlobal()
{
  g_messageLog.LogInfo("OverlayInterface", "Start global hook");
  g_ProcessFinished = false;
  g_CaptureAll = true;
  globalHook_.Activate();
}

void OverlayInterface::StopCapture(std::vector<int> overlayThreads)
{
  overlay_.Stop();
  processTermination_.UnRegister();
  globalHook_.Deactivate();

  for (int threadID : overlayThreads) {
    FreeDLLOverlay(threadID);
  }

  g_ProcessFinished = true;
}

void OverlayInterface::FreeInjectedDlls(std::vector<int> injectedProcesses)
{
  for (int processID : injectedProcesses) {
    FreeDLL(processID, g_fileDirectory.GetDirectory(DirectoryType::Bin));
  }
}

void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
  // Only interested in finished process if a specific process is captured.
  g_ProcessFinished = !g_CaptureAll;
  // Send message to frontend to trigger a UI update.
  OverlayMessage::PostFrontendMessage(g_FrontendHwnd, OverlayMessageType::ThreadTerminating, 0);
}

bool OverlayInterface::ProcessFinished()
{
  return g_ProcessFinished;
}

bool OverlayInterface::SetMessageFilter()
{
  CHANGEFILTERSTRUCT changeFilter;
  changeFilter.cbSize = sizeof(CHANGEFILTERSTRUCT);
  if (!ChangeWindowMessageFilterEx(g_FrontendHwnd, OverlayMessage::overlayMessageType, MSGFLT_ALLOW, &changeFilter))
  {
    g_messageLog.LogError("OverlayInterface",
      "Changing window message filter failed", GetLastError());
    return false;
  }
  return true;
}
