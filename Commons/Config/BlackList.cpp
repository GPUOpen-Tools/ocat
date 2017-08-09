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

#include "BlackList.h"
#include "..\Utility\ProcessHelper.h"
#include "..\Utility\FileDirectory.h"
#include "..\Logging\MessageLog.h"
#include "..\Utility\StringUtils.h"

#include <fstream>

void BlackList::Load()
{
  if (loaded_) return;

  const std::string blackListFile =
      g_fileDirectory.GetDirectory(FileDirectory::DIR_CONFIG) + "blackList.txt";
  std::ifstream file(blackListFile);
  if (file.is_open()) {
    for (std::string line; std::getline(file, line);) {
      blackList_.push_back(line);
    }
    g_messageLog.Log(MessageLog::LOG_INFO, "BlackList", "Blacklist file loaded");
  }
  else {
    CreateDefault(blackListFile);
  }

  loaded_ = true;
}

bool BlackList::Contains(const std::wstring& value) const
{
  const std::string v = ConvertUTF16StringToUTF8String(value);
  return Contains(v);
}

bool BlackList::Contains(const std::string& value) const
{
  // ignore system processes without names
  if (value.empty()) {
    return true;
  }

  for (auto& entry : blackList_) {
    if (_strcmpi(entry.c_str(), value.c_str()) == 0) {
      return true;
    }
  }
  return false;
}

void BlackList::CreateDefault(const std::string& fileName)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "BlackList", "Create default blacklist file");

  blackList_ = { "Steam.exe", "Origin.exe", "GalaxyClient.exe", "Battle.net.exe", 
    "OCAT.exe", "firefox.exe", "RadeonSettings.exe", "dwm.exe",
    // Uplay
    "upc.exe", "Uplay.exe", "UplayWebCore.exe", "UbisoftGameLauncher.exe" };

  std::ofstream file(fileName);
  if (file.is_open()) {
    for (auto& value : blackList_) {
      file << value << std::endl;
    }
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "BlackList", "Unable to create default blacklist file");
  }
}