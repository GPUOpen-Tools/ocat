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

#include <Windows.h>
#include <codecvt>

#include "Config.h"
#include "..\Utility\IniParser.h"
#include "..\Utility\FileUtils.h"
#include "..\Logging\MessageLog.h"
#include "..\Utility\ProcessHelper.h"

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
		json_providers = j["provider"].get < std::vector<nlohmann::json> >();

		for (auto& json_provider : json_providers) {
			Provider p;
			p.name = json_provider["name"].get<std::string>();

			std::string uuid = json_provider["guid"].get<std::string>();
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::wstring w_uuid = converter.from_bytes(uuid);

			// convert GUID
			if (CLSIDFromString(w_uuid.c_str(), (LPCLSID)&p.guid) != ERROR_SUCCESS) {
				g_messageLog.LogError("Config", "Could not convert string to GUID - skip provider");
				continue;
			}

			p.handler = json_provider["handler"].get<std::string>();
			p.eventIDs = json_provider["events"].get<std::vector<USHORT>>();
			p.traceLevel = json_provider["trace-level"].get<int>();
			p.matchAnyKeyword = json_provider["match-any-keyword"].get<uint64_t>();
			p.matchAllKeyword = json_provider["match-all-keyword"].get<uint64_t>();
			provider.push_back(p);
		}

		return true;
	}
	else {
		g_messageLog.LogWarning("Config",
			"Unable to open capture config file. No providers loaded.");
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
				{ "name", "steamVR" },
				{ "guid", "{8C8F13B1-60EB-4B6A-A433-DE86104115AC}" },
				{ "handler", "HandleSteamVREvent" },
				{ "events", nlohmann::json::array() },
				{ "trace-level", 4 },
				{ "match-any-keyword", 0 },
				{ "match-all-keyword", 0 }
			}
		}
		}
	};

	std::ofstream file(fileName);
	if (file.is_open())
	{
		file << std::setw(4) << configJson << std::endl;
	}
}