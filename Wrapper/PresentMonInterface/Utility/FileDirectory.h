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

#include <string>
#include <array>

class FileDirectory
{
public:
  enum Type
  {
    DIR_DOCUMENTS,
    DIR_LOG,
    DIR_CONFIG,
    DIR_RECORDING,
    DIR_BIN,
    DIR_MAX
  };

  FileDirectory();
  ~FileDirectory();

  bool CreateDirectories();
  const std::wstring& GetDirectoryW(Type type);
  const std::string& GetDirectory(Type type);
  const std::wstring& GetFolderW(Type type);
  const std::string& GetFolder(Type type);
private:
  struct Directory
  {
    std::wstring dirW;
    std::string dir;
    Directory() {}
    Directory(const std::wstring& directory);
    bool Initialized() { return !dirW.empty(); }
  };

  bool FindDocumentDir();
  bool FindBinaryDir();
  bool CreateDir(const std::wstring& dir, Type type);
  void SetDir(Type type);
  void PrintValue(const std::wstring& value, const std::wstring& message);


  std::array<Directory, DIR_MAX> directories_;
  std::array<Directory, DIR_MAX> folders_;
};

extern FileDirectory g_fileDirectory;

