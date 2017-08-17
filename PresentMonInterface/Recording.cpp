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

const std::string Recording::defaultProcessName_ = "*";

Recording::Recording() {}
Recording::~Recording() {}
 
void Recording::Start()
{
    recording_ = true;
    processName_ = defaultProcessName_;

    if (recordAllProcesses_)
    {
        g_messageLog.Log(MessageLog::LOG_INFO, "Recording",
            "Capturing all processes");
        return;
    }

    processID_ = GetProcessFromWindow();
    if (!processID_ || processID_ == GetCurrentProcessId()) {
        g_messageLog.Log(MessageLog::LOG_WARNING, "Recording",
                         "No active process was found, capturing all processes");
        recordAllProcesses_ = true;
        return;
    }

    processName_ = ConvertUTF16StringToUTF8String(GetProcessNameFromID(processID_));
    g_messageLog.Log(MessageLog::LOG_INFO, "Recording", "Active Process found " + processName_);
    recordAllProcesses_ = false;
}

void Recording::Stop()
{
  recording_ = false;
  processName_.clear();
  processID_ = 0;

  auto summary = ReadPerformanceData();
  PrintSummary(summary);
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

// Returns true if all columns could be set. False otherwise.
bool RetrieveColumnIndices(const std::string& line, int& processNameIndex, int& timeInSecondsIndex, int& frameTimeIndex)
{
  processNameIndex = -1;
  timeInSecondsIndex = -1;
  frameTimeIndex = -1;

  std::vector<std::string> columns = Split(line, ',');
  for (int i = 0; i < static_cast<int>(columns.size()); ++i)
  {
    if (columns[i] == "Application")
    {
      processNameIndex = i;
      continue;
    }

    if (columns[i] == "TimeInSeconds")
    {
      timeInSecondsIndex = i;
      continue;
    }

    if (columns[i] == "MsBetweenPresents")
    {
      frameTimeIndex = i;
      continue;
    }
  }

  bool result = true;
  if (processNameIndex == -1)
  {
    g_messageLog.Log(MessageLog::LogLevel::LOG_WARNING, "Recording",
      "Invalid format, could not retrieve column 'Application'.");
    result = false;
  }

  if (timeInSecondsIndex == -1)
  {
    g_messageLog.Log(MessageLog::LogLevel::LOG_WARNING, "Recording",
      "Invalid format, could not retrieve column 'TimeInSeconds'.");
    result = false;
  }

  if (frameTimeIndex == -1)
  {
    g_messageLog.Log(MessageLog::LogLevel::LOG_WARNING, "Recording",
      "Invalid format, could not retrieve column 'MsBetweenPresents'.");
    result = false;
  }
  return result;
}

std::unordered_map<std::string, Recording::AccumulatedResults> Recording::ReadPerformanceData()
{
    // Map that accumulates the performance numbers of various processes.
    std::unordered_map<std::string, AccumulatedResults> summary;

    // Collect performance data.
    std::ifstream recordedFile(outputFilePath_);
    if (!recordedFile.good())
    {
        g_messageLog.Log(MessageLog::LogLevel::LOG_ERROR, "Recording",
            "Can't open recording file. Either it is open in another process or OCAT is missing read permissions.");
        return summary;
    }

    // Read all lines of the input file.
    std::string line;
    
    // Retrieve column indices from the header line
    std::getline(recordedFile, line);
    int processNameIndex, timeInSecondsIndex, frameTimeIndex;
    if (!RetrieveColumnIndices(line, processNameIndex, timeInSecondsIndex, frameTimeIndex))
    {
      return summary;
    }

    while (std::getline(recordedFile, line))
    {
        std::vector<std::string> columns = Split(line, ',');
        std::string processName = columns[processNameIndex];
        auto it = summary.find(processName);
        if (it == summary.end())
        {
            AccumulatedResults input = {};
            summary.insert(std::pair<std::string, AccumulatedResults>(processName, input));
            it = summary.find(processName);
        }

        AccumulatedResults& accInput = it->second;
        // TimeInSeconds
        accInput.timeInSeconds = std::atof(columns[timeInSecondsIndex].c_str());
        // MsBetweenPresents
        accInput.frameTimes.push_back(std::atof(columns[frameTimeIndex].c_str()));
    }

    return summary;
}

void Recording::PrintSummary(const std::unordered_map<std::string, AccumulatedResults>& summary)
{
    std::string summaryFilePath = directory_ + "perf_summary.csv";
    bool summaryFileExisted = FileExists(summaryFilePath);

    // Open summary file, possibly create it.
    std::ofstream summaryFile(summaryFilePath, std::ofstream::app);
    if (summaryFile.fail())
    {
        g_messageLog.Log(MessageLog::LogLevel::LOG_ERROR, "Recording",
            "Can't open summary file. Either it is open in another process or OCAT is missing write permissions.");
        return;
    }

    // If newly created, append header:
    if (!summaryFileExisted)
    {
        std::string header = "Application Name,Date and Time,Average FPS," \
            "Average frame time (ms),99th-percentile frame time (ms)\n";
        summaryFile << header;
    }

    for (auto& item : summary)
    {
        std::stringstream line;
        const AccumulatedResults& input = item.second;

        double avgFPS = input.frameTimes.size() / input.timeInSeconds;
        double avgFrameTime = (input.timeInSeconds * 1000.0) / input.frameTimes.size();
        std::vector<double> frameTimes(input.frameTimes);
        std::sort(frameTimes.begin(), frameTimes.end(), std::less<double>());
        const auto rank = static_cast<int>(0.99 * frameTimes.size());
        double frameTimePercentile = frameTimes[rank];

        line << item.first << "," << dateAndTime_ << "," << avgFPS << "," 
            << avgFrameTime << "," << frameTimePercentile << std::endl;

        summaryFile << line.str();
    }
}
