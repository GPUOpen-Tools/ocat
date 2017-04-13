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

#include "Recording.h"

#include <UIAutomationClient.h>
#include <atlcomcli.h>

#include "MessageLog.h"
#include "ProcessHelper.h"

const std::string Recording::defaultProcessName_ = "*";

Recording::Recording() {}
Recording::~Recording() {}
const std::string& Recording::Start()
{
  recording_ = true;
  processName_ = defaultProcessName_;

  if (recordAllProcesses_)
  {
    return processName_;
  }

  processID_ = GetProcessFromWindow();
  if (!processID_ || processID_ == GetCurrentProcessId()) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "Recording",
                     "No active process was found, capturing all processes");
    return processName_;
  }

  processName_ = ConvertUTF16StringToUTF8String(GetProcessNameFromID(processID_));
  g_messageLog.Log(MessageLog::LOG_INFO, "Recording", "Active Process found " + processName_);

  processTermination_.Register(processID_);

  return processName_;
}

void Recording::Stop()
{
  recording_ = false;

  processTermination_.UnRegister();
  processName_.clear();
  processID_ = 0;
}

DWORD Recording::GetProcessFromWindow()
{
  const auto window = GetForegroundWindow();
  if (!window) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "Recording", "No foreground window found");
    return 0;
  }

  DWORD processID = IsUWPWindow(window) ? GetUWPProcess(window) : GetProcess(window);
  return processID;
}

DWORD Recording::GetUWPProcess(HWND window)
{
  HRESULT hr = CoInitialize(NULL);
  if (hr == RPC_E_CHANGED_MODE) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                     "GetUWPProcess: CoInitialize failed, HRESULT", hr);
  }
  else {
    CComPtr<IUIAutomation> uiAutomation;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&uiAutomation));
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                       "GetUWPProcess: CoCreateInstance failed, HRESULT", hr);
    }
    else {
      CComPtr<IUIAutomationElement> foregroundWindow;
      HRESULT hr = uiAutomation->ElementFromHandle(GetForegroundWindow(), &foregroundWindow);
      if (FAILED(hr)) {
        g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                         "GetUWPProcess: ElementFromHandle failed, HRESULT", hr);
      }
      else {
        VARIANT variant;
        variant.vt = VT_BSTR;
        variant.bstrVal = SysAllocString(L"Windows.UI.Core.CoreWindow");
        CComPtr<IUIAutomationCondition> condition;
        hr = uiAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId, variant, &condition);
        if (FAILED(hr)) {
          g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                           "GetUWPProcess: ElemenCreatePropertyCondition failed, HRESULT", hr);
        }
        else {
          CComPtr<IUIAutomationElement> coreWindow;
          HRESULT hr =
              foregroundWindow->FindFirst(TreeScope::TreeScope_Children, condition, &coreWindow);
          if (FAILED(hr)) {
            g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                             "GetUWPProcess: FindFirst failed, HRESULT", hr);
          }
          else {
            int procesID = 0;
            if (coreWindow) {
              hr = coreWindow->get_CurrentProcessId(&procesID);
              if (FAILED(hr)) {
                g_messageLog.Log(MessageLog::LOG_ERROR, "Recording",
                                 "GetUWPProcess: get_CurrentProcessId failed, HRESULT", hr);
              }
              else {
                g_messageLog.Log(MessageLog::LOG_INFO, "Recording",
                                 "GetUWPProcess: found uwp process", procesID);
                return procesID;
              }
            }
          }
        }

        SysFreeString(variant.bstrVal);
      }
    }
  }

  CoUninitialize();
  return 0;
}

DWORD Recording::GetProcess(HWND window)
{
  DWORD processID = 0;
  const auto threadID = GetWindowThreadProcessId(window, &processID);

  if (processID && threadID) {
    return processID;
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "Recording", "GetWindowThreadProcessId failed");
    return 0;
  }
}

bool Recording::IsUWPWindow(HWND window)
{
  const auto className = GetWindowClassName(window);
  if (className.compare(L"ApplicationFrameWindow") == 0) {
    g_messageLog.Log(MessageLog::LOG_INFO, "Recording", "UWP window found");
    return true;
  }
  return false;
}