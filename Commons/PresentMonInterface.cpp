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

static const uint32_t c_Hotkey = 0x80;

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

static void LockedStopRecording()
{
	g_StopRecording = true;
	if (g_RecordingThread.joinable()) {
		g_RecordingThread.join();
	}
}

void PresentMonInterface::StopCapture()
{
  g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop capturing");
	LockedStopRecording();
}

static void LockedStartRecording(CommandLineArgs& args, HWND window)
{
	g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Start capturing");

	g_StopRecording = false;
	//TODO g_RecordingThread = std::thread(PresentMonEtw, args, window);
    g_RecordingThread = std::thread(EtwConsumingThread, args);
}

void PresentMonInterface::KeyEvent()
{
    std::lock_guard<std::mutex> lock(g_RecordingMutex);
    if (g_recording.IsRecording()) {
        StopRecording();
        // args_->mRestartCount++;
        args_->mRecordingCount++;
    }
    else {
        StartRecording();
    }
}

bool PresentMonInterface::ProcessFinished()
{
    // only interested in finished process if specific process is captured
    // TODO const auto finished = args_->mCaptureAll ? false : g_processFinished;
    const bool captureAll = args_->mTargetProcessName == nullptr;
    const auto finished = captureAll ? false : g_processFinished;
    g_processFinished = false;
    return finished;
}

void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
  LockedStopRecording();
  g_recording.Stop();
  g_processFinished = true;
}

 void PresentMonInterface::StartRecording()
{
  std::string targetProcess;
	assert(g_recording.IsRecording() == false);
  args_->mTargetProcessName = g_recording.Start().c_str();
  // TODO args_->mOutputDirectory = g_recording.GetDirectory().c_str();
  args_->mOutputFileName = g_recording.GetDirectory().c_str();
  g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface",
                   "Start capturing " + g_recording.GetProcessName());
  LockedStartRecording(*args_, hwnd_);
}

void PresentMonInterface::StopRecording()
{
    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop capturing");
	if (g_recording.IsRecording()) {
		LockedStopRecording();
	}
	
    // TODO args_->mRestartCount++;
    args_->mRecordingCount++;
    g_recording.Stop();
}

const std::string PresentMonInterface::GetRecordedProcess()
{
  if (g_recording.IsRecording()) {
    return g_recording.GetProcessName();
  }
  return "";
}

bool PresentMonInterface::CurrentlyRecording() { return g_recording.IsRecording(); }
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
