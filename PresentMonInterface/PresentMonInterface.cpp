/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "PresentMonInterface.h"
#include "Config\BlackList.h"
#include "Config\Config.h"
#include "Logging\MessageLog.h"
#include "Recording.h"
#include "Utility\Constants.h"
#include "Utility\ProcessHelper.h"
#include "Utility\FileDirectory.h"
#include "Utility\StringUtils.h"

#include "..\PresentMon\PresentMon\PresentMon.hpp"

// avoid changing the PresentMon implementation by including main.cpp directly
#include "..\PresentMon\PresentMon\main.cpp"

std::mutex g_RecordingMutex;

PresentMonInterface::PresentMonInterface()
{
  // Nothing to do
}

PresentMonInterface::~PresentMonInterface()
{
  std::lock_guard<std::mutex> lock(g_RecordingMutex);
  StopRecording();
}

bool PresentMonInterface::Init(HWND hwnd)
{
  if (!g_fileDirectory.Initialize())
  {
    return false;
  }

  BlackList blackList;
  blackList.Load();
  blackList_ = blackList.GetBlackList();

  g_hWnd = hwnd; // Tell PresentMon where to send its messages 
  recording_.SetRecordingDirectory(g_fileDirectory.GetDirectory(DirectoryType::Recording));
  g_messageLog.Start(g_fileDirectory.GetDirectory(DirectoryType::Log) + g_logFileName,
    L"PresentMon", false);
  g_messageLog.LogOS();
  return true;
}

int PresentMonInterface::GetPresentMonRecordingStopMessage()
{
  return WM_STOP_ETW_THREADS;
}

void PresentMonInterface::UpdateOutputFolder(const std::wstring& outputFolder)
{
  recording_.SetRecordingDirectory(outputFolder + L"\\");
  g_messageLog.LogInfo("OutputFolder", "Updated");
}

void PresentMonInterface::UpdateUserNote(const std::wstring& userNote)
{
  recording_.SetUserNote(userNote);
}

void PresentMonInterface::SetPresentMonArgs(unsigned int timer)
{
  args_ = {};
  args_.mHotkeySupport = false;

  // default is verbose
  args_.mVerbosity = Verbosity::Verbose;
  // Keep in sync with enum in Frontend
  // ---> removed, only record in Verbose mode unless capture config says differently
  /*if (recordingDetail == 0)
  {
    args_.mVerbosity = Verbosity::Simple;
  }
  else if (recordingDetail == 1)
  {
    args_.mVerbosity = Verbosity::Normal;
  }
  else if (recordingDetail == 2)
  {
    args_.mVerbosity = Verbosity::Verbose;
  }*/

  if (timer > 0) {
    args_.mTimer = timer;
  }

  if (recording_.GetRecordAllProcesses())
  {
    args_.mTargetProcessNames.clear();
  }
  else
  {
    targetProcessName_ = ConvertUTF16StringToUTF8String(recording_.GetProcessName());
    args_.mTargetProcessNames.emplace_back(targetProcessName_.c_str());
  }

  outputFileName_ = ConvertUTF16StringToUTF8String(recording_.GetDirectory() + L"OCAT.csv");
  args_.mOutputFileName = outputFileName_.c_str();

  // We want to keep our OCAT window open.
  args_.mTerminateOnProcExit = false;
  args_.mTerminateAfterTimer = false;
  
  args_.mMultiCsv = true;
  args_.mOutputFile = true;

  args_.mPresentCallback = [this](const std::wstring & fileName,const std::wstring & processName, const CompositorInfo compositor, double timeInSeconds, double msBetweenPresents,
      PresentFrameInfo frameInfo) {
    recording_.AddPresent(fileName, processName, compositor, timeInSeconds, msBetweenPresents, frameInfo);
  };

  args_.mBlackList = blackList_;

  args_.mIncludeWindowsMixedReality = true;

  ConfigCapture config;
  config.Load(g_fileDirectory.GetDirectory(DirectoryType::Config));
  args_.mProviders = config.provider;
}

void PresentMonInterface::ToggleRecording(bool recordAllProcesses, unsigned int timer)
{
  std::lock_guard<std::mutex> lock(g_RecordingMutex);
  if (recording_.IsRecording())
  {
    StopRecording();
  }
  else
  {
    StartRecording(recordAllProcesses, timer);
  }
}

void PresentMonInterface::StartRecording(bool recordAllProcesses, unsigned int timer)
{
  assert(recording_.IsRecording() == false);

  recording_.SetRecordAllProcesses(recordAllProcesses);
  recording_.Start();

  g_messageLog.LogInfo("PresentMonInterface",
    L"Start recording " + recording_.GetProcessName());
  
  SetPresentMonArgs(timer);
  StartEtwThreads(args_);
}

void PresentMonInterface::StopRecording()
{
  if (recording_.IsRecording())
  {
    g_messageLog.LogInfo("PresentMonInterface", "Stop recording");
    if (EtwThreadsRunning())
    {
      StopEtwThreads(&args_);
    }
    recording_.Stop();
  }
}

const std::wstring PresentMonInterface::GetRecordedProcess()
{
  if (recording_.IsRecording())
  {
    return recording_.GetProcessName();
  }
  return L"";
}

bool PresentMonInterface::CurrentlyRecording()
{
  return EtwThreadsRunning();
}
