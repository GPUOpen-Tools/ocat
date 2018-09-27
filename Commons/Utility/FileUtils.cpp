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
// The above copyright notice and this permission notice shall be included in
// all
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

#include "FileUtils.h"
#include <windows.h>

bool FileExists(const std::string& filePath)
{
  std::ifstream file(filePath);
  return file.good();
}

bool FileExists(const std::wstring& fileName)
{
  std::ifstream file(fileName);
  return file.good();
}

std::wstring GetDirFromPathSlashes(const std::wstring& path)
{
  const auto pathEnd = path.find_last_of('\\');
  if (pathEnd == std::string::npos) {
    printf("Failed finding end of path\n");
    return L"";
  }

  return path.substr(0, pathEnd + 1);
}

std::wstring GetDirFomPathSlashesRemoved(const std::wstring& path)
{
  const size_t directoryEnd = path.find_last_of('\\');
  std::wstring directory;
  if (std::string::npos != directoryEnd) {
    directory = path.substr(0, directoryEnd);
  }
  else {
    MessageBox(NULL, L"Invalid file path ", NULL, MB_OK);
  }
  return directory;
}
