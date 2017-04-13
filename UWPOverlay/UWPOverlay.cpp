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

#include "UWPOverlay.h"

#include <appmodel.h>
#include <vector>

#include "Constants.h"
#include "DLLPermissions.h"
#include "MessageLog.h"
#include "ProcessHelper.h"
#include "FileDirectory.h"

const std::wstring UWPOverlay::uwpDebugExe_ = L"UWPDebug.exe";

DWORD UWPOverlay::StartProcess(const wchar_t* path)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "UWPOverlay", L"Trying to start uwp executable" + std::wstring(path));

  const auto packageID = GetPackageIdFromPath(path);
  const auto appId = GetAppIdFromPackageId(packageID);

  DLLPermissions dllPermissions;
  dllPermissions.SetDLLPermissions(g_libraryName64);
  dllPermissions.SetDLLPermissions(g_libraryName32);

  g_messageLog.Log(MessageLog::LOG_INFO, "UWPOverlay", "enable debugging");
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  // stop if failed and not already called
  if (hr != S_OK && hr != S_FALSE) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "CoInitializeEx failed HRESULT ", hr);
    return false;
  }

  CComQIPtr<IPackageDebugSettings> debugSettings;
  // enable debug settings to suspend the uwp application
  hr = debugSettings.CoCreateInstance(CLSID_PackageDebugSettings, NULL, CLSCTX_ALL);
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay",
      "Failed creting debug settings instance HRESULT", hr);
    return false;
  }

  EnableDebugging(packageID, debugSettings);

  const auto pId = StartUWPProcess(appId);
  if (!pId) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed starting uwp executable");
  }

  DisableDebugging(packageID, debugSettings);

  return pId;
}

DWORD UWPOverlay::StartUWPProcess(const std::wstring& appId)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "UWPOverlay", "starting process");
  CComPtr<IApplicationActivationManager> appActivationManager;

  HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, NULL, CLSCTX_LOCAL_SERVER,
                                IID_IApplicationActivationManager,
                                reinterpret_cast<LPVOID*>(&appActivationManager));
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay",
                     "Failed creating app activation manager instance HRESULT ", hr);
    return 0;
  }

  hr = CoAllowSetForegroundWindow(appActivationManager, NULL);
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed setting to foreground HRESULT ",
                     hr);
    return 0;
  }

  DWORD processId = 0;
  hr = appActivationManager->ActivateApplication(appId.c_str(), NULL, AO_NONE, &processId);
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed activating Application HRESULT ",
                     hr);
    return 0;
  }

  return processId;
}

bool UWPOverlay::EnableDebugging(const std::wstring& packageID, IPackageDebugSettings* debugSettings)
{
  // remove last \ because otherwise the process id is not attached correctly
  auto directory = GetCurrentProcessDirectory();
  directory.pop_back();
  // need to start this with an executable that starts the dll injection and resumes the thread
  // passes -d as the lib directory because the exe process is started in a different directory
  const auto debuggerCmdLine =
      L"\"" + GetAbsolutePath(g_fileDirectory.GetDirectoryW(FileDirectory::DIR_BIN) + uwpDebugExe_) + L"\" -d \"" + directory + L"\"";
  HRESULT hr = debugSettings->EnableDebugging(packageID.c_str(), debuggerCmdLine.c_str(), NULL);
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed enabling debugging HRESULT", hr);
    return false;
  }

  return true;
}

void UWPOverlay::DisableDebugging(const std::wstring& packageID, IPackageDebugSettings* debugSettings)
{
  HRESULT hr = debugSettings->DisableDebugging(packageID.c_str());
  if (hr != S_OK) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "UWPOverlay", "Failed disabling debugging HRESULT",
                     hr);
  }
}

std::wstring UWPOverlay::GetPackageIdFromPath(const std::wstring& input)
{
  const auto directory = GetDirFromPathSlashes(input);
  const auto directoryEnd = directory.size() - 2;
  auto folderStart = directory.rfind('\\', directoryEnd);
  if (folderStart == std::string::npos) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "GetPackageIdFromPath failed");
    return L"";
  }

  return directory.substr(folderStart + 1, directoryEnd - folderStart);
}

std::wstring UWPOverlay::GetAppIdFromPackageId(const std::wstring& packageId)
{
  PACKAGE_INFO_REFERENCE packageInfo;
  auto rc = OpenPackageInfoByFullName(packageId.c_str(), 0, &packageInfo);
  if (rc != ERROR_SUCCESS) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed open package info");
    return L"";
  }

  UINT32 bufferLength = 0;
  UINT32 appIDCount = 0;
  rc = GetPackageApplicationIds(packageInfo, &bufferLength, nullptr, &appIDCount);
  // query the required size, should always return ERROR_INSUFFICIENT_BUFFER
  if (rc != ERROR_INSUFFICIENT_BUFFER) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed to query app id count");
    ClosePackageInfo(packageInfo);
    return L"";
  }

  std::vector<BYTE> buffer(bufferLength);

  rc = GetPackageApplicationIds(packageInfo, &bufferLength, buffer.data(), &appIDCount);
  if (rc != ERROR_SUCCESS) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "UWPOverlay", "Failed getting app ids");
    ClosePackageInfo(packageInfo);
    return L"";
  }

  PCWSTR* applicationUserModelIds = reinterpret_cast<PCWSTR*>(buffer.data());
  if (appIDCount != 1) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "UWPOverlay", "multiple appIds, using first Id");
  }

  const std::wstring appId = applicationUserModelIds[0];
  ClosePackageInfo(packageInfo);

  return appId;
}