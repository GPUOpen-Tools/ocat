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

#include "OverlayThread.h"
#include "..\Overlay\OverlayMessage.h"
#include "..\Utility\Constants.h"
#include "..\Logging\MessageLog.h"
#include "RecordingState.h"

namespace GameOverlay {

HWND g_windowHandle = NULL;

OverlayThread::OverlayThread() {}
OverlayThread::~OverlayThread() { Stop(); }
void OverlayThread::Stop()
{
  HANDLE thread = reinterpret_cast<HANDLE>(overlayThread_.native_handle());
  if (thread) {
    const auto threadID = GetThreadId(thread);
    if (threadID) {
      PostThreadMessage(threadID, WM_QUIT, 0, 0);
      if (overlayThread_.joinable()) {
        overlayThread_.join();
      }
    }
  }
}

void OverlayThread::Start()
{
  g_messageLog.Log(MessageLog::LOG_INFO, "OverlayThread", "Start overlay thread ");
  overlayThread_ = std::thread(ThreadProc);
}

void OverlayThread::ThreadProc()
{
  if (!ThreadStartup(g_windowHandle)) {
    return;
  }

  OverlayMessage::PostFrontendMessage(g_windowHandle, OVERLAY_Initialized, GetCurrentProcessId());

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (OverlayMessage::overlayMessageType == msg.message)
    {
      switch (msg.wParam)
      {
      case OVERLAY_FreeLibrary:
        DisableOverlay();
        break;
      case OVERLAY_StartRecording:
        RecordingState::GetInstance().Start();
        break;
      case OVERLAY_StopRecording:
        RecordingState::GetInstance().Stop();
        break;
      default:
        break;
      }
    }
  }

  ThreadCleanup(g_windowHandle);
}

void OverlayThread::DisableOverlay()
{
  ThreadCleanup(g_windowHandle);

  HMODULE dll = NULL;
#if _WIN64
  auto& dllName = g_libraryName64;
#else
  auto& dllName = g_libraryName32;
#endif
  if (!GetModuleHandleEx(0, dllName.c_str(), &dll)) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayThread", "GetModuleHandleEx failed ",
                     GetLastError());
    return;
  }

  RecordingState::GetInstance().StopOverlay();

  FreeLibraryAndExitThread(dll, 0);
}

bool OverlayThread::ThreadStartup(HWND& windowHandle)
{
  windowHandle = FindWindowHandle();
  if (!windowHandle) {
    return false;
  }

  return OverlayMessage::PostFrontendMessage(windowHandle, OVERLAY_ThreadInitialized, GetCurrentThreadId());
}

bool OverlayThread::ThreadCleanup(HWND windowHandle)
{
  return OverlayMessage::PostFrontendMessage(windowHandle, OVERLAY_ThreadTerminating, GetCurrentThreadId());
}

HWND OverlayThread::FindWindowHandle()
{
  const auto windowHandle = FindWindow(NULL, L"OCAT Configuration");
  if (!windowHandle) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayThread", "GetWindow handle failed",
                     GetLastError());
    return NULL;
  }
  return windowHandle;
}
}