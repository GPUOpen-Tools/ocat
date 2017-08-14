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


#include "FileDirectory.h"
#include <ShlObj.h>

#include "..\Utility\ProcessHelper.h"
#include "..\Utility\Constants.h"
#include "..\Logging\MessageLog.h"
#include "..\Utility\StringUtils.h"
#include "..\Utility\FileUtils.h"

FileDirectory g_fileDirectory;

FileDirectory::FileDirectory() : initialized_(false)
{
  folders_.emplace(DIR_DOCUMENTS, L"OCAT\\");
  folders_.emplace(DIR_LOG, L"Logs\\");
  folders_.emplace(DIR_CONFIG, L"Config\\");
  folders_.emplace(DIR_RECORDING, L"Recordings\\");
  folders_.emplace(DIR_BIN, L"Bin\\");
}

FileDirectory::~FileDirectory()
{
}

bool FileDirectory::Initialize()
{
  if (initialized_)
  {
    return true;
  }

  if (!FindBinaryDir())
  {
    return false;
  }

  if (!FindDocumentsDir())
  {
    return false;
  }

  if (!CreateDir(directories_[DIR_DOCUMENTS].dirW, DIR_DOCUMENTS))
  {
    return false;
  }

  std::vector<DirectoryType> documentDirectories = { DIR_LOG, DIR_CONFIG, DIR_RECORDING };
  for (auto type : documentDirectories)
  {
    bool success = CreateDir(directories_[DIR_DOCUMENTS].dirW + folders_[type].dirW, type);
    if (!success)
    {
      return false;
    }
    LogFileDirectory(directories_[type].dirW, folders_[type].dirW);
  }

  initialized_ = true;
  return true;
}

const std::wstring& FileDirectory::GetDirectoryW(DirectoryType type)
{
  if (!initialized_)
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", "Use of uninitialized file directory.");
    throw std::runtime_error("Use of uninitialized file directory.");
  }
  return directories_[type].dirW;
}

const std::string& FileDirectory::GetDirectory(DirectoryType type)
{
  if (!initialized_)
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", "Use of uninitialized file directory.");
    throw std::runtime_error("Use of uninitialized file directory.");
  }
  return directories_[type].dir;
}

const std::wstring& FileDirectory::GetFolderW(DirectoryType type)
{
  return folders_[type].dirW;
}

const std::string& FileDirectory::GetFolder(DirectoryType type)
{
  return folders_[type].dir;
}

FileDirectory::Directory::Directory()
{
  // Do nothing.
}

FileDirectory::Directory::Directory(const std::wstring& directory)
{
  dirW = directory;
  dir = ConvertUTF16StringToUTF8String(dirW);
}

bool FileDirectory::FindDocumentsDir()
{
  PWSTR docDir = nullptr;
  const auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &docDir);
  if (FAILED(hr))
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Unable to find Documents directory");
    return false;
  }

  directories_[DIR_DOCUMENTS] = Directory(std::wstring(docDir) + L"\\" + folders_[DIR_DOCUMENTS].dirW);
  LogFileDirectory(directories_[DIR_DOCUMENTS].dirW, folders_[DIR_DOCUMENTS].dirW);
  CoTaskMemFree(docDir);
  return true;
}

bool FileDirectory::FindBinaryDir()
{
  HKEY registryKey;
  LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\AMD\\OCAT\\", 0, KEY_READ | KEY_WOW64_64KEY, &registryKey);
  if (result != ERROR_SUCCESS)
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Failed to open OCAT registry key", result);
    return false;
  }

  std::wstring ocatExecutableDirectory;
  result = GetStringRegKey(registryKey, L"InstallDir", ocatExecutableDirectory, L"");
  RegCloseKey(registryKey);
  if (result != ERROR_SUCCESS)
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Failed to retrieve binary directory", result);
    return false;
  }

  directories_[DIR_BIN] = Directory(ocatExecutableDirectory + folders_[DIR_BIN].dirW);
  LogFileDirectory(directories_[DIR_BIN].dirW, folders_[DIR_BIN].dirW);
  return true;
}

void FileDirectory::LogFileDirectory(const std::wstring& value, const std::wstring& message)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "FileDirectory", message + L"\t" + value);
}

bool FileDirectory::CreateDir(const std::wstring& dir, DirectoryType type)
{
  const auto result = CreateDirectory(dir.c_str(), NULL);
  if (!result)
  {
    const auto error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS)
    {
      g_messageLog.Log(MessageLog::LOG_DEBUG, "FileDirectory", L"Unable to create directory " + dir, error);
      return false;
    }
  }
  directories_[type] = Directory(dir);
  return true;
}