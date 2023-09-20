﻿//
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

#include "Recording.h"

#include <UIAutomationClient.h>
#include <atlcomcli.h>
#include <algorithm>
#include <cmath>

#include "Logging/MessageLog.h"
#include "Utility/FileUtils.h"
#include "Utility/ProcessHelper.h"
#include "Utility/StringUtils.h"

#include <time.h>

// GPU specs
// AMD
#include "amd_ags.h"
// Nvidia
#include "nvapi.h"
// Intel
#include <d3d11.h>
#include <dxgi.h>
#include "GPUDetect.h"

const std::wstring Recording::defaultProcessName_ = L"*";

Recording::Recording() { PopulateSystemSpecs(); }
Recording::~Recording() {}

struct RawSMBIOSData {
  BYTE Used20CallingMethod;
  BYTE SMBIOSMajorVersion;
  BYTE SMBIOSMinorVersion;
  BYTE DmiRevision;
  DWORD Length;
  BYTE SMBIOSTableData[];
};

struct SMBIOSHeader {
  UINT8 Type;
  UINT8 Length;
  UINT16 Handle;
};

void Recording::ParseSMBIOS()
{
  // get total global memory via GlobalMemoryStatusEx
  MEMORYSTATUSEX lpBuffer;
  lpBuffer.dwLength = sizeof(lpBuffer);
  bool result = GlobalMemoryStatusEx(&lpBuffer);
  if (result) {
    specs_.ram = std::to_string(
        static_cast<int>((lpBuffer.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f)) + 0.5f));
    specs_.ram += " GB";
  }
  else {
    specs_.ram = "Unknown GB";
  }

  DWORD error = ERROR_SUCCESS;
  DWORD smBiosDataSize = 0;
  RawSMBIOSData* smBiosData = NULL;
  DWORD bytesWritten = 0;
  smBiosDataSize = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

  smBiosData = (RawSMBIOSData*)HeapAlloc(GetProcessHeap(), 0, smBiosDataSize);
  if (!smBiosData) {
    error = ERROR_OUTOFMEMORY;
    HeapFree(GetProcessHeap(), 0, smBiosData);
    return;
  }

  bytesWritten = GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);

  if (bytesWritten != smBiosDataSize) {
    error = ERROR_INVALID_DATA;
    HeapFree(GetProcessHeap(), 0, smBiosData);
    return;
  }

  uint32_t slots = 0;
  specs_.motherboard = "\"";
  for (uint32_t i = 0; i < smBiosData->Length; i++) {
    SMBIOSHeader header = *(SMBIOSHeader*)&smBiosData->SMBIOSTableData[i];
    // Motherboard
    if (header.Type == 2) {
      // retrieve Motherboard information
      int count = 0;
      while (smBiosData->SMBIOSTableData[i + header.Length + count] != '\0') {
        // remove ',' in name due to csv structure
        specs_.motherboard += smBiosData->SMBIOSTableData[i + header.Length + count];
        count++;
      }
      count++;
      specs_.motherboard += " ";
      while (smBiosData->SMBIOSTableData[i + header.Length + count] != '\0') {
        // remove ',' in name due to csv structure
        specs_.motherboard += smBiosData->SMBIOSTableData[i + header.Length + count];
        count++;
      }
    }
    // Memory device
    if (header.Type == 17) {
      // retrieve RAM data
      WORD size = *(WORD*)&smBiosData->SMBIOSTableData[i + 12];  // 0Ch

      // if size is 0, no memory device is installed in the socket
      if (size != 0) {
        slots++;
        // we just gather information from the first one we find, assuming any subsequent are
        // identical
        if (slots == 1) {
          BYTE memoryType = smBiosData->SMBIOSTableData[i + 18];  // 12h
          // according to SMBIOS Spec version 3.2.0, Type 17 Memory Device: Type
          switch (memoryType) {
            case 0x01:
              specs_.ram += " Other Memory Type";
              break;
            case 0x02:
              specs_.ram += " Unknown Memory Type";
              break;
            case 0x03:
              specs_.ram += " DRAM";
              break;
            case 0x04:
              specs_.ram += " EDRAM";
              break;
            case 0x05:
              specs_.ram += " VRAM";
              break;
            case 0x06:
              specs_.ram += " SRAM";
              break;
            case 0x07:
              specs_.ram += " RAM";
              break;
            case 0x08:
              specs_.ram += " ROM";
              break;
            case 0x09:
              specs_.ram += " FLASH";
              break;
            case 0x0A:
              specs_.ram += " EEPROM";
              break;
            case 0x0B:
              specs_.ram += " FEPROM";
              break;
            case 0x0C:
              specs_.ram += " EPROM";
              break;
            case 0x0D:
              specs_.ram += " CDRAM";
              break;
            case 0x0E:
              specs_.ram += " 3DRAM";
              break;
            case 0x0F:
              specs_.ram += " SDRAM";
              break;
            case 0x10:
              specs_.ram += " SGRAM";
              break;
            case 0x11:
              specs_.ram += " RDRAM";
              break;
            case 0x12:
              specs_.ram += " DDR";
              break;
            case 0x13:
              specs_.ram += " DDR2";
              break;
            case 0x14:
              specs_.ram += " DDR2 FB-DIMM";
              break;
            case 0x15:
            case 0x16:
            case 0x17:
              specs_.ram += " Reserved Memory Type";
              break;
            case 0x18:
              specs_.ram += " DDR3";
              break;
            case 0x19:
              specs_.ram += " FBD2";
              break;
            case 0x1A:
              specs_.ram += " DDR4";
              break;
            case 0x1B:
              specs_.ram += " LPDDR";
              break;
            case 0x1C:
              specs_.ram += " LPDDR2";
              break;
            case 0x1D:
              specs_.ram += " LPDDR3";
              break;
            case 0x1E:
              specs_.ram += " LPDDR4";
              break;
            case 0x1F:
              specs_.ram += " Logical non-volatile device";
              break;
          }

          WORD speed = *(WORD*)&smBiosData->SMBIOSTableData[i + 21];
          specs_.ram += " " + std::to_string(speed) + " MT/s";
        }
      }
    }

    // jump to next type field
    {
      // formatted fields
      i = i + header.Length;

      // unformatted fields - terminate with \0\0 (0x0 0x0) sequence
      while (
          !(smBiosData->SMBIOSTableData[i] == '\0' && smBiosData->SMBIOSTableData[i + 1] == '\0')) {
        ++i;
      }
      ++i;
    }
  }

  specs_.motherboard += "\"";
}

