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

#include "Recording.h"

#include <UIAutomationClient.h>
#include <atlcomcli.h>
#include <algorithm>

#include "Logging/MessageLog.h"
#include "Utility/ProcessHelper.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

#include <time.h>

// GPU specs
// AMD
#include "amd_ags.h"
// Nvidia
#include "nvapi.h"
// Intel
#include "DeviceId.h"

const std::wstring Recording::defaultProcessName_ = L"*";

Recording::Recording() { PopulateSystemSpecs(); }
Recording::~Recording() {}

struct RawSMBIOSData
{
  BYTE    Used20CallingMethod;
  BYTE    SMBIOSMajorVersion;
  BYTE    SMBIOSMinorVersion;
  BYTE    DmiRevision;
  DWORD    Length;
  BYTE    SMBIOSTableData[];
};

struct SMBIOSHeader
{
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
    specs.ram = std::to_string(static_cast<int>((lpBuffer.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f)) + 0.5f));
    specs.ram += " GB";
  }
  else {
    specs.ram = "Unknown GB";
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
  specs.motherboard = "\"";
  for (uint32_t i = 0; i < smBiosData->Length; i++)
  {
    SMBIOSHeader header = *(SMBIOSHeader*)&smBiosData->SMBIOSTableData[i];
    // Motherboard
    if (header.Type == 2) {
      // retrieve Motherboard information
      int count = 0;
      while (smBiosData->SMBIOSTableData[i + header.Length + count] != '\0') {
        // remove ',' in name due to csv structure
        specs.motherboard += smBiosData->SMBIOSTableData[i + header.Length + count];
        count++;
      }
      count++;
      specs.motherboard += " ";
      while (smBiosData->SMBIOSTableData[i + header.Length + count] != '\0') {
        // remove ',' in name due to csv structure
        specs.motherboard += smBiosData->SMBIOSTableData[i + header.Length + count];
        count++;
      }
    }
    // Memory device
    if (header.Type == 17) {
      // retrieve RAM data
      WORD size = *(WORD*)&smBiosData->SMBIOSTableData[i + 12]; // 0Ch

            // if size is 0, no memory device is installed in the socket
      if (size != 0) {
        slots++;
        // we just gather information from the first one we find, assuming any subsequent are identical
        if (slots == 1) {
          BYTE memoryType = smBiosData->SMBIOSTableData[i + 18]; // 12h
          // according to SMBIOS Spec version 3.2.0, Type 17 Memory Device: Type
          switch (memoryType) {
          case 0x01: specs.ram += " Other Memory Type"; break;
          case 0x02: specs.ram += " Unknown Memory Type"; break;
          case 0x03: specs.ram += " DRAM"; break;
          case 0x04: specs.ram += " EDRAM"; break;
          case 0x05: specs.ram += " VRAM"; break;
          case 0x06: specs.ram += " SRAM"; break;
          case 0x07: specs.ram += " RAM"; break;
          case 0x08: specs.ram += " ROM"; break;
          case 0x09: specs.ram += " FLASH"; break;
          case 0x0A: specs.ram += " EEPROM"; break;
          case 0x0B: specs.ram += " FEPROM"; break;
          case 0x0C: specs.ram += " EPROM"; break;
          case 0x0D: specs.ram += " CDRAM"; break;
          case 0x0E: specs.ram += " 3DRAM"; break;
          case 0x0F: specs.ram += " SDRAM"; break;
          case 0x10: specs.ram += " SGRAM"; break;
          case 0x11: specs.ram += " RDRAM"; break;
          case 0x12: specs.ram += " DDR"; break;
          case 0x13: specs.ram += " DDR2"; break;
          case 0x14: specs.ram += " DDR2 FB-DIMM"; break;
          case 0x15:
          case 0x16:
          case 0x17: specs.ram += " Reserved Memory Type"; break;
          case 0x18: specs.ram += " DDR3"; break;
          case 0x19: specs.ram += " FBD2"; break;
          case 0x1A: specs.ram += " DDR4"; break;
          case 0x1B: specs.ram += " LPDDR"; break;
          case 0x1C: specs.ram += " LPDDR2"; break;
          case 0x1D: specs.ram += " LPDDR3"; break;
          case 0x1E: specs.ram += " LPDDR4"; break;
          case 0x1F: specs.ram += " Logical non-volatile device"; break;
          }

          WORD speed = *(WORD*)&smBiosData->SMBIOSTableData[i + 21];
          specs.ram += " " + std::to_string(speed) + " MT/s";
        }
      }
    }

    // jump to next type field
    {
      // formatted fields
      i = i + header.Length;

      // unformatted fields - terminate with \0\0 (0x0 0x0) sequence
      while (!(smBiosData->SMBIOSTableData[i] == '\0'
        && smBiosData->SMBIOSTableData[i + 1] == '\0')) {
        ++i;
      }
      ++i;
    }
  }

  specs.motherboard += "\"";
}

