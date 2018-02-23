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
#include <algorithm>

#include "Logging\MessageLog.h"
#include "Utility\ProcessHelper.h"
#include "Utility\FileUtils.h"
#include "Utility\StringUtils.h"

#include <time.h>

const std::wstring Recording::defaultProcessName_ = L"*";

Recording::Recording() {}
Recording::~Recording() {}

void Recording::Start()
{
  recording_ = true;
  processName_ = defaultProcessName_;
  accumulatedResultsPerProcess_.clear();
  accumulatedResultsPerProcessCompositor_.clear();

  if (recordAllProcesses_)
  {
    g_messageLog.LogInfo("Recording",
      "Capturing all processes");
    return;
  }

  processID_ = GetProcessFromWindow();
  if (!processID_ || processID_ == GetCurrentProcessId()) {
    g_messageLog.LogWarning("Recording",
      "No active process was found, capturing all processes");
    recordAllProcesses_ = true;
    return;
  }

  processName_ = GetProcessNameFromID(processID_);
  g_messageLog.LogInfo("Recording", L"Active Process found " + processName_);
  recordAllProcesses_ = false;
}

void Recording::Stop()
{
  PrintSummary();
  recording_ = false;
  processName_.clear();
  processID_ = 0;
}

bool Recording::IsRecording() const
{ 
  return recording_;
}

const std::wstring & Recording::GetProcessName() const
{
  return processName_;
}

void Recording::SetRecordingDirectory(const std::wstring & dir)
{
  directory_ = dir;
}

void Recording::SetRecordAllProcesses(bool recordAll)
{
  recordAllProcesses_ = recordAll;
}

bool Recording::GetRecordAllProcesses()
{
  return recordAllProcesses_;
}

const std::wstring & Recording::GetDirectory()
{
  return directory_;
}

DWORD Recording::GetProcessFromWindow()
{
  const auto window = GetForegroundWindow();
  if (!window) {
    g_messageLog.LogError("Recording", "No foreground window found");
    return 0;
  }

  DWORD processID = IsUWPWindow(window) ? GetUWPProcess(window) : GetProcess(window);
  return processID;
}

