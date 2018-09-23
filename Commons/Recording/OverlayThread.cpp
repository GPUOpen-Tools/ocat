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
#include "../Overlay/OverlayMessage.h"
#include "../Utility/Constants.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "RecordingState.h"

namespace GameOverlay {

HWND g_windowHandle = NULL;

OverlayThread::~OverlayThread() 
{ 
  Stop(); 
}

void OverlayThread::Stop()
{
  HANDLE thread = reinterpret_cast<HANDLE>(overlayThread_.native_handle());
  if (thread) 
  {
    const auto threadID = GetThreadId(thread);
    if (threadID) 
    {
      PostThreadMessage(threadID, WM_QUIT, 0, 0);
      if (overlayThread_.joinable()) 
      {
        overlayThread_.join();
      }
    }
  }
}

void OverlayThread::Start()
{
  g_messageLog.LogInfo("OverlayThread", "Start overlay thread ");
  overlayThread_ = std::thread(ThreadProc);
}

void OverlayThread::ThreadProc()
{
  if (!ThreadStartup(g_windowHandle)) 
  {
    return;
  }

  OverlayMessage::PostFrontendMessage(g_windowHandle, OverlayMessageType::Initialized, GetCurrentProcessId());

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) 
  {
    if (OverlayMessage::overlayMessageType == msg.message)
    {
      OverlayMessageType messageType = (OverlayMessageType)msg.wParam;
      switch (messageType)
      {
      case OverlayMessageType::FreeLibrary:
        DisableOverlay();
        break;
      case OverlayMessageType::StartRecording:
        RecordingState::GetInstance().Start();
        break;
      case OverlayMessageType::StopRecording:
        RecordingState::GetInstance().Stop();
        break;
      case OverlayMessageType::ShowOverlay:
        RecordingState::GetInstance().ShowOverlay();
        break;
      case OverlayMessageType::HideOverlay:
        RecordingState::GetInstance().HideOverlay();
        break;
      case OverlayMessageType::UpperLeft:
      case OverlayMessageType::UpperRight:
      case OverlayMessageType::LowerLeft:
      case OverlayMessageType::LowerRight:
      {
        const auto overlayPosition = GetOverlayPositionFromMessageType(messageType);
        RecordingState::GetInstance().SetOverlayPosition(overlayPosition);
        break;
      }
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
    g_messageLog.LogWarning("OverlayThread", "GetModuleHandleEx failed ",
                     GetLastError());
    return;
  }
  RecordingState::GetInstance().HideOverlay();
  FreeLibraryAndExitThread(dll, 0);
}

bool OverlayThread::ThreadStartup(HWND& windowHandle)
{
  windowHandle = FindOcatWindowHandle();
  if (!windowHandle) 
  {
    return false;
  }

  return OverlayMessage::PostFrontendMessage(windowHandle, OverlayMessageType::ThreadInitialized, GetCurrentThreadId());
}

bool OverlayThread::ThreadCleanup(HWND windowHandle)
{
  return OverlayMessage::PostFrontendMessage(windowHandle, OverlayMessageType::ThreadTerminating, GetCurrentThreadId());
}

}