void Recording::ReadRegistry()
{
  WCHAR processor[512];
  DWORD processorSize = sizeof(processor);

  HKEY registryKey;

  LONG resultRegistry =
      RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
                   KEY_READ, &registryKey);
  resultRegistry = RegQueryValueEx(registryKey, L"ProcessornameString", 0, NULL,
                                   (LPBYTE)processor, &processorSize);

  std::wstring processorStr = processor;
  specs_.cpu = "\"" + ConvertUTF16StringToUTF8String(processorStr) + "\"";

  resultRegistry =
      RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
                   KEY_READ, &registryKey);
  WCHAR osProduct[512];
  DWORD osProductSize = sizeof(osProduct);
  resultRegistry = RegQueryValueEx(registryKey, L"ProductName", 0, NULL, (LPBYTE)osProduct, &osProductSize);
  WCHAR displayVersion[512];
  DWORD displayVersionSize = sizeof(displayVersion);
  resultRegistry = RegQueryValueEx(registryKey, L"DisplayVersion", 0, NULL, (LPBYTE)displayVersion,
                                   &displayVersionSize);
  WCHAR currentBuild[512];
  DWORD currentBuildSize = sizeof(currentBuild);
  resultRegistry = RegQueryValueEx(registryKey, L"CurrentBuild", 0, NULL, (LPBYTE)currentBuild,
                                   &currentBuildSize);
  DWORD UBR[1];
  DWORD UBRSize = sizeof(UBR);
  resultRegistry = RegQueryValueEx(registryKey, L"UBR", 0, NULL, (LPBYTE)UBR, &UBRSize);

  std::wstring osProductStr = osProduct;
  std::wstring displayVersionStr = displayVersion;
  std::wstring currentBuildStr = currentBuild;
  std::wstring ubrStr = std::to_wstring(UBR[0]);

  if (std::stoi(currentBuildStr) > 20000) { // Windows 10: 19xxx; Windows 11: 22xxx
    osProductStr.replace(osProductStr.find(L"10"), sizeof("10") - 1, L"11");
  }

  specs_.os = "\"" + ConvertUTF16StringToUTF8String(osProductStr) + " " +
              ConvertUTF16StringToUTF8String(displayVersionStr) + " OS Build " +
              ConvertUTF16StringToUTF8String(currentBuildStr) + "." + 
              ConvertUTF16StringToUTF8String(ubrStr) + "\"";

  if (specs_.motherboard == "\"\"" || specs_.motherboard.empty()) {
    resultRegistry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0,
                                  KEY_READ, &registryKey);
    WCHAR manufacturer[512];
    DWORD manufacturerSize = sizeof(manufacturer);
    resultRegistry = RegQueryValueEx(registryKey, L"BaseBoardManufacturer", 0, NULL, (LPBYTE)manufacturer, &manufacturerSize);
    WCHAR product[512];
    DWORD productSize = sizeof(product);
    resultRegistry = RegQueryValueEx(registryKey, L"BaseBoardProduct", 0, NULL, (LPBYTE)product, &productSize);

	std::wstring manufacturerStr = manufacturer;
    std::wstring productStr = product;

    specs_.motherboard =
        "\"" + ConvertUTF16StringToUTF8String(manufacturerStr) +
        ConvertUTF16StringToUTF8String(productStr) + "\"";
  }
}

