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

#include "MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "../Utility/StringUtils.h"

#include <windows.h>
#include <chrono>
#include <ctime>
#include <iomanip>

MessageLog g_messageLog;

MessageLog::MessageLog()
  : filter_({ LogLevel::Error, LogLevel::Info, LogLevel::Warning }),
  started_(false), caller_(L"")
{
  parentProcess_ = std::to_wstring(GetCurrentProcessId()) + L" " + GetProcessNameFromHandle(GetCurrentProcess());
}

MessageLog::~MessageLog()
{
  outFile_.close();
}

void MessageLog::Start(const std::wstring& logFilePath, const std::wstring& caller, bool overwrite)
{
  auto openMode = overwrite ? std::wofstream::out : std::wofstream::app;
  outFile_.open(logFilePath + L".txt", openMode);
  if (!outFile_.is_open())
  {
    const std::wstring message = L"Unable to open logFile " + logFilePath + L" for " + caller;
    LogWarning("MessageLog", message);
  }
  caller_ = caller;
  started_ = true;
  LogInfo("MessageLog", "Logging started");
}

void MessageLog::LogError(const std::string & category, const std::string & message, DWORD errorCode)
{
  Log(LogLevel::Error, category, message, errorCode);
}

void MessageLog::LogError(const std::string & category, const std::wstring & message, DWORD errorCode)
{
  Log(LogLevel::Error, category, message, errorCode);
}

void MessageLog::LogWarning(const std::string & category, const std::string & message, DWORD errorCode)
{
  Log(LogLevel::Warning, category, message, errorCode);
}

void MessageLog::LogWarning(const std::string & category, const std::wstring & message, DWORD errorCode)
{
  Log(LogLevel::Warning, category, message, errorCode);
}

void MessageLog::LogInfo(const std::string & category, const std::string & message, DWORD errorCode)
{
  Log(LogLevel::Info, category, message, errorCode);
}

void MessageLog::LogInfo(const std::string & category, const std::wstring & message, DWORD errorCode)
{
  Log(LogLevel::Info, category, message, errorCode);
}

void MessageLog::LogVerbose(const std::string & category, const std::string & message, DWORD errorCode)
{
  Log(LogLevel::Verbose, category, message, errorCode);
}

void MessageLog::LogVerbose(const std::string & category, const std::wstring & message, DWORD errorCode)
{
  Log(LogLevel::Verbose, category, message, errorCode);
}

std::wstring MessageLog::CreateLogMessage(LogLevel logLevel, const std::wstring& category, const std::wstring& message,
  DWORD errorCode)
{
  SetCurrentTime();
  std::wostringstream outstream;
  outstream << std::put_time(&currentTime_, L"%c") << "\t";

  outstream << std::left << std::setw(12) << std::setfill(L' ');
  outstream << logLevelNames_[static_cast<int>(logLevel)];
  if (started_)
  {
    outstream << caller_ << " ";
  }
  outstream << parentProcess_;
  outstream << " - " << category;
  outstream << " - " << message;
  if (errorCode)
  {
    auto systemErrorMessage = GetSystemErrorMessageW(errorCode);
    outstream << " - " << " Error Code: " << errorCode << " (" << systemErrorMessage << ")";
  }
  return outstream.str();
}

void MessageLog::Log(LogLevel logLevel, const std::string& category, const std::string& message,
  DWORD errorCode)
{
  Log(logLevel, category, ConvertUTF8StringToUTF16String(message), errorCode);
}

void MessageLog::Log(LogLevel logLevel, const std::string& category, const std::wstring& message,
  DWORD errorCode)
{
  // Filter the message
  if (filter_.find(logLevel) == filter_.end())
  {
    return;
  }

  const auto logMessage = CreateLogMessage(logLevel, ConvertUTF8StringToUTF16String(category), message, errorCode);
  if (started_ && outFile_.is_open())
  {
    outFile_ << logMessage << std::endl;
    outFile_.flush();
  }

  // Always print to debug console
  std::wstring debugOutput = L"OCAT: " + logMessage + L"\n";
  OutputDebugString(debugOutput.c_str());
}

typedef void(WINAPI* FGETSYSTEMINFO)(LPSYSTEM_INFO);
typedef BOOL(WINAPI* FGETPRODUCTINFO)(DWORD, DWORD, DWORD, DWORD, PDWORD);
void MessageLog::LogOS()
{
  SYSTEM_INFO systemInfo = {};
  FGETSYSTEMINFO fGetSystemInfo = reinterpret_cast<FGETSYSTEMINFO>(
    GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo"));
  if (fGetSystemInfo)
  {
    fGetSystemInfo(&systemInfo);
  }
  else {
    LogWarning("MessageLog", "LogOS: Unable to get address for GetNativeSystemInfo using GetSystemInfo");
    GetSystemInfo(&systemInfo);
  }

  std::string processorArchitecture = "Processor architecture: ";
  switch (systemInfo.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_AMD64:
    processorArchitecture += "x64";
    break;
  case PROCESSOR_ARCHITECTURE_INTEL:
    processorArchitecture += "x86";
    break;
  case PROCESSOR_ARCHITECTURE_ARM:
    processorArchitecture += "ARM";
    break;
  case PROCESSOR_ARCHITECTURE_IA64:
    processorArchitecture += "Intel Itanium";
    break;
  default:
    processorArchitecture += "Unknown";
    break;
  }

  FGETPRODUCTINFO fGetProductInfo = reinterpret_cast<FGETPRODUCTINFO>(
    GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetProductInfo"));
  if (!fGetProductInfo)
  {
    LogError("MessageLog", "LogOS: Unable to get address for GetProductInfo");
    return;
  }

  DWORD type = 0;
  fGetProductInfo(6, 0, 0, 0, &type);
  const std::string osInfo = "OS: type: " + std::to_string(type);

  LogInfo("MessageLog", osInfo + " " + processorArchitecture);
}

void MessageLog::SetCurrentTime()
{
  const std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  localtime_s(&currentTime_, &time);
}
