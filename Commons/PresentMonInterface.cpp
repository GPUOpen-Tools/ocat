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

std::mutex g_RecordingMutex;
Recording g_Recording;
bool g_ProcessFinished;

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

static void LockedStopRecording(CommandLineArgs* args)
{
    if (g_Recording.IsRecording())
    {
        g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop recording");
        if (EtwThreadsRunning())
        {
            StopEtwThreads(args);
        }
        g_Recording.Stop();
    }
}

PresentMonInterface::~PresentMonInterface() 
{
    std::lock_guard<std::mutex> lock(g_RecordingMutex);
    LockedStopRecording(args_);
	if (args_)
	{
		delete args_;
		args_ = nullptr;
	}
}

void PresentMonInterface::StartCapture(HWND hwnd)
{
    Config config;

    if (config.Load(g_fileDirectory.GetDirectoryW(FileDirectory::DIR_CONFIG))) 
    {
        config.SetPresentMonArgs(*args_);
        g_Recording.SetRecordingDirectory(g_fileDirectory.GetDirectory(FileDirectory::DIR_RECORDING));
        g_Recording.SetRecordAllProcesses(config.recordAllProcesses_);
    }

    hwnd_ = hwnd;

    if (!initialized_) 
    {
        SetMessageFilter();
        initialized_ = true;
    }
}

void PresentMonInterface::StopCapture()
{
    std::lock_guard<std::mutex> lock(g_RecordingMutex);
    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface", "Stop capturing");
    StopRecording();
}

void PresentMonInterface::KeyEvent()
{
    std::lock_guard<std::mutex> lock(g_RecordingMutex);
    if (g_Recording.IsRecording()) 
    {
        StopRecording();
    }
    else 
    {
        StartRecording();
    }
}

bool PresentMonInterface::ProcessFinished()
{
    // only interested in finished process if specific process is captured
    const bool captureAll = args_->mTargetProcessName == nullptr;
    const auto finished = captureAll ? false : g_ProcessFinished;
    g_ProcessFinished = false;
    return finished;
}

void CALLBACK OnProcessExit(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
    // copy paste from StopEtwThreads (PresentMon --> main.cpp)
    g_StopEtwThreads = true;
    g_EtwConsumingThread.join();
    g_Recording.Stop();
    g_ProcessFinished = true;
}

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
    assert(g_Recording.IsRecording() == false);

    std::stringstream outputFilePath;
    outputFilePath << g_Recording.GetDirectory() << "perf_";

    g_Recording.Start();
    if (g_Recording.GetRecordAllProcesses())
    {
        outputFilePath << "AllProcesses";
        args_->mTargetProcessName = nullptr;
    }
    else 
    {
        outputFilePath << g_Recording.GetProcessName();
        args_->mTargetProcessName = g_Recording.GetProcessName().c_str();
    }

    auto dateAndTime = FormatCurrentTime();
    outputFilePath << "_" << dateAndTime << "_RecordingResults";
    presentMonOutputFilePath_ = outputFilePath.str() + ".csv";
    args_->mOutputFileName = presentMonOutputFilePath_.c_str();

    // Keep the output file path in the current recording to attach 
    // its contents to the performance summary later on.
    outputFilePath << "-" << args_->mRecordingCount << ".csv";
    g_Recording.SetOutputFilePath(outputFilePath.str());
    g_Recording.SetDateAndTime(dateAndTime);

    g_messageLog.Log(MessageLog::LOG_INFO, "PresentMonInterface",
                   "Start recording " + g_Recording.GetProcessName());
    StartEtwThreads(*args_);
}

void PresentMonInterface::StopRecording()
{
    LockedStopRecording(args_);
}

const std::string PresentMonInterface::GetRecordedProcess()
{
    if (g_Recording.IsRecording()) 
    {
        return g_Recording.GetProcessName();
    }
    return "";
}

bool PresentMonInterface::CurrentlyRecording() 
{ 
    return g_Recording.IsRecording(); 
}

bool PresentMonInterface::SetMessageFilter()
{
    CHANGEFILTERSTRUCT changeFilter;
    changeFilter.cbSize = sizeof(CHANGEFILTERSTRUCT);
    if (!ChangeWindowMessageFilterEx(hwnd_, WM_APP + 1, MSGFLT_ALLOW, &changeFilter)) 
    {
        g_messageLog.Log(MessageLog::LOG_ERROR, "PresentMonInterface",
                            "ChangeWindowMessageFilterEx failed", GetLastError());
        return false;
    }
    return true;
}