void Recording::GetGPUsInfo()
{
  // AMD
  AGSContext* agsContext = nullptr;
  AGSGPUInfo gpuInfo;
  AGSConfiguration config = {};
  if (agsInitialize(
        AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH),
        &config, &agsContext, &gpuInfo) == AGS_SUCCESS) {
    specs_.driverVersionBasic = gpuInfo.radeonSoftwareVersion;
    specs_.driverVersionDetail = gpuInfo.driverVersion;
    specs_.gpuCount = gpuInfo.numDevices;

    for (int i = 0; i < gpuInfo.numDevices; i++) {
      AGSDeviceInfo& device = gpuInfo.devices[i];
      GPU gpu = {};
      std::string deviceName = device.adapterString;

      // add # of CU for Vega
      if (deviceName.compare("Radeon RX Vega") == 0)
        deviceName += " " + std::to_string(device.numCUs);

      gpu.name = deviceName;
      gpu.coreClock = device.coreClock;
      gpu.memoryClock = device.memoryClock;
      gpu.totalMemory = (int)(device.localMemoryInBytes / (1024 * 1024));

      specs_.gpus.push_back(gpu);
    }
    agsDeInitialize(agsContext);
  }
  else {
    // Nvidia
    NvAPI_Status ret = NVAPI_OK;
    ret = NvAPI_Initialize();
    if (ret == NVAPI_OK) {
      NvU32 driverVersion;
      NvAPI_ShortString buildBranchString;
      NvAPI_SYS_GetDriverAndBranchVersion(&driverVersion, buildBranchString);
      specs_.driverVersionBasic = std::to_string(driverVersion);
      specs_.driverVersionDetail = buildBranchString;

      NvPhysicalGpuHandle handles[NVAPI_MAX_PHYSICAL_GPUS];
      NvU32 gpuCount;
      ret = NvAPI_EnumPhysicalGPUs(handles, &gpuCount);
      specs_.gpuCount = gpuCount;

      for (uint32_t i = 0; i < gpuCount; i++) {
        GPU gpu = {};
        NvAPI_ShortString nvGPU;
        NvAPI_GPU_GetFullName(handles[i], nvGPU);
        gpu.name = nvGPU;

        NV_GPU_CLOCK_FREQUENCIES clkFreqs = {NV_GPU_CLOCK_FREQUENCIES_VER};
        NvAPI_GPU_GetAllClockFrequencies(handles[0], &clkFreqs);
        if (clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent) {
          gpu.coreClock =
              (int)((float)(clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency) * 0.001f);
        }

        NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = {NV_DISPLAY_DRIVER_MEMORY_INFO_VER};
        NvAPI_GPU_GetMemoryInfo(handles[0], &memoryInfo);
        gpu.totalMemory = memoryInfo.dedicatedVideoMemory / 1024;

        specs_.gpus.push_back(gpu);
      }
      NvAPI_Unload();
    }
    else {
      // Intel - GPUDetect - TestMain.cpp

      ////////////////////////////////////////////////////////////////////////////////
      // Copyright 2017-2018 Intel Corporation
      //
      // Licensed under the Apache License, Version 2.0 (the "License");
      // you may not use this file except in compliance with the License.
      // You may obtain a copy of the License at
      //
      // http://www.apache.org/licenses/LICENSE-2.0
      //
      // Unless required by applicable law or agreed to in writing, software
      // distributed under the License is distributed on an "AS IS" BASIS,
      // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
      // See the License for the specific language governing permissions and
      // limitations under the License.
      ////////////////////////////////////////////////////////////////////////////////
      
        // defaults to adapter #0
      IDXGIAdapter* adapter = nullptr;
      int initReturnCode = GPUDetect::InitAdapter(&adapter, 0);
      if (initReturnCode != EXIT_SUCCESS) {
        return;
      };

      ID3D11Device* device = nullptr;
      initReturnCode = GPUDetect::InitDevice(adapter, &device);
      if (initReturnCode != EXIT_SUCCESS) {
        adapter->Release();
        return;
      };

      GPUDetect::GPUData gpuData = {};
      initReturnCode = GPUDetect::InitExtensionInfo(&gpuData, adapter, device);
      if (initReturnCode == EXIT_SUCCESS) {
        specs_.gpuCount = 1;
        GPU gpu = {};

        gpu.name = ConvertUTF16StringToUTF8String(gpuData.description);
        gpu.totalMemory = (int)(gpuData.videoMemory / (1024 * 1024));

        //  Find and print driver version information
        //
        initReturnCode = GPUDetect::InitDxDriverVersion(&gpuData);
        if (gpuData.d3dRegistryDataAvailability) {
          char driverVersion[19] = {};
          GPUDetect::GetDriverVersionAsCString(&gpuData, driverVersion, _countof(driverVersion));

          specs_.driverVersionBasic = driverVersion;
          specs_.driverVersionDetail = std::to_string(gpuData.driverInfo.driverReleaseRevision);
        }
        else {
          specs_.driverVersionBasic = "-";
          specs_.driverVersionDetail = "-";
        }

        if (gpuData.vendorID == GPUDetect::INTEL_VENDOR_ID) {
          //
          // In DirectX, Intel exposes additional information through the driver that can be
          // obtained querying a special DX counter
          //

          // Populate the GPU architecture data with info from the counter, otherwise gpuDetect will
          // use the value we got from the Dx11 extension
          initReturnCode = GPUDetect::InitCounterInfo(&gpuData, device);
          if (initReturnCode == EXIT_SUCCESS) {
            gpu.coreClock = gpuData.maxFrequency;
          }
        }
        // Intel - GPUDetect

        specs_.gpus.push_back(gpu);
      }
      device->Release();
      adapter->Release();
    }
  }
}

