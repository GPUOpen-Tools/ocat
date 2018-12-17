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

#include "StringUtils.h"
#include <codecvt>
#include <sstream>

std::vector<std::string> Split(const std::string& text, const char delimiter)
{
  std::vector<std::string> result;
  size_t position = 0;
  size_t hit;
  while ((hit = text.find_first_of(delimiter, position)) != std::string::npos)
  {
    result.push_back(text.substr(position, hit - position));
    position = hit + 1;
  }
  // Append remaining characters.
  result.push_back(text.substr(position));
  return result;
}

std::wstring Join(const std::vector<std::wstring>& elements, const wchar_t delimiter)
{
  std::wstringstream stream;
  for (const auto& element : elements)
  {
    stream << element << delimiter;
  }
  return stream.str();
}


std::wstring ConvertUTF8StringToUTF16String(const std::string& input)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring result = converter.from_bytes(input);
  return result;
}

std::string ConvertUTF16StringToUTF8String(const std::wstring& input)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::string result = converter.to_bytes(input);
  return result;
}