DWORD Recording::GetUWPProcess(HWND window)
{
  HRESULT hr = CoInitialize(NULL);
  if (hr == RPC_E_CHANGED_MODE) {
    g_messageLog.LogError("Recording",
      "GetUWPProcess: CoInitialize failed, HRESULT", hr);
  }
  else {
    CComPtr<IUIAutomation> uiAutomation;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(&uiAutomation));
    if (FAILED(hr)) {
      g_messageLog.LogError("Recording",
        "GetUWPProcess: CoCreateInstance failed, HRESULT", hr);
    }
    else {
      CComPtr<IUIAutomationElement> foregroundWindow;
      HRESULT hr = uiAutomation->ElementFromHandle(GetForegroundWindow(), &foregroundWindow);
      if (FAILED(hr)) {
        g_messageLog.LogError("Recording",
          "GetUWPProcess: ElementFromHandle failed, HRESULT", hr);
      }
      else {
        VARIANT variant;
        variant.vt = VT_BSTR;
        variant.bstrVal = SysAllocString(L"Windows.UI.Core.CoreWindow");
        CComPtr<IUIAutomationCondition> condition;
        hr = uiAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId, variant, &condition);
        if (FAILED(hr)) {
          g_messageLog.LogError("Recording",
            "GetUWPProcess: ElemenCreatePropertyCondition failed, HRESULT", hr);
        }
        else {
          CComPtr<IUIAutomationElement> coreWindow;
          HRESULT hr =
            foregroundWindow->FindFirst(TreeScope::TreeScope_Children, condition, &coreWindow);
          if (FAILED(hr)) {
            g_messageLog.LogError("Recording",
              "GetUWPProcess: FindFirst failed, HRESULT", hr);
          }
          else {
            int procesID = 0;
            if (coreWindow) {
              hr = coreWindow->get_CurrentProcessId(&procesID);
              if (FAILED(hr)) {
                g_messageLog.LogError("Recording",
                  "GetUWPProcess: get_CurrentProcessId failed, HRESULT", hr);
              }
              else {
                g_messageLog.LogInfo("Recording",
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
    g_messageLog.LogError("Recording", "GetWindowThreadProcessId failed");
    return 0;
  }
}

bool Recording::IsUWPWindow(HWND window)
{
  const auto className = GetWindowClassName(window);
  if (className.compare(L"ApplicationFrameWindow") == 0) {
    g_messageLog.LogInfo("Recording", "UWP window found");
    return true;
  }
  return false;
}

void Recording::FrameStats::UpdateFrameStats(bool presented) {
  if (!presented) {
    totalMissed++;
    consecutiveMissed++;
  }
  else {
    if (consecutiveMissed > maxConsecutiveMissed) {
      maxConsecutiveMissed = consecutiveMissed;
    }
    consecutiveMissed = 0;
  }
}

void Recording::AddPresent(const std::string& processName, double timeInSeconds, double msBetweenPresents,
  PresentFrameInfo frameInfo)
{
  AccumulatedResults* accInput;

  if (frameInfo == PresentFrameInfo::DXGI) {
    auto it = accumulatedResultsPerProcess_.find(processName);
    if (it == accumulatedResultsPerProcess_.end())
    {
      AccumulatedResults input = {};
      input.startTime = FormatCurrentTime();
      g_messageLog.LogInfo("Recording", "Received first present for process " + processName + " at " + input.startTime + ".");
      accumulatedResultsPerProcess_.insert(std::pair<std::string, AccumulatedResults>(processName, input));
      it = accumulatedResultsPerProcess_.find(processName);
    }

    accInput = &it->second;
    accInput->timeInSeconds = timeInSeconds;
    accInput->frameTimes.push_back(msBetweenPresents);
    return;
  }
  // Compositor present
  else {
    auto it = accumulatedResultsPerProcessCompositor_.find(processName);
    if (it == accumulatedResultsPerProcessCompositor_.end())
    {
      AccumulatedResults input = {};
      input.startTime = FormatCurrentTime();
      g_messageLog.LogInfo("Recording", "Received first present for process " + processName + " at " + input.startTime + ".");
      accumulatedResultsPerProcessCompositor_.insert(std::pair<std::string, AccumulatedResults>(processName, input));
      it = accumulatedResultsPerProcessCompositor_.find(processName);
    }

    accInput = &it->second;
    accInput->timeInSeconds = timeInSeconds;
    accInput->frameTimes.push_back(msBetweenPresents);
  }

  switch (frameInfo)
  {
  case PresentFrameInfo::COMPOSITOR_APP_WARP:
  {
    accInput->app.UpdateFrameStats(true);
    accInput->warp.UpdateFrameStats(true);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APPMISS_WARP:
  {
    accInput->app.UpdateFrameStats(false);
    accInput->warp.UpdateFrameStats(true);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APP_WARPMISS:
  {
    accInput->app.UpdateFrameStats(true);
    accInput->warp.UpdateFrameStats(false);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS:
  {
    accInput->app.UpdateFrameStats(false);
    accInput->warp.UpdateFrameStats(false);
    return;
  }
  }
}

std::string Recording::FormatCurrentTime()
{
  struct tm tm;
  time_t time_now = time(NULL);
  localtime_s(&tm, &time_now);
  char buffer[4096];
  _snprintf_s(buffer, _TRUNCATE, "%4d%02d%02d-%02d%02d%02d",  // ISO 8601
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return std::string(buffer);
}

void Recording::PrintSummary()
{
  if (accumulatedResultsPerProcess_.size() == 0)
  {
    // Only print the summary, if we have actual results.
    return;
  }

  std::wstring summaryFilePath = directory_ + L"perf_summary.csv";
  bool summaryFileExisted = FileExists(summaryFilePath);

  // Open summary file, possibly create it.
  std::ofstream summaryFile(summaryFilePath, std::ofstream::app);
  if (summaryFile.fail())
  {
    g_messageLog.LogError("Recording",
      "Can't open summary file. Either it is open in another process or OCAT is missing write permissions.");
    return;
  }

  // If newly created, append header:
  if (!summaryFileExisted)
  {
    std::string header = "Application Name,Date and Time,Average FPS (Application)," \
      "Average frame time (ms) (Application),99th-percentile frame time (ms) (Application)," \
      "Missed frames (Application),Average number of missed frames (Application)," \
      "Maximum number of consecutive missed frames (Application),Missed frames (Compositor)," \
      "Average number of missed frames (Compositor),Maximum number of consecutive missed frames (Compositor)\n";
    summaryFile << header;
  }

  for (auto& item : accumulatedResultsPerProcess_)
  {
    std::stringstream line;
    AccumulatedResults& input = item.second;

    double avgFPS = input.frameTimes.size() / input.timeInSeconds;
    double avgFrameTime = (input.timeInSeconds * 1000.0) / input.frameTimes.size();
    std::sort(input.frameTimes.begin(), input.frameTimes.end(), std::less<double>());
    const auto rank = static_cast<int>(0.99 * input.frameTimes.size());
    double frameTimePercentile = input.frameTimes[rank];

    line << item.first << "," << input.startTime << "," << avgFPS << ","
      << avgFrameTime << "," << frameTimePercentile << ","
      << "-,-,-,-,-,-" << std::endl;

    summaryFile << line.str();
  }

  for (auto& item : accumulatedResultsPerProcessCompositor_)
  {
    std::stringstream line;
    AccumulatedResults& input = item.second;

    double avgFPS = input.frameTimes.size() / input.timeInSeconds;
    double avgFrameTime = (input.timeInSeconds * 1000.0) / input.frameTimes.size();
    std::sort(input.frameTimes.begin(), input.frameTimes.end(), std::less<double>());
    const auto rank = static_cast<int>(0.99 * input.frameTimes.size());
    double frameTimePercentile = input.frameTimes[rank];
    double avgMissedFramesApp = static_cast<double> (input.app.totalMissed)
      / (input.frameTimes.size() + input.app.totalMissed);
    double avgMissedFramesCompositor = static_cast<double> (input.warp.totalMissed)
      / (input.frameTimes.size() + input.warp.totalMissed);

    line << item.first << "," << input.startTime << "," << avgFPS << ","
      << avgFrameTime << "," << frameTimePercentile << "," << input.app.totalMissed << ","
      << avgMissedFramesApp << "," << input.app.maxConsecutiveMissed << ","
      << input.warp.totalMissed << "," << avgMissedFramesCompositor << ","
      << input.warp.maxConsecutiveMissed << std::endl;

    summaryFile << line.str();
  }
}