void Recording::PopulateSystemSpecs()
{
  specs_ = {};

  // SMBIOS information - Motherboard and RAM
  ParseSMBIOS();

  // Registry information - CPU and OS and if needed, Motherboard
  ReadRegistry();

  // GPU information - AGS or NvAPI or GPUDetect
  GetGPUsInfo();
}

void Recording::Start()
{
  recording_ = true;
  processName_ = defaultProcessName_;
  accumulatedResultsPerProcess_.clear();

  if (recordAllProcesses_) {
    g_messageLog.LogInfo("Recording", "Capturing all processes");
    return;
  }

  processID_ = GetProcessFromWindow();
  if (!processID_ || processID_ == GetCurrentProcessId()) {
    g_messageLog.LogWarning("Recording", "No active process was found, capturing all processes");
    recordAllProcesses_ = true;
    return;
  }

  processName_ = GetProcessNameFromID(processID_);
  g_messageLog.LogInfo("Recording", L"Active Process found " + processName_);
  recordAllProcesses_ = false;
}

void Recording::Stop()
{
  PrintSummary();
  recording_ = false;
  processName_.clear();
  processID_ = 0;
}

bool Recording::IsRecording() const { return recording_; }

const std::wstring& Recording::GetProcessName() const { return processName_; }

void Recording::SetRecordingDirectory(const std::wstring& dir) { directory_ = dir; }

