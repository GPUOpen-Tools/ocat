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

#include "DenyList.h"
#include "../Utility/ProcessHelper.h"
#include "../Utility/FileDirectory.h"
#include "../Logging/MessageLog.h"
#include "../Utility/StringUtils.h"

#include <fstream>

void DenyList::Load()
{
  if (loaded_) return;

  const std::wstring defaultDenyList =
    g_fileDirectory.GetDirectory(DirectoryType::Config) + L"defaultDenyList.txt";

  std::wifstream file(defaultDenyList);
  if (file.is_open())
  {
    // check for version number
    // if we know app version and it differs from default denylist version -> update
    std::wstring version;
    std::getline(file, version);
    if (version_.empty() || ConvertUTF16StringToUTF8String(version).compare(version_) == 0)
    {
      // at this point the correct version number should be the one parsed from the denyList file
      version_ = ConvertUTF16StringToUTF8String(version);
      for (std::wstring line; std::getline(file, line);)
      {
        denyList_.push_back(line);
      }
      g_messageLog.LogInfo("DenyList", "Denylist file loaded");
    }
    else {
      file.close();
      CreateDefault(defaultDenyList);
    }
  }
  else
  {
    CreateDefault(defaultDenyList);
  }

  const std::wstring userDenyList =
    g_fileDirectory.GetDirectory(DirectoryType::Config) + L"userDenyList.txt";
  std::wifstream userfile(userDenyList);
  if (userfile.is_open())
  {
    for (std::wstring line; std::getline(userfile, line);)
    {
      denyList_.push_back(line);
    }
    g_messageLog.LogInfo("DenyList", "Denylist file loaded");
  }
  else {
    CreateUserDenyList(userDenyList);
  }

  loaded_ = true;
}

bool DenyList::Contains(const std::wstring& value) const
{
  // ignore system processes without names
  if (value.empty()) {
    return true;
  }

  for (auto& entry : denyList_) {
    if (_wcsicmp(entry.c_str(), value.c_str()) == 0) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> DenyList::GetDenyList()
{
  std::vector<std::string> denyList;
  denyList.reserve(denyList_.size());
  for (const auto& item : denyList_)
  {
    denyList.push_back(ConvertUTF16StringToUTF8String(item));
  }
  return denyList;
}

void DenyList::CreateDefault(const std::wstring& fileName)
{
  g_messageLog.LogInfo("DenyList", "Create default denyList file");

  /* TODO: properly separate and document this list */
  denyList_ = {ConvertUTF8StringToUTF16String(version_),
      L"dwm.exe", L"explorer.exe", L"firefox.exe",
      L"chrome.exe", L"taskhostw.exe", L"notepad.exe", 
      L"RadeonSettings.exe", L"Nvidia Share.exe", L"devenv.exe", L"Outlook.exe", L"Excel.exe",
      L"OCAT.exe", L"GlobalHook64.exe", L"GlobalHook32.exe",
      L"Steam.exe", L"steamwebhelper.exe", L"vrcompositor.exe", L"steamtours.exe",
      L"upc.exe", L"Uplay.exe", L"UplayWebCore.exe", L"UbisoftGameLauncher.exe",
      L"EpicGamesLauncher.exe", L"UnrealCESSubProcess.exe", L"Origin.exe",L"Battle.net.exe",
      L"GalaxyClient.exe", L"GalaxyClient Helper.exe", L"GOG Galaxy Notifications Renderer.exe",
      L"OculusClient.exe", L"IAStorIcon.exe", L"conhost.exe", L"Agent.exe", L"Slack.exe",
      L"Code.exe", L"powershell.exe", L"python.exe", L"conda.exe", L"wmic.exe", L"onenote.exe",
      L"SearchProtocolHost.exe", L"lync.exe", L"taskmgr.exe", L"teams.exe", L"SpeechRuntime.exe",
      L"ApplicationFrameHost.exe", L"LogonUI.exe", L"winword.exe" , L"powerpnt.exe", L"Launcher.exe",
      L"RadeonSoftware.exe"
      };

  std::wofstream file(fileName);
  if (file.is_open())
  {
    for (auto& value : denyList_)
    {
      file << value << std::endl;
    }
  }
  else
  {
    g_messageLog.LogError("DenyList", "Unable to create default denyList file");
  }
}

void DenyList::CreateUserDenyList(const std::wstring& fileName)
{
  g_messageLog.LogInfo("DenyList", "Create user denyList file");
  std::wofstream file(fileName);
  if (file.is_open())
  {
    file << L"app_to_be_ignored.exe" << std::endl;
  }
  else
  {
    g_messageLog.LogError("DenyList", "Unable to create user denyList file");
  }
}