void Recording::ReadRegistry()
{
  std::wstring processor;

  HKEY registryKey;

  LONG resultRegistry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &registryKey);
  resultRegistry = GetStringRegKey(registryKey, L"ProcessornameString", processor, L"");

  specs.cpu = "\"" + std::string(processor.begin(), processor.end()) + "\"";;

  resultRegistry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &registryKey);
  std::wstring osProduct;
  resultRegistry = GetStringRegKey(registryKey, L"ProductName", osProduct, L"");
  std::wstring releaseId;
  resultRegistry = GetStringRegKey(registryKey, L"ReleaseId", releaseId, L"");
  std::wstring buildLabEx;
  resultRegistry = GetStringRegKey(registryKey, L"BuildLabEx", buildLabEx, L"");

  specs.os = "\"" + std::string(osProduct.begin(), osProduct.end())
    + " " + std::string(releaseId.begin(), releaseId.end())
    + " (OS Build " + std::string(buildLabEx.begin(), buildLabEx.end()) + ")\"";

  if (specs.motherboard == "\"\"" || specs.motherboard.empty()) {
    resultRegistry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &registryKey);
    std::wstring manufacturer;
    resultRegistry = GetStringRegKey(registryKey, L"BaseBoardManufacturer", manufacturer, L"");
    std::wstring product;
    resultRegistry = GetStringRegKey(registryKey, L"BaseBoardProduct", product, L"");

    specs.motherboard = "\"" + std::string(manufacturer.begin(), manufacturer.end())
      + std::string(product.begin(), product.end()) + "\"";
  }
}

void Recording::GetGPUsInfo()
{
  // AMD
  AGSContext* agsContext = nullptr;
  AGSGPUInfo gpuInfo;
  AGSConfiguration config = {};
  if (agsInit(&agsContext, &config, &gpuInfo) == AGS_SUCCESS)
  {
    specs.driverVersionBasic = gpuInfo.radeonSoftwareVersion;
    specs.driverVersionDetail = gpuInfo.driverVersion;
    specs.gpuCount = gpuInfo.numDevices;

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

      specs.gpus.push_back(gpu);
    }
    agsDeInit(agsContext);
  }
  else {
    // Nvidia
    NvAPI_Status ret = NVAPI_OK;
    ret = NvAPI_Initialize();
    if (ret == NVAPI_OK)
    {
      NvU32 driverVersion;
      NvAPI_ShortString buildBranchString;
      NvAPI_SYS_GetDriverAndBranchVersion(&driverVersion, buildBranchString);
      specs.driverVersionBasic = std::to_string(driverVersion);
      specs.driverVersionDetail = buildBranchString;

      NvPhysicalGpuHandle handles[NVAPI_MAX_PHYSICAL_GPUS];
      NvU32 gpuCount;
      ret = NvAPI_EnumPhysicalGPUs(handles, &gpuCount);
      specs.gpuCount = gpuCount;

      for (uint32_t i = 0; i < gpuCount; i++)
      {
        GPU gpu = {};
        NvAPI_ShortString nvGPU;
        NvAPI_GPU_GetFullName(handles[i], nvGPU);
        gpu.name = nvGPU;

        NV_GPU_CLOCK_FREQUENCIES clkFreqs = { NV_GPU_CLOCK_FREQUENCIES_VER };
        NvAPI_GPU_GetAllClockFrequencies(handles[0], &clkFreqs);
        if (clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent)
        {
          gpu.coreClock = (int)((float)(clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency) * 0.001f);
        }

        NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { NV_DISPLAY_DRIVER_MEMORY_INFO_VER };
        NvAPI_GPU_GetMemoryInfo(handles[0], &memoryInfo);
        gpu.totalMemory = memoryInfo.dedicatedVideoMemory / 1024;

        specs.gpus.push_back(gpu);
      }
      NvAPI_Unload();
    }
    else {
      // Intel
      // only detects primary graphics driver
      uint32_t vendorId, deviceId;
      uint64_t videoMemory;
      std::wstring GFXBrand;
      if (getGraphicsDeviceInfo(&vendorId, &deviceId, &videoMemory, &GFXBrand))
      {
        specs.driverVersionBasic = "-";
        specs.driverVersionDetail = "-";
        specs.gpuCount = 1;
        GPU gpu = {};
        gpu.name = ConvertUTF16StringToUTF8String(GFXBrand);
        gpu.totalMemory = static_cast<int>(videoMemory / (1024 * 1024));

        if (vendorId == INTEL_VENDOR_ID)
        {
          IntelDeviceInfoHeader intelDeviceInfoHeader = { 0 };
          unsigned char intelDeviceInfoBuffer[1024];
          if (GGF_SUCCESS == getIntelDeviceInfo(vendorId, &intelDeviceInfoHeader, &intelDeviceInfoBuffer))
          {
            if (intelDeviceInfoHeader.Version == 2)
            {
              IntelDeviceInfoV2 intelDeviceInfo;
              memcpy(&intelDeviceInfo, intelDeviceInfoBuffer, intelDeviceInfoHeader.Size);
              gpu.coreClock = intelDeviceInfo.GPUMaxFreq;
            }
            else if (intelDeviceInfoHeader.Version == 1)
            {
              IntelDeviceInfoV1 intelDeviceInfo;
              memcpy(&intelDeviceInfo, intelDeviceInfoBuffer, intelDeviceInfoHeader.Size);
              gpu.coreClock = intelDeviceInfo.GPUMaxFreq;
            }
          }
        }
        specs.gpus.push_back(gpu);
      }
    }
  }
}