void Recording::SetRecordAllProcesses(bool recordAll) { recordAllProcesses_ = recordAll; }

bool Recording::GetRecordAllProcesses() { return recordAllProcesses_; }

const std::wstring& Recording::GetDirectory() { return directory_; }

void Recording::SetUserNote(const std::wstring& userNote) { userNote_ = userNote; }

DWORD Recording::GetProcessFromWindow()
{
  const auto window = GetForegroundWindow();
  if (!window) {
    g_messageLog.LogError("Recording", "No foreground window found");
    return 0;
  }

  DWORD processID = IsUWPWindow(window) ? GetUWPProcess(window) : GetProcess(window);
  return processID;
}

DWORD Recording::GetUWPProcess(HWND window)
{
  HRESULT hr = CoInitialize(NULL);
  if (hr == RPC_E_CHANGED_MODE) {
    g_messageLog.LogError("Recording", "GetUWPProcess: CoInitialize failed, HRESULT", hr);
  }
  else {
    CComPtr<IUIAutomation> uiAutomation;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&uiAutomation));
    if (FAILED(hr)) {
      g_messageLog.LogError("Recording", "GetUWPProcess: CoCreateInstance failed, HRESULT", hr);
    }
    else {
      CComPtr<IUIAutomationElement> foregroundWindow;
      HRESULT hr = uiAutomation->ElementFromHandle(GetForegroundWindow(), &foregroundWindow);
      if (FAILED(hr)) {
        g_messageLog.LogError("Recording", "GetUWPProcess: ElementFromHandle failed, HRESULT", hr);
      }
      else {
        VARIANT variant;
        variant.vt = VT_BSTR;
        variant.bstrVal = SysAllocString(L"Windows.UI.Core.CoreWindow");
        CComPtr<IUIAutomationCondition> condition;
        hr = uiAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId, variant, &condition);
        if (FAILED(hr)) {
          g_messageLog.LogError("Recording",
                                "GetUWPProcess: ElemenCreatePropertyCondition failed, HRESULT", hr);
        }
        else {
          CComPtr<IUIAutomationElement> coreWindow;
          HRESULT hr =
              foregroundWindow->FindFirst(TreeScope::TreeScope_Children, condition, &coreWindow);
          if (FAILED(hr)) {
            g_messageLog.LogError("Recording", "GetUWPProcess: FindFirst failed, HRESULT", hr);
          }
          else {
            int procesID = 0;
            if (coreWindow) {
              hr = coreWindow->get_CurrentProcessId(&procesID);
              if (FAILED(hr)) {
                g_messageLog.LogError("Recording",
                                      "GetUWPProcess: get_CurrentProcessId failed, HRESULT", hr);
              }
              else {
                g_messageLog.LogInfo("Recording", "GetUWPProcess: found uwp process", procesID);
                return procesID;
              }
            }
          }
        }

        SysFreeString(variant.bstrVal);
      }
    }
  }

  CoUninitialize();
  return 0;
}

