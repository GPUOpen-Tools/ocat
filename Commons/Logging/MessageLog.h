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

#pragma once

#include <windows.h>
#include <ctime>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <set>

class MessageLog {
public:


  MessageLog();
  ~MessageLog();

  void Start(const std::wstring& logFilePath, const std::wstring& caller, bool overwrite = false);

  void LogError(const std::string& category, const std::string& message, DWORD errorCode = 0);
  void LogError(const std::string& category, const std::wstring& message, DWORD errorCode = 0);
  void LogWarning(const std::string& category, const std::string& message, DWORD errorCode = 0);
  void LogWarning(const std::string& category, const std::wstring& message, DWORD errorCode = 0);
  void LogInfo(const std::string& category, const std::string& message, DWORD errorCode = 0);
  void LogInfo(const std::string& category, const std::wstring& message, DWORD errorCode = 0);
  void LogVerbose(const std::string& category, const std::string& message, DWORD errorCode = 0);
  void LogVerbose(const std::string& category, const std::wstring& message, DWORD errorCode = 0);

  void LogOS();

private:

  enum class LogLevel
  {
    Error,
    Warning,
    Info,
    Verbose,
  };

  const std::wstring logLevelNames_[4] = { L"ERROR", L"WARNING", L"INFO", L"VERBOSE" };

  void SetCurrentTime();
  std::wstring CreateLogMessage(LogLevel logLevel, const std::wstring & category, const std::wstring & message, DWORD errorCode);
  void Log(LogLevel logLevel, const std::string& category, const std::string& message,
    DWORD errorCode = 0);
  void Log(LogLevel logLevel, const std::string& category, const std::wstring& message,
    DWORD errorCode = 0);

  std::wofstream outFile_;
  std::tm currentTime_;
  std::wstring caller_;
  std::wstring parentProcess_;
  std::set<LogLevel> filter_; // Contains all allowed log levels.
  bool started_;
};

extern MessageLog g_messageLog;
