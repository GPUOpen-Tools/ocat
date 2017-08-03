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

#include "IniParser.h"

#include <fstream>

std::wstring ReadStringFromIni(LPCTSTR appName, LPCWSTR keyName, LPCTSTR fileName)
{
  std::wstring buffer;
  DWORD bufferSize = 256;
  DWORD stringSize = 0;
  do {
    buffer.resize(bufferSize);
    stringSize = GetPrivateProfileString(appName, keyName, NULL, &buffer[0], bufferSize, fileName);
    if (errno == ENOENT) {
      printf("Error reading string from ini file %lu\n", GetLastError());
      return L"";
    }
    bufferSize *= 2;
  }
  // if buffer is too small buffer.size() - 1 is returned
  while (stringSize == buffer.size() - 1);

  // stringSize without terminating null character
  buffer.resize(stringSize);
  return buffer;
}

float ReadFloatFromIni(LPCTSTR appName, LPCWSTR keyName, const float defaultValue, LPCTSTR fileName)
{
  const auto string = ReadStringFromIni(appName, keyName, fileName);
  if (string.empty()) {
    return defaultValue;
  }

  size_t processed = 0;
  const float result = std::stof(string, &processed);
  if (processed != string.size()) {
    printf("String was not completely converted to float\n");
    return defaultValue;
  }

  return result;
}

bool ReadBoolFromIni(LPCTSTR appName, LPCWSTR keyName, const bool defaultValue, LPCTSTR fileName)
{
  int defaultInt = static_cast<int>(defaultValue);
  int value = GetPrivateProfileInt(appName, keyName, defaultInt, fileName);
  return value == 0 ? false : true;
}