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

#include "Overlay.h"

#include <assert.h>
#include <climits>
#include <cstdio>
#include <vector>

#include "../Utility/Constants.h"
#include "DLLInjection.h"
#include "../Utility/FileDirectory.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "../Utility/FileUtils.h"

#include "VK_Environment.h"

Overlay::Overlay()
{
  // Empty
}

typedef LONG(*PStartUWPProcessWithOverlay) (const wchar_t*);

bool Overlay::StartProcess(const std::wstring& path, std::wstring& cmdArgs, bool enableOverlay)
{
  const auto processStartedStatus = CreateDesktopProcess(path, cmdArgs, enableOverlay);
  // UWP app detected
  if (processStartedStatus == ERROR_APPCONTAINER_REQUIRED)
  {
    const std::wstring uwpDllPath = g_fileDirectory.GetDirectory(DirectoryType::Bin) + L"UWPOverlay.dll";
    auto uwpDll = LoadLibrary(uwpDllPath.c_str());
    if (uwpDll)
    {
      PStartUWPProcessWithOverlay startUWPProcessWithOverlay = reinterpret_cast<PStartUWPProcessWithOverlay>(GetProcAddress(uwpDll, "StartUWPProcessWithOverlay"));
      if (startUWPProcessWithOverlay != NULL)
      {
        const auto processID = startUWPProcessWithOverlay(path.c_str());
        FreeLibrary(uwpDll);
        SetProcessInfo(processID);
        if (processID) {
          return true;
        }
      }
      const std::wstring message = L"GetProcAddress failed " + std::to_wstring(GetLastError());
      MessageBox(NULL, message.c_str(), NULL, MB_OK);
    }
    else
    {
      const std::wstring message = L"GetModuleHandle failed " + uwpDllPath + L" " + std::to_wstring(GetLastError());
      MessageBox(NULL, message.c_str(), NULL, MB_OK);
    }
    return false;
  }

  // normal process created
  else if (processStartedStatus == ERROR_SUCCESS)
  {
    if (enableOverlay)
    {
      InjectDLL(processInfo_.dwProcessId, g_fileDirectory.GetDirectory(DirectoryType::Bin));
    }
    ResumeThread(processInfo_.hThread);
    processName_ = GetProcessNameFromHandle(processInfo_.hProcess);
  }
  else
  {
    g_messageLog.LogError("Overlay",
      "Unable to start process", processStartedStatus);
    return false;
  }

  processTermination_.Register(processID_);

  return true;
}

void Overlay::SetProcessInfo(DWORD id)
{
  processID_ = id;
  processName_ = GetProcessNameFromID(id);
}

const DWORD Overlay::GetProcessID() const
{
  return processID_;
}

DWORD Overlay::CreateDesktopProcess(const std::wstring& path, std::wstring& cmdArgs, bool enableOverlay)
{
  g_messageLog.LogInfo("Overlay",
    L"Trying to start executable " + path + L" cmdArgs " + cmdArgs);
  const auto directory = GetDirFromPathSlashes(path);
  if (directory.empty()) return ERROR_PATH_NOT_FOUND;

  VK_Environment environment;
  if (enableOverlay)
  {
	  environment.SetVKEnvironment(g_fileDirectory.GetDirectory(DirectoryType::Bin));
  }

  std::wstring commandLine = L"\"" + path + L"\" " + cmdArgs;
  STARTUPINFO startupInfo{};
  startupInfo.cb = sizeof(startupInfo);

  if (!CreateProcess(NULL, &commandLine[0], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL,
    directory.c_str(), &startupInfo, &processInfo_))
  {
    return GetLastError();
  }

  environment.ResetVKEnvironment();
  SetProcessInfo(processInfo_.dwProcessId);
  return ERROR_SUCCESS;
}

void Overlay::Stop()
{
  processTermination_.UnRegister();
}