void Recording::PopulateSystemSpecs()
{
  specs = {};

  // SMBIOS information - Motherboard and RAM
  ParseSMBIOS();

  // Registry information - CPU and OS and if needed, Motherboard
  ReadRegistry();

  // GPU information - AGS or NvAPI
  GetGPUsInfo();
}

void Recording::Start()
{
  recording_ = true;
  processName_ = defaultProcessName_;
  accumulatedResultsPerProcess_.clear();

  if (recordAllProcesses_)
  {
    g_messageLog.LogInfo("Recording",
      "Capturing all processes");
    return;
  }

  processID_ = GetProcessFromWindow();
  if (!processID_ || processID_ == GetCurrentProcessId()) {
    g_messageLog.LogWarning("Recording",
      "No active process was found, capturing all processes");
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

bool Recording::IsRecording() const
{ 
  return recording_;
}

const std::wstring & Recording::GetProcessName() const
{
  return processName_;
}

void Recording::SetRecordingDirectory(const std::wstring & dir)
{
  directory_ = dir;
}

void Recording::SetRecordAllProcesses(bool recordAll)
{
  recordAllProcesses_ = recordAll;
}

bool Recording::GetRecordAllProcesses()
{
  return recordAllProcesses_;
}

const std::wstring & Recording::GetDirectory()
{
  return directory_;
}

void Recording::SetUserNote(const std::wstring & userNote)
{
  userNote_ = userNote;
}

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
    g_messageLog.LogError("Recording",
      "GetUWPProcess: CoInitialize failed, HRESULT", hr);
  }
  else {
    CComPtr<IUIAutomation> uiAutomation;
    hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(&uiAutomation));
    if (FAILED(hr)) {
      g_messageLog.LogError("Recording",
        "GetUWPProcess: CoCreateInstance failed, HRESULT", hr);
    }
    else {
      CComPtr<IUIAutomationElement> foregroundWindow;
      HRESULT hr = uiAutomation->ElementFromHandle(GetForegroundWindow(), &foregroundWindow);
      if (FAILED(hr)) {
        g_messageLog.LogError("Recording",
          "GetUWPProcess: ElementFromHandle failed, HRESULT", hr);
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
            g_messageLog.LogError("Recording",
              "GetUWPProcess: FindFirst failed, HRESULT", hr);
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
                g_messageLog.LogInfo("Recording",
                  "GetUWPProcess: found uwp process", procesID);
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

void Recording::FrameStats::UpdateFrameStats(bool presented) {
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

void Recording::AddPresent(const std::string& fileName, const std::string& processName, const CompositorInfo compositorInfo, double timeInSeconds, double msBetweenPresents,
  PresentFrameInfo frameInfo)
{
  AccumulatedResults* accInput;

  // key is based on process name and compositor
  std::string key = fileName;

  auto it = accumulatedResultsPerProcess_.find(key);
  if (it == accumulatedResultsPerProcess_.end())
  {
    AccumulatedResults input = {};
    input.startTime = FormatCurrentTime();
    input.processName = processName;
    switch (compositorInfo)
    {
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
    g_messageLog.LogInfo("Recording", "Received first present for process " + key + " at " + input.startTime + ".");
    accumulatedResultsPerProcess_.insert(std::pair<std::string, AccumulatedResults>(key, input));
    it = accumulatedResultsPerProcess_.find(key);
  }
  accInput = &it->second;

  if (msBetweenPresents > 0) {
  accInput->frameTimes.push_back(msBetweenPresents);
  }
  else if (accInput->timeInSeconds > 0 && timeInSeconds > 0) {
    accInput->frameTimes.push_back(1000 * (timeInSeconds - accInput->timeInSeconds));
  }

  if (timeInSeconds > 0)
    accInput->timeInSeconds = timeInSeconds;

  switch (frameInfo)
  {
  case PresentFrameInfo::COMPOSITOR_APP_WARP:
  {
    accInput->app.UpdateFrameStats(true);
    accInput->warp.UpdateFrameStats(true);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APPMISS_WARP:
  {
    accInput->app.UpdateFrameStats(false);
    accInput->warp.UpdateFrameStats(true);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APP_WARPMISS:
  {
    accInput->app.UpdateFrameStats(true);
    accInput->warp.UpdateFrameStats(false);
    return;
  }
  case PresentFrameInfo::COMPOSITOR_APPMISS_WARPMISS:
  {
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

void Recording::PrintSummary()
{
  if (accumulatedResultsPerProcess_.size() == 0)
  {
    // Only print the summary, if we have actual results.
    return;
  }

  std::wstring summaryFilePath = directory_ + L"perf_summary.csv";

  bool summaryFileExisted = FileExists(summaryFilePath);

  // Open summary file, possibly create it.
  std::ofstream summaryFile(summaryFilePath, std::ofstream::app);
  if (summaryFile.fail())
  {
    g_messageLog.LogError("Recording",
      "Can't open summary file. Either it is open in another process or OCAT is missing write permissions.");
    return;
  }

  // If newly created, append header:
  if (!summaryFileExisted)
  {
    std::string bom_utf8 = "\xef\xbb\xbf";
    summaryFile << bom_utf8;
    std::string header = "File,Application Name,Compositor,Date and Time,Average FPS (Application)," \
      "Average frame time (ms) (Application),99th-percentile frame time (ms) (Application)," \
      "Missed frames (Application),Average number of missed frames (Application)," \
      "Maximum number of consecutive missed frames (Application),Missed frames (Compositor)," \
      "Average number of missed frames (Compositor),Maximum number of consecutive missed frames (Compositor)," \
      "User Note,"
      "Motherboard, OS, Processor, System RAM, Base Driver Version, Driver Package," \
      "GPU #, GPU, GPU Core Clock (MHz), GPU Memory Clock (MHz), GPU Memory (MB)\n";
    summaryFile << header;
  }

  for (auto& item : accumulatedResultsPerProcess_)
  {
    std::stringstream line;
    AccumulatedResults& input = item.second;

    double avgFPS = input.frameTimes.size() / input.timeInSeconds;
    double avgFrameTime = (input.timeInSeconds * 1000.0) / input.frameTimes.size();
    std::sort(input.frameTimes.begin(), input.frameTimes.end(), std::less<double>());
    const auto rank = static_cast<int>(0.99 * input.frameTimes.size());
    double frameTimePercentile = input.frameTimes[rank];
    double avgMissedFramesApp = static_cast<double> (input.app.totalMissed)
      / (input.frameTimes.size() + input.app.totalMissed);
    double avgMissedFramesCompositor = static_cast<double> (input.warp.totalMissed)
    / (input.frameTimes.size() + input.warp.totalMissed);

    line << item.first << "," << input.processName << "," << input.compositor << ","
      << input.startTime << "," << avgFPS << ","
      << avgFrameTime << "," << frameTimePercentile << "," << input.app.totalMissed << ","
      << avgMissedFramesApp << "," << input.app.maxConsecutiveMissed << ","
      << input.warp.totalMissed << "," << avgMissedFramesCompositor << ","
      << input.warp.maxConsecutiveMissed << ","  << ConvertUTF16StringToUTF8String(userNote_) << ","
      << specs.motherboard << "," << specs.os << "," << specs.cpu << "," << specs.ram << ","
      << specs.driverVersionBasic << "," << specs.driverVersionDetail << "," << specs.gpuCount;

    for (int i = 0; i < specs.gpuCount; i++) {
      line << "," << specs.gpus[i].name << "," << specs.gpus[i].coreClock << ","
        << ((specs.gpus[i].memoryClock > 0) ? std::to_string(specs.gpus[i].memoryClock) : "-") << "," << specs.gpus[i].totalMemory;
    }

    line << std::endl;

    summaryFile << line.str();
  }
}
