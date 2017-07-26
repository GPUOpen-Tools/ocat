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
#include "Config\Config.h"
#include "Utility\FileDirectory.h"
#include "Logging\MessageLog.h"
#include "Recording\Recording.h"
#include "Utility\Constants.h"
#include "Utility\ProcessHelper.h"
#include "Config\BlackList.h"

#include "..\PresentMon\PresentMon\PresentMon.hpp"

// avoid changing the PresentMon implementation by including main.cpp directly
#include "..\PresentMon\PresentMon\main.cpp"

bool g_StopRecording = false;
bool g_processFinished = false;

std::mutex g_RecordingMutex;
std::thread g_RecordingThread;
Recording g_recording;

PresentMonInterface::PresentMonInterface()
{
	args_ = new CommandLineArgs();
    g_fileDirectory.CreateDirectories();
    g_messageLog.Start(g_fileDirectory.GetDirectory(FileDirectory::DIR_LOG) + g_logFileName,
                     "PresentMon", false);
    g_messageLog.LogOS();
    BlackList blackList;
    blackList.Load();
}

PresentMonInterface::~PresentMonInterface() 
{
	if (args_)
	{
		delete args_;
		args_ = nullptr;
	}
}

void PresentMonInterface::StartCapture(HWND hwnd)
{
    Config config;

    if (config.Load(g_fileDirectory.GetDirectoryW(FileDirectory::DIR_CONFIG))) {
      config.SetPresentMonArgs(*args_);
      g_recording.SetRecordingDirectory(g_fileDirectory.GetDirectory(FileDirectory::DIR_RECORDING));
      g_recording.SetRecordAllProcesses(config.recordAllProcesses_);
    }

    hwnd_ = hwnd;

    if (!initialized_) {
      SetMessageFilter();
      initialized_ = true;
    }
}

static void LockedStopRecording(CommandLineArgs* args)
{
	g_StopRecording = true;
	if (g_RecordingThread.joinable()) {
		g_RecordingThread.join();
	}

    if (EtwThreadsRunning())
    {
        StopEtwThreads(args);
    }
}

void PresentMonInterface::StopCapture()
{
    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop capturing");
	LockedStopRecording(args_);
}

static void LockedStartRecording(CommandLineArgs& args, HWND /*window*/)
{
	g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Start capturing");
	g_StopRecording = false;
    StartEtwThreads(args);
}

void PresentMonInterface::KeyEvent()
{
    std::lock_guard<std::mutex> lock(g_RecordingMutex);
    if (g_recording.IsRecording()) {
        StopRecording();
    }
    else {
        StartRecording();
    }
}

bool PresentMonInterface::ProcessFinished()
{
    // only interested in finished process if specific process is captured
    const bool captureAll = args_->mTargetProcessName == nullptr;
    const auto finished = captureAll ? false : g_processFinished;
    g_processFinished = false;
    return finished;
}

void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
    // copy paste from StopEtwThreads (PresentMon --> main.cpp)
    g_StopEtwThreads = true;
    g_EtwConsumingThread.join();

    g_recording.Stop();
    g_processFinished = true;
}

// TODO check for duplicate in RecordingResults::Result::CreateOutputPath
std::string FormatCurrentTime() 
{
    struct tm tm;
    time_t time_now = time(NULL);
    localtime_s(&tm, &time_now);
    char buffer[4096];
    _snprintf_s(buffer, _TRUNCATE, "%4d%02d%02d-%02d%02d%02d",  // ISO 8601
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buffer);
}

void PresentMonInterface::StartRecording()
{
    assert(g_recording.IsRecording() == false);

    std::stringstream outputFilePath;
    outputFilePath << g_recording.GetDirectory() << "perf_";

    g_recording.Start();
    if (g_recording.GetRecordAllProcesses())
    {
        outputFilePath << "AllProcesses";
        args_->mTargetProcessName = nullptr;
    }
    else 
    {
        outputFilePath << g_recording.GetProcessName();
        args_->mTargetProcessName = g_recording.GetProcessName().c_str();
    }

    outputFilePath << "_" << FormatCurrentTime() << "_RecordingResult";
    presentMonOutputFilePath_ = outputFilePath.str() + ".csv";
    args_->mOutputFileName = presentMonOutputFilePath_.c_str();

    // Keep the output file path in the current recording to attach 
    // its contents to the performance summary later on.
    outputFilePath << "-" << args_->mRecordingCount << ".csv";
    g_recording.SetOutputFilePath(outputFilePath.str());

    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface",
                   "Start capturing " + g_recording.GetProcessName());
    LockedStartRecording(*args_, hwnd_);
}

void PresentMonInterface::StopRecording()
{
    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop capturing");
	if (g_recording.IsRecording()) {
		LockedStopRecording(args_);
	}
    g_recording.Stop();
}

const std::string PresentMonInterface::GetRecordedProcess()
{
  if (g_recording.IsRecording()) {
    return g_recording.GetProcessName();
  }
  return "";
}

bool PresentMonInterface::CurrentlyRecording() 
{ 
    return g_recording.IsRecording(); 
}

bool PresentMonInterface::SetMessageFilter()
{
  CHANGEFILTERSTRUCT changeFilter;
  changeFilter.cbSize = sizeof(CHANGEFILTERSTRUCT);
  if (!ChangeWindowMessageFilterEx(hwnd_, WM_APP + 1, MSGFLT_ALLOW, &changeFilter)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "PresentMonInterface",
                     "ChangeWindowMessageFilterEx failed", GetLastError());
    return false;
  }
  return true;
}