DWORD Recording::GetProcess(HWND window)
{
  DWORD processID = 0;
  const auto threadID = GetWindowThreadProcessId(window, &processID);

  if (processID && threadID) {
    return processID;
  }
  else {
    g_messageLog.LogError("Recording", "GetWindowThreadProcessId failed");
    return 0;
  }
}

bool Recording::IsUWPWindow(HWND window)
{
  const auto className = GetWindowClassName(window);
  if (className.compare(L"ApplicationFrameWindow") == 0) {
    g_messageLog.LogInfo("Recording", "UWP window found");
    return true;
  }
  return false;
}

void Recording::FrameStats::UpdateFrameStats(bool presented)
{
  if (!presented) {
    totalMissed++;
    consecutiveMissed++;
  }
  else {
    if (consecutiveMissed > maxConsecutiveMissed) {
      maxConsecutiveMissed = consecutiveMissed;
    }
    consecutiveMissed = 0;
  }
}

void Recording::AddPresent(const std::wstring& fileName, const std::wstring& processName,
                           const CompositorInfo compositorInfo, double timeInSeconds,
                           double msBetweenPresents, PresentFrameInfo frameInfo,
                           double estimatedDriverLag, uint32_t width, uint32_t height)
{
  AccumulatedResults* accInput;

  // key is based on process name and compositor
  std::wstring key = fileName;

  auto it = accumulatedResultsPerProcess_.find(key);
  if (it == accumulatedResultsPerProcess_.end()) {
    AccumulatedResults input = {};
    input.startTime = FormatCurrentTime();
    input.processName = processName;
    input.width = width;
    input.height = height;
    switch (compositorInfo) {
      case CompositorInfo::DWM:
        input.compositor = "DWM";
        break;
      case CompositorInfo::WMR:
        input.compositor = "WMR";
        break;
      case CompositorInfo::SteamVR:
        input.compositor = "SteamVR";
        break;
      case CompositorInfo::OculusVR:
        input.compositor = "OculusVR";
        break;
      default:
        input.compositor = "Unknown";
        break;
    }
    g_messageLog.LogInfo("Recording", L"Received first present for process " + key + L" at " +
                                          ConvertUTF8StringToUTF16String(input.startTime) + L".");
    accumulatedResultsPerProcess_.insert(std::pair<std::wstring, AccumulatedResults>(key, input));
    it = accumulatedResultsPerProcess_.find(key);
  }
  accInput = &it->second;

  if (msBetweenPresents > 0) {
    accInput->frameTimes.push_back(msBetweenPresents);
  }
  else if (accInput->timeInSeconds > 0 && timeInSeconds > 0) {
    accInput->frameTimes.push_back(1000 * (timeInSeconds - accInput->timeInSeconds));
  }

  if (timeInSeconds > 0) accInput->timeInSeconds = timeInSeconds;

  accInput->estimatedDriverLag += estimatedDriverLag;

  switch (frameInfo) {
    case PresentFrameInfo::COMPOSITOR_APP_WARP: {
      accInput->app.UpdateFrameStats(true);
      accInput->warp.UpdateFrameStats(true);
      return;
    }
    case PresentFrameInfo::COMPOSITOR_APPMISS_WARP: {
      accInput->app.UpdateFrameStats(false);
      accInput->warp.UpdateFrameStats(true);
      return;
    }
    case PresentFrameInfo::COMPOSITOR_APP_WARPMISS: {
      accInput->app.UpdateFrameStats(true);
      accInput->warp.UpdateFrameStats(false);
      return;
    }
    case PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS: {
      accInput->app.UpdateFrameStats(false);
      accInput->warp.UpdateFrameStats(false);
      return;
    }
  }
  return;
}

