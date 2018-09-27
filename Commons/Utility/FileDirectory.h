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
#pragma once

#include "DirectoryType.h"

#include <string>
#include <unordered_map>
#include <windows.h>

class FileDirectory
{
public:
  FileDirectory();
  ~FileDirectory();

  // This method has to be called, before using the directory. 
  // Don't proceed, if this method returns false, as the file directory will not be usable.
  bool Initialize();
  const std::wstring& GetDirectory(DirectoryType type);
  const std::wstring& GetFolder(DirectoryType type);

private:
  struct Directory
  {
    std::wstring dirW;
    Directory();
    Directory(const std::wstring& directory);
  };

  bool FindDocumentsDir();
  bool FindBinaryDir();
  bool SetBinaryDirFromRegistryKey(HKEY registryKey);
  bool CreateDir(const std::wstring& dir, DirectoryType type);
  void LogFileDirectory(const std::wstring& value, const std::wstring& message);

  bool initialized_;
  std::unordered_map<DirectoryType, Directory> directories_;
  std::unordered_map<DirectoryType, Directory> folders_;
};

extern FileDirectory g_fileDirectory;

