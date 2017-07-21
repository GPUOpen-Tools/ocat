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

FileDirectory g_fileDirectory;

FileDirectory::FileDirectory() :
  folders_{L"OCAT\\", L"Logs\\", L"Config\\", L"Recordings\\", L"Bin\\"}
{
}

FileDirectory::~FileDirectory()
{
}

bool FileDirectory::CreateDirectories()
{
  if (!FindDocumentDir())
    return false;

  if (!CreateDir(directories_[DIR_DOCUMENTS].dirW, DIR_DOCUMENTS))
    return false;

  for (int type = DIR_LOG; type <= DIR_RECORDING; ++type)
  {
    if (!CreateDir(directories_[DIR_DOCUMENTS].dirW + folders_[type].dirW, static_cast<Type>(type)))
      return false;
  }

  return true;
}

const std::wstring& FileDirectory::GetDirectoryW(Type type)
{
  auto& dir = directories_[type];
  if (!dir.Initialized())
  {
    SetDir(type);
  }

  return dir.dirW;
}

const std::string& FileDirectory::GetDirectory(Type type)
{
  auto& dir = directories_[type];
  if (!dir.Initialized())
  {
    SetDir(type);
  }

  return dir.dir;
}

const std::wstring& FileDirectory::GetFolderW(Type type)
{
  return folders_[type].dirW;
}

const std::string& FileDirectory::GetFolder(Type type)
{
  return folders_[type].dir;
}

FileDirectory::Directory::Directory(const std::wstring& directory)
{
  dirW = directory;
  dir = ConvertUTF16StringToUTF8String(dirW);
}

bool FileDirectory::FindDocumentDir()
{
  if (directories_[DIR_DOCUMENTS].Initialized())
  {
    return true;
  }

  PWSTR docDir = nullptr;
  const auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &docDir);
  if (FAILED(hr))
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Unable to Get Documents folder");
    return false;
  }

  directories_[DIR_DOCUMENTS] = Directory(std::wstring(docDir) + L"\\" + folders_[DIR_DOCUMENTS].dirW);
  PrintValue(directories_[DIR_DOCUMENTS].dirW, folders_[DIR_DOCUMENTS].dirW);
  CoTaskMemFree(docDir);
  return true;
}

bool FileDirectory::FindBinaryDir()
{
#if _WIN64
  const auto libraryName = g_libraryName64;
#else
  const auto libraryName = g_libraryName32;
#endif
  
  const auto module = GetModuleHandle(libraryName.c_str());

  if (!module)
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "FileDirectory", 
      L"Get module handle failed (" + libraryName + L")", GetLastError());
    const auto processDir = GetCurrentProcessDirectory();
    if (processDir.empty())
      return false;

    directories_[DIR_BIN] = Directory(processDir + folders_[DIR_BIN].dirW);
    PrintValue(directories_[DIR_BIN].dirW, folders_[DIR_BIN].dirW);
  }
  else
  {
    std::wstring buffer;
    DWORD stringSize = 0;
    DWORD bufferSize = MAX_PATH;
    do {
      buffer.resize(bufferSize);
      stringSize = GetModuleFileName(module, &buffer[0], bufferSize);
      if (stringSize == 0) {
        g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Get Module File Name failed ", GetLastError());
        return false;
      }
      bufferSize *= 2;
    } while (buffer.size() == stringSize);

    // stringSize is excluding the null terminator
    buffer.resize(stringSize);

    const auto pathEnd = buffer.find_last_of('\\');
    if (pathEnd == std::string::npos) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "FileDirectory", L"Failed finding end of path ");
      return L"";
    }

    directories_[DIR_BIN] = Directory(buffer.substr(0, pathEnd + 1));
    PrintValue(directories_[DIR_BIN].dirW, L"BIN");
  }

  return true;
}

void FileDirectory::PrintValue(const std::wstring& value, const std::wstring& message)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "FileDirectory", message + L"\t" + value);
}

bool FileDirectory::CreateDir(const std::wstring& dir, Type type)
{
  directories_[type] = Directory(dir);
  const auto result = CreateDirectory(dir.c_str(), NULL);
  if (!result)
  {
    const auto error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS)
    {
      g_messageLog.Log(MessageLog::LOG_DEBUG, "FileDirectory", L"Unable to Create folder " + dir);
      return false;
    }
  }
  return true;
}

void FileDirectory::SetDir(Type type)
{
  switch (type)
  {
  case DIR_DOCUMENTS:
    FindDocumentDir();
    break;
  case DIR_CONFIG:
  case DIR_LOG:
  case DIR_RECORDING:
    if (!directories_[DIR_DOCUMENTS].Initialized())
      FindDocumentDir();
    directories_[type] = Directory(directories_[DIR_DOCUMENTS].dirW + folders_[type].dirW);
    PrintValue(directories_[type].dirW, folders_[type].dirW);
    break;
  case DIR_BIN:
    FindBinaryDir();
    break;
  default:
    break;
  }
}