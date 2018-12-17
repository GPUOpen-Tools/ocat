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

#include <windows.h>
#include <string>

class VK_Environment {
 public:
  void SetVKEnvironment(const std::wstring& dllDirectory);
  void ResetVKEnvironment();

 private:
  // Updates the environment variable with value or sets a new one if not existing
  // Returns the original value of the environment variable
  const std::wstring WriteEnvironmentVariable(const std::wstring& variableName, 
    const std::wstring& value, bool append);
  void LogEnvironmentVariables();
  
  // Saves the value of the environmentVariable inside currValue
  static DWORD ReadEnvironmentVariable(const std::wstring& variableName,
    std::wstring& currValue);

  struct EnvironmentValues {
    std::wstring path;
    std::wstring layers;
    std::wstring ocatVulkan;
  };

  EnvironmentValues originalEnvironment_;
  bool changed_ = false;
};