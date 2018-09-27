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

#include "Config.h"
#include "Utility/IniParser.h"

#include "Logging/MessageLog.h"

static std::wstring g_iniFile = L"settings.ini";

bool Config::Load(const std::wstring& path)
{
  g_messageLog.LogInfo("Config", "Loading config");
  const auto fileName = (path + g_iniFile);
  if (IsFileAccessible(fileName)) {
    hotkey_ = GetPrivateProfileInt(L"Recording", L"hotkey", hotkey_, fileName.c_str());
    recordingTime_ =
        ReadFloatFromIni(L"Recording", L"recordTime", recordingTime_, fileName.c_str());

    return true;
  }
  else {
    g_messageLog.LogWarning("Config",
                     "Unable to open config file. Using default values");
    return false;
  }
}
