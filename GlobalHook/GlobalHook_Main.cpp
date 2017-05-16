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

#include <Windows.h>

#include "..\Overlay\Hook.h"
#include "Constants.h"
#include "MessageLog.h"
#include "ProcessHelper.h"
#include "FileDirectory.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
  g_messageLog.Log(MessageLog::LOG_VERBOSE, "GlobalHook", "Entered wWinMain");

  Hook globalHook;
  Hook::HookInfo hookInfo;
  hookInfo.hookID = WH_CBT;

#if _WIN64
  g_messageLog.Start(g_fileDirectory.GetDirectory(FileDirectory::DIR_LOG) + "GlobalHook64Log", "GlobalHook64");
  hookInfo.hookFunction = g_globalHookFunction64;
  hookInfo.libName = g_fileDirectory.GetDirectoryW(FileDirectory::DIR_BIN) + g_libraryName64;
#else
  g_messageLog.Start(g_fileDirectory.GetDirectory(FileDirectory::DIR_LOG) + "GlobalHook32Log", "GlobalHook32");
  hookInfo.hookFunction = g_globalHookFunction32;
  hookInfo.libName = g_fileDirectory.GetDirectoryW(FileDirectory::DIR_BIN) + g_libraryName32;
#endif

  if (!globalHook.Activate(hookInfo)) {
    g_messageLog.Log(MessageLog::LOG_INFO, "GlobalHook", "Could not activate hook");
    return -1;
  }
  g_messageLog.Log(MessageLog::LOG_INFO, "GlobalHook", "Successfully activated hook");

  MSG msg;
  //Wait for WM_QUIT
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  globalHook.Deactivate();
  g_messageLog.Log(MessageLog::LOG_INFO, "GlobalHook", "Shutdown");
  return (int)msg.wParam;
}