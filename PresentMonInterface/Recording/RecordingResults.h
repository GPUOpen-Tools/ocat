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

#pragma once

#include <map>
#include "..\Config\BlackList.h"
#include "SimpleResults.h"

class RecordingResults {
 public:
  struct Result {
    FILE* outputFile = nullptr;
    SimpleResults simpleResults;
    bool created = false;
    void CreateRecordingFile(const std::string& processName, const std::string& outputDir);
    std::string CreateOutputPath(const std::string& processName, const std::string& outputDir);
  };

  void Init(const std::string& outputDir);
  Result& OnPresent(uint32_t processID, const std::string& processName);
  void OnShutdown(bool logCorrupted);

 private:
  std::map<uint32_t, Result> recordingResults_;
  std::string outputDir_;
  BlackList blackList_;
};