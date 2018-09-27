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

#include <windows.h>
#include <codecvt>

#include "Config.h"
#include "../Utility/IniParser.h"
#include "../Utility/FileUtils.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"

#include "json.hpp"

static std::wstring g_iniFile = L"settings.ini";
static std::wstring g_captureConfigFile = L"capture-config.json";

bool Config::Load(const std::wstring& path)
{
  const auto fileName = (path + g_iniFile);

  if (FileExists(fileName)) {
    hotkey_ = GetPrivateProfileInt(L"Recording", L"hotkey", hotkey_, fileName.c_str());
    toggleOverlayHotKey_ = GetPrivateProfileInt(L"Recording", L"toggleOverlayHotkey", toggleOverlayHotKey_, fileName.c_str());
    recordingTime_ = GetPrivateProfileInt(L"Recording", L"recordTime", recordingTime_, fileName.c_str());
    recordAllProcesses_ = ReadBoolFromIni(L"Recording", L"recordAllProcesses", recordAllProcesses_, fileName.c_str());
    overlayPosition_ = GetPrivateProfileInt(L"Recording", L"overlayPosition", overlayPosition_, fileName.c_str());

    g_messageLog.LogInfo("Config", "file loaded");
    return true;
  }
  else {
    g_messageLog.LogWarning("Config",
                     "Unable to open config file. Using default values");
    return false;
  }
}

template<typename T> bool ReadJObject(const nlohmann::json json, std::string key, T& dst) {
  try {
    dst = json.at(key).get<T>();
    return true;
  }
  catch (const std::exception&) {	}

  return false;
}

bool ConfigCapture::Load(const std::wstring& path)
{
  const auto fileName = (path + g_captureConfigFile);

  if (!FileExists(fileName)) {
    CreateDefault(fileName);
  }

  if (FileExists(fileName)) {
    nlohmann::json j;
    std::ifstream i(fileName);
    i >> j;
    std::vector<nlohmann::json> json_providers;
    ReadJObject<std::vector<nlohmann::json>>(j, "provider", json_providers);

    for (const auto& json_provider : json_providers) {
      std::string providerTag;
      std::string verbosity;
      ProviderConfig config;
      // valid names: "SteamVR", "OculusVR", "WMR", "DXGI"
      ReadJObject<std::string>(json_provider, "name", providerTag);
      ReadJObject<std::string>(json_provider, "recording-detail", verbosity);
      // map string to verbosity types, if invalid string keep default
      // valid strings: "Simple", "Normal", "Verbose"
      if (verbosity == "Simple") {
        config.recordingDetail = Verbosity::Simple;
      }
      else if (verbosity == "Normal") {
        config.recordingDetail = Verbosity::Normal;
      }
      else if (verbosity == "Verbose") {
        config.recordingDetail = Verbosity::Verbose;
      }

      ReadJObject<bool>(json_provider, "enabled", config.enabled);
      provider.emplace(std::pair<std::string, ProviderConfig>(providerTag, config));
    }

    return true;
  }
  else {
    g_messageLog.LogWarning("Config",
      "Unable to open capture config file. Enable all providers according to recording detail.");
    return false;
  }
}

void ConfigCapture::CreateDefault(const std::wstring& fileName)
{
  g_messageLog.LogInfo("Capture Config", "Create default capture config file");

  nlohmann::json configJson = {
    { "provider",
    {
      {
        { "name", "SteamVR" },
        { "enabled", true },
        { "recording-detail", "Default"}
      },
      {
        { "name", "OculusVR" },
        { "enabled", true },
        { "recording-detail", "Default" }
      },
      {
        { "name", "WMR" },
        { "enabled", true },
        { "recording-detail", "Default" }
      },
      {
        { "name", "DXGI" },
        { "enabled", true },
        { "recording-detail", "Default" }
      }
    }
    }
  };

  std::ofstream file(fileName);
  if (file.is_open())
  {
    file << std::setw(4) << configJson << std::endl;
  }
  file.close();
}
