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

#include "SimpleResults.h"

#include <algorithm>
#include <fstream>

constexpr float percentile = 0.99f;
static const std::string g_fileName = "perf_summary.csv";

SimpleResults::SimpleResults() {}
SimpleResults::~SimpleResults() {}
void SimpleResults::Reset(const std::string& date, const std::string& processName)
{
  date_ = date;
  processName_ = processName;
  frameTimes_.clear();
  frameCount_ = 0;
}

void SimpleResults::AddPresent(double deltaMS, double totalTime)
{
  frameCount_++;
  frameTimes_.push_back(deltaMS);
  totalTimeSeconds_ = totalTime;
}

void SimpleResults::Save(const std::string& outputDir)
{
  double avgFPS = 0.0;
  double avgFrameTime = 0.0;
  double frameTimePercentile = 0.0;

  const std::string outputPath = outputDir + g_fileName;

  if (!frameTimes_.empty()) {
    avgFPS = frameCount_ / totalTimeSeconds_;
    avgFrameTime = (totalTimeSeconds_ * 1000.0) / frameCount_;

    std::sort(frameTimes_.begin(), frameTimes_.end(), std::less<double>());
    const auto rank = static_cast<int>(percentile * frameTimes_.size());
    frameTimePercentile = frameTimes_[rank];
  }

  WriteHeading(outputPath);

  FILE* file;
  fopen_s(&file, outputPath.c_str(), "a");
  if (file) {
    fprintf(file, "%s,%s,%.1f,%.1f,%.1f\n", processName_.c_str(), date_.c_str(), avgFPS,
            avgFrameTime, frameTimePercentile);
    fclose(file);
  }
}

bool SimpleResults::FileExists(const std::string& outputPath)
{
  std::ifstream inFile(outputPath.c_str());
  return inFile.good();
}

void SimpleResults::WriteHeading(const std::string& outputPath)
{
  if (!FileExists(outputPath)) {
    FILE* file;
    fopen_s(&file, outputPath.c_str(), "w");
    if (file) {
      fprintf(file,
              "Application Name,Date and Time,Average FPS,Average frame time (ms),99th-percentile "
              "frame time (ms)\n");
    }
    fclose(file);
  }
}