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

#include "RecordingResults.h"

#include <time.h>
#include "..\Logging\MessageLog.h"

void RecordingResults::Result::CreateRecordingFile(const std::string& processName,
                                                   const std::string& outputPath)
{
  const auto filePath = CreateOutputPath(processName, outputPath);

  fopen_s(&outputFile, filePath.c_str(), "w");
  if (!outputFile) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "RecordingResults",
                     "Unable to open recording file " + filePath);
    return;
  }
  g_messageLog.Log(MessageLog::LOG_INFO, "RecordingResults",
                   "Recording file created at " + filePath);

  fprintf(outputFile,
          "Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,AllowsTearing,"
          "PresentFlags,PresentMode,Dropped,TimeInSeconds,"
          "MsBetweenPresents,MsBetweenDisplayChange,MsInPresentAPI,MsUntilRenderComplete,"
          "MsUntilDisplayed\n");
  created = true;
}

std::string RecordingResults::Result::CreateOutputPath(const std::string& processName,
                                                       const std::string& outputPath)
{
  struct tm tm;
  time_t time_now = time(NULL);
  localtime_s(&tm, &time_now);
	char buffer[4096];
	_snprintf_s(buffer, _TRUNCATE, "%4d%02d%02d-%02d%02d%02d",  // ISO 8601
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	std::string date = std::string(buffer);
	
  simpleResults.Reset(date, processName);

	_snprintf_s(buffer, _TRUNCATE, "%sperf_%s_%s_RecordingResults.csv", outputPath.c_str(), processName.c_str(), date.c_str());
	return std::string(buffer);
}

void RecordingResults::Init(const std::string& outputDir)
{
  outputDir_ = outputDir;
  blackList_.Load();
}
RecordingResults::Result& RecordingResults::OnPresent(uint32_t processID,
                                                      const std::string& processName)
{
  auto it = recordingResults_.find(processID);
  if (it == recordingResults_.end() && !blackList_.Contains(processName)) {
    recordingResults_[processID] = Result();
    recordingResults_[processID].CreateRecordingFile(processName, outputDir_);
  }

  return recordingResults_[processID];
}

void RecordingResults::OnShutdown(bool logCorrupted)
{
  int savedFileCount = 0;
  for (auto& recordings : recordingResults_) {
    auto& result = recordings.second;
    if (result.created) {
      if (logCorrupted) {
        fprintf(result.outputFile,
                "Error: Some ETW packets were lost. Collected data is unreliable.\n");
      }
      fclose(result.outputFile);
      result.outputFile = nullptr;
      savedFileCount++;

      result.simpleResults.Save(outputDir_);
    }
  }

  g_messageLog.Log(MessageLog::LOG_INFO, "RecordingResults",
                   "Saved " + std::to_string(savedFileCount) + " recordings to " + outputDir_);
}