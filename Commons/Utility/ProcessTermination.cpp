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

#include "ProcessTermination.h"
#include "..\Logging\MessageLog.h"

extern void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired);

void ProcessTermination::Register(DWORD processID)
{
  if (processExitHandle_) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "ProcessTermination",
                     "Failed to unregister process exit handle for " + std::to_string(processID_));
  }

  processID_ = processID;
  g_messageLog.Log(MessageLog::LOG_INFO, "ProcessTermination",
                   "Registering process termination ID " + std::to_string(processID_));
  auto processHandle = OpenProcess(SYNCHRONIZE, FALSE, processID_);
  if (processHandle) {
    if (!RegisterWaitForSingleObject(&processExitHandle_, processHandle, OnProcessExit, NULL,
                                     INFINITE, WT_EXECUTEONLYONCE)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "ProcessTermination",
                       "Registering process end callback failed", GetLastError());
    }
    return;
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "ProcessTermination",
                     "Opening Process with synchronization failed ", GetLastError());
  }

  CloseHandle(processHandle);
}

void ProcessTermination::UnRegister()
{
  if (processExitHandle_ && !UnregisterWait(processExitHandle_)) {
    const auto error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      g_messageLog.Log(MessageLog::LOG_WARNING, "ProcessTermination",
                       "UnregisterWait failed for " + std::to_string(processID_), error);
    }
  }
  processExitHandle_ = NULL;
}