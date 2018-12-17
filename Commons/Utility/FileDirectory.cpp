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
#include <shlobj.h>

#include "../Utility/ProcessHelper.h"
#include "../Utility/Constants.h"
#include "../Logging/MessageLog.h"
#include "../Utility/StringUtils.h"
#include "../Utility/FileUtils.h"

FileDirectory g_fileDirectory;

FileDirectory::FileDirectory() : initialized_(false)
{
  folders_.emplace(DirectoryType::Documents, L"OCAT\\");
  folders_.emplace(DirectoryType::Log, L"Logs\\");
  folders_.emplace(DirectoryType::Config, L"Config\\");
  folders_.emplace(DirectoryType::Recording, L"Captures\\");
  folders_.emplace(DirectoryType::Bin, L"Bin\\");
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

  if (!CreateDir(directories_[DirectoryType::Documents].dirW, DirectoryType::Documents))
  {
    return false;
  }

  std::vector<DirectoryType> documentDirectories = { DirectoryType::Log, DirectoryType::Config, DirectoryType::Recording };
  for (const auto& type : documentDirectories)
  {
    bool success = CreateDir(directories_[DirectoryType::Documents].dirW + folders_[type].dirW, type);
    if (!success)
    {
      return false;
    }
    LogFileDirectory(directories_[type].dirW, folders_[type].dirW);
  }

  initialized_ = true;
  return true;
}

const std::wstring& FileDirectory::GetDirectory(DirectoryType type)
{
  if (!initialized_)
  {
    g_messageLog.LogError("FileDirectory", "Use of uninitialized file directory.");
    throw std::runtime_error("Use of uninitialized file directory.");
  }
  return directories_[type].dirW;
}

const std::wstring& FileDirectory::GetFolder(DirectoryType type)
{
  return folders_[type].dirW;
}

FileDirectory::Directory::Directory()
{
  // Do nothing.
}

FileDirectory::Directory::Directory(const std::wstring& directory)
  : dirW(directory)
{
  // Empty
}

bool FileDirectory::FindDocumentsDir()
{
  PWSTR docDir = nullptr;
  const auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &docDir);
  if (FAILED(hr))
  {
    g_messageLog.LogError("FileDirectory", L"Unable to find Documents directory");
    return false;
  }

  directories_[DirectoryType::Documents] = Directory(std::wstring(docDir) + L"\\" + folders_[DirectoryType::Documents].dirW);
  LogFileDirectory(directories_[DirectoryType::Documents].dirW, folders_[DirectoryType::Documents].dirW);
  CoTaskMemFree(docDir);
  return true;
}


bool FileDirectory::SetBinaryDirFromRegistryKey(HKEY registryKey)
{
  std::wstring ocatExecutableDirectory;
  LONG result = GetStringRegKey(registryKey, L"InstallDir", ocatExecutableDirectory, L"");
  if (result != ERROR_SUCCESS)
  {
    g_messageLog.LogError("FileDirectory", L"Failed to retrieve binary directory", result);
    return false;
  }

  directories_[DirectoryType::Bin] = Directory(ocatExecutableDirectory + folders_[DirectoryType::Bin].dirW);
  LogFileDirectory(directories_[DirectoryType::Bin].dirW, folders_[DirectoryType::Bin].dirW);
  return true;
}

bool FileDirectory::FindBinaryDir()
{
  HKEY registryKey;
  // Try to open 32bit registry key on local machine.
  LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OCAT\\", 0, KEY_READ | KEY_WOW64_32KEY, &registryKey);
  if (result != ERROR_SUCCESS)
  {
    // Try to open 64bit registry key on local machine.
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OCAT\\", 0, KEY_READ | KEY_WOW64_64KEY, &registryKey);
    if (result != ERROR_SUCCESS)
    {
      // Try to open 32bit registry key on current user.
      result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\OCAT\\", 0, KEY_READ | KEY_WOW64_32KEY, &registryKey);
      if (result != ERROR_SUCCESS)
      {
        // Try to open 64bit registry key on current user.
        result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\OCAT\\", 0, KEY_READ | KEY_WOW64_64KEY, &registryKey);
        if (result != ERROR_SUCCESS)
        {
          g_messageLog.LogError("FileDirectory", L"Failed to open OCAT registry key", result);
          return false;
        }
      }
    }
  }

  bool success = SetBinaryDirFromRegistryKey(registryKey);
  RegCloseKey(registryKey);
  return success;
}

void FileDirectory::LogFileDirectory(const std::wstring& value, const std::wstring& message)
{
  g_messageLog.LogInfo("FileDirectory", message + L"\t" + value);
}

bool FileDirectory::CreateDir(const std::wstring& dir, DirectoryType type)
{
  const auto result = CreateDirectory(dir.c_str(), NULL);
  if (!result)
  {
    const auto error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS)
    {
      g_messageLog.LogVerbose("FileDirectory", L"Unable to create directory " + dir, error);
      return false;
    }
  }
  directories_[type] = Directory(dir);
  return true;
}