std::string Recording::FormatCurrentTime()
{
  struct tm tm;
  time_t time_now = time(NULL);
  localtime_s(&tm, &time_now);
  char buffer[4096];
  _snprintf_s(buffer, _TRUNCATE, "%4d%02d%02d-%02d%02d%02d",  // ISO 8601
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return std::string(buffer);
}

double calcPercentile(std::vector<double> sortedData, double percentile)
{
  const size_t size = sortedData.size();

  if (size < 2) {
    return -1.0;  // throw instead?
  }
  double rank = (percentile / 100.0) * (size - 1) + 1;
  size_t rankInt = (size_t)rank;
  double rankFrac = rank - rankInt;
  double low = sortedData[rankInt - 1];
  double hi = sortedData[rankInt];
  return low + rankFrac * (hi - low);
}

double calcMedian(const std::vector<double>& sortedData)
{
  const size_t size = sortedData.size();
  if (size == 0) {
    return 0.0f;
  }
  if (size % 2 == 1) {
    return sortedData[size / 2];
  }
  double lo = sortedData[(size - 1) / 2];
  double high = sortedData[size / 2];
  return (lo + high) / 2.0;
}

double calcMean(const std::vector<double>& sortedData)
{
  const size_t size = sortedData.size();
  if (size == 0) {
    return 0.0f;
  }
  double sum = 0.0;
  for (const double& val : sortedData) {
    sum += val;
  }
  return sum / size;
}

double calcStdDev(const std::vector<double>& sortedData, double mean)
{
  const size_t size = sortedData.size();
  double squaredDiffsSum = 0.0;
  for (const double& val : sortedData) {
    double diff = val - mean;
    squaredDiffsSum += diff * diff;
  }
  return sqrt(squaredDiffsSum / size);
}

struct Statistics {
  double minimum;
  double maximum;
  double mean;
  double stdDev;
  double median;
  double percentile01;
  double percentile1;
  double percentile5;
  double percentile25;
  double percentile75;
  double percentile95;
  double percentile99;
  double percentile999;
};

Statistics calcStats(const std::vector<double>& data)
{
  const size_t size = data.size();
  if (size < 2) {
    return {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0};  // throw instead?
  }

  std::vector<double> sortedData(data);
  std::sort(sortedData.begin(), sortedData.end(), std::less<double>());

  Statistics stats;
  stats.minimum = sortedData[0];
  stats.maximum = sortedData[size - 1];
  stats.mean = calcMean(sortedData);
  stats.stdDev = calcStdDev(sortedData, stats.mean);
  stats.median = calcMedian(sortedData);
  stats.percentile01 = calcPercentile(sortedData, 0.1);
  stats.percentile1 = calcPercentile(sortedData, 1);
  stats.percentile5 = calcPercentile(sortedData, 5);
  stats.percentile25 = calcPercentile(sortedData, 25);
  stats.percentile75 = calcPercentile(sortedData, 75);
  stats.percentile95 = calcPercentile(sortedData, 95);
  stats.percentile99 = calcPercentile(sortedData, 99);
  stats.percentile999 = calcPercentile(sortedData, 99.9);
  return stats;
}

void Recording::PrintSummary()
{
  if (accumulatedResultsPerProcess_.size() == 0) {
    // Only print the summary, if we have actual results.
    return;
  }

  std::wstring summaryFilePath = directory_ + L"perf_summary.csv";

  bool summaryFileExisted = FileExists(summaryFilePath);

  // Open summary file, possibly create it.
  std::ofstream summaryFile(summaryFilePath, std::ofstream::app);
  if (summaryFile.fail()) {
    g_messageLog.LogError("Recording",
                          "Can't open summary file. Either it is open in another process or OCAT "
                          "is missing write permissions.");
    return;
  }

  // If newly created, append header:
  if (!summaryFileExisted) {
    std::string bom_utf8 = "\xef\xbb\xbf";
    summaryFile << bom_utf8;
    std::string header =
        "File,Application Name,Compositor,Date and Time,Average FPS (Application),"
        "Average frame time (ms) (Application),"
        "Minimum frame time (ms) (Application),"
        "Maximum frame time (ms) (Application),"
        "Median frame time (ms) (Application),"
        "Standard deviation of frame time (ms) (Application),"
        "0.1st-percentile frame time (ms) (Application),"
        "1st-percentile frame time (ms) (Application),"
        "5th-percentile frame time (ms) (Application),"
        "25th-percentile frame time (ms) (Application),"
        "75th-percentile frame time (ms) (Application),"
        "95th-percentile frame time (ms) (Application),"
        "99th-percentile frame time (ms) (Application),"
        "99.9th-percentile frame time (ms) (Application),"
        "Missed frames (Application),Average number of missed frames (Application),"
        "Maximum number of consecutive missed frames (Application),Missed frames (Compositor),"
        "Average number of missed frames (Compositor),Maximum number of consecutive missed frames "
        "(Compositor),"
        "Average Estimated Driver Lag (ms),Width,Height,User Note,"
        "Motherboard,OS,Processor,System RAM,Base Driver Version,Driver Package,"
        "GPU #,GPU,GPU Core Clock (MHz),GPU Memory Clock (MHz),GPU Memory (MB)\n";
    summaryFile << header;
  }

  for (auto& item : accumulatedResultsPerProcess_) {
    std::stringstream line;
    AccumulatedResults& input = item.second;

    Statistics frameStats = calcStats(input.frameTimes);

    double avgFPS = input.frameTimes.size() / input.timeInSeconds;
    double avgFrameTime = (input.timeInSeconds * 1000.0) / input.frameTimes.size();
    double avgMissedFramesApp = static_cast<double>(input.app.totalMissed) /
                                (input.frameTimes.size() + input.app.totalMissed);
    double avgMissedFramesCompositor = static_cast<double>(input.warp.totalMissed) /
                                       (input.frameTimes.size() + input.warp.totalMissed);
    double avgEstimatedDriverLag = (input.estimatedDriverLag) / input.frameTimes.size();

    line.precision(1);

    line << ConvertUTF16StringToUTF8String(item.first) << ","
         << ConvertUTF16StringToUTF8String(input.processName) << "," << input.compositor << ","
         << input.startTime << "," << std::fixed << avgFPS << "," << avgFrameTime << ","
         << frameStats.minimum << "," << frameStats.maximum << "," << frameStats.median << ","
         << frameStats.stdDev << "," << frameStats.percentile01 << "," << frameStats.percentile1
         << "," << frameStats.percentile5 << "," << frameStats.percentile25 << ","
         << frameStats.percentile75 << "," << frameStats.percentile95 << ","
         << frameStats.percentile99 << "," << frameStats.percentile999 << ","
         << input.app.totalMissed << "," << avgMissedFramesApp << ","
         << input.app.maxConsecutiveMissed << "," << input.warp.totalMissed << ","
         << avgMissedFramesCompositor << "," << input.warp.maxConsecutiveMissed << ","
         << avgEstimatedDriverLag << "," << input.width << "," << input.height << ","
         << ConvertUTF16StringToUTF8String(userNote_) << "," << specs_.motherboard << ","
         << specs_.os << "," << specs_.cpu << "," << specs_.ram << "," << specs_.driverVersionBasic
         << "," << specs_.driverVersionDetail << "," << specs_.gpuCount;

    for (int i = 0; i < specs_.gpuCount; i++) {
      line << "," << specs_.gpus[i].name << "," << specs_.gpus[i].coreClock << ","
           << ((specs_.gpus[i].memoryClock > 0) ? std::to_string(specs_.gpus[i].memoryClock) : "-")
           << "," << specs_.gpus[i].totalMemory;
    }

    line << std::endl;

    summaryFile << line.str();
  }

  summaryFile.close();
}
