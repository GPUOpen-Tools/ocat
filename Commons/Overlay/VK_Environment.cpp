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

#include "VK_Environment.h"
#include "../Logging/MessageLog.h"

#include <cstdlib>

namespace {
const wchar_t g_vkEnvPath[] = L"VK_LAYER_PATH";
const wchar_t g_vkEnvLayers[] = L"VK_INSTANCE_LAYERS";
const wchar_t g_vkLayerValue[] = L"VK_LAYER_OCAT_overlay32;VK_LAYER_OCAT_overlay64";
const wchar_t g_vkEnvOcat[] = L"OCAT_VULKAN_LAYER_ENABLED";
const wchar_t g_vkEnvOcatEnabled[] = L"1";
}

void VK_Environment::SetVKEnvironment(const std::wstring& dllDirectory)
{
  const auto dir = dllDirectory.substr(0, dllDirectory.find_last_of('\\'));
  originalEnvironment_.path = WriteEnvironmentVariable(g_vkEnvPath, dir, true);
  originalEnvironment_.layers = WriteEnvironmentVariable(g_vkEnvLayers, g_vkLayerValue, true);
  originalEnvironment_.ocatVulkan = WriteEnvironmentVariable(g_vkEnvOcat, g_vkEnvOcatEnabled, true);
  changed_ = true;
}

void VK_Environment::ResetVKEnvironment()
{
  if (changed_) 
  {
    WriteEnvironmentVariable(g_vkEnvPath, originalEnvironment_.path, false);
    WriteEnvironmentVariable(g_vkEnvLayers, originalEnvironment_.layers, false);
    WriteEnvironmentVariable(g_vkEnvOcat, originalEnvironment_.ocatVulkan, false);
  }
  changed_ = false;
}

const std::wstring VK_Environment::WriteEnvironmentVariable(const std::wstring& variableName,
  const std::wstring& value, bool append)
{
  std::wstring currVariableValue;
  std::wstring newVariableValue;

  const auto result = ReadEnvironmentVariable(variableName, currVariableValue);
  if (result == ERROR_SUCCESS || result == ERROR_ENVVAR_NOT_FOUND) 
  {
    if (append) 
    {
      newVariableValue = value + L';' + currVariableValue;
    }
    else 
    {
      newVariableValue = value;
    }

    const auto error = _wputenv_s(variableName.c_str(), newVariableValue.c_str());
    if (error) 
    {
      g_messageLog.LogError("Overlay",
        L"Failed setting environment variable " + variableName, error);
    }
    else
    {
      g_messageLog.LogInfo("Overlay", 
        L"Set environment variable " + variableName + L" to \"" + newVariableValue + L"\"");
    }
  }
  else
  {
    g_messageLog.LogWarning("Overlay",
      L"Failed getting environment variable", result);
  }

  return currVariableValue;
}

void VK_Environment::LogEnvironmentVariables()
{
  const auto currEnv = GetEnvironmentStrings();
  LPTSTR lpszVariable = (LPTSTR)currEnv;

  std::wstring value;
  while (*lpszVariable) {
    value += std::wstring(lpszVariable) + L'\n';
    lpszVariable += lstrlen(lpszVariable) + 1;
  }
  FreeEnvironmentStrings(currEnv);

  g_messageLog.LogVerbose("Overlay", 
    L"Environment variables " + value);
}

DWORD VK_Environment::ReadEnvironmentVariable(const std::wstring& variableName,
                                                 std::wstring& currValue)
{
  currValue.clear();
  DWORD bufferSize = GetEnvironmentVariable(variableName.c_str(), nullptr, 0);
  if (!bufferSize) 
  {
    // we only ever call this method to set the variable, so not retrieving one is totally fine.
    return GetLastError();
  }

  // if the buffer is empty
  if (bufferSize == 1) 
  {
    g_messageLog.LogWarning("Overlay", 
      L"Environment variable is empty.");
    return ERROR_ENVVAR_NOT_FOUND;
  }

  currValue.resize(bufferSize);
  bufferSize = GetEnvironmentVariable(variableName.c_str(), &currValue[0], bufferSize);
  if (!bufferSize) 
  {
    const auto error = GetLastError();
    g_messageLog.LogWarning("Overlay", 
      L"Failed to retrieve buffer size " + std::to_wstring(bufferSize), error);
    return error;
  }

  return ERROR_SUCCESS;
}
