/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
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
/////////////////////////////////////////////////////////////////////////////////////////////

//
// DeviceId.h
//
#pragma once

#include <windows.h>
#include <string>

#define INTEL_VENDOR_ID 0x8086

typedef enum
{ 
    IGFX_UNKNOWN     = 0x0, 
    IGFX_SANDYBRIDGE = 0xC, 
    IGFX_IVYBRIDGE,    
    IGFX_HASWELL,
	IGFX_VALLEYVIEW,
	IGFX_BROADWELL,
	IGFX_SKYLAKE,
	IGFX_KABYLAKE,
	IGFX_COFFEELAKE
} PRODUCT_FAMILY;

//
// Device dependent counter and associated data structures
//
#define INTEL_DEVICE_INFO_COUNTERS         "Intel Device Information"

struct IntelDeviceInfoV1
{
    DWORD GPUMaxFreq;
    DWORD GPUMinFreq;
};

struct IntelDeviceInfoV2
{
	DWORD GPUMaxFreq;
	DWORD GPUMinFreq;
	DWORD GTGeneration;
	DWORD EUCount;
	DWORD PackageTDP;
	DWORD MaxFillRate;
};

struct IntelDeviceInfoHeader
{
	DWORD Size;
	DWORD Version;
};

/*****************************************************************************************
 * getGraphicsDeviceInfo
 *
 *     Function to get the primary graphics device's Vendor ID and Device ID, either 
 *     through the new DXGI interface or through the older D3D9 interfaces.
 *     The function also returns the amount of memory availble for graphics using 
 *     the value shared + dedicated video memory returned from DXGI, or, if the DXGI
 *	   interface is not available, the amount of memory returned from WMI.
 *
 *****************************************************************************************/

bool getGraphicsDeviceInfo( unsigned int* VendorId,
                          unsigned int* DeviceId,
						  unsigned __int64* VideoMemory,
						  std::wstring* GFXBrand);

/*****************************************************************************************
 * getIntelDeviceInfo
 *
 *     Returns the device info:
 *       GPU Max Frequency (Mhz)
 *       GPU Min Frequency (Mhz)
 *       GT Generation (enum)
 *       EU Count (unsigned int)
 *       Package TDP (Watts)
 *       Max Fill Rate (Pixel/Clk)
 * 
 * A return value of GGF_SUCCESS indicates 
 *	   the frequency was returned correctly. 
 *     This function is only valid on Intel graphics devices SNB and later.
 *****************************************************************************************/

long getIntelDeviceInfo( unsigned int VendorId, IntelDeviceInfoHeader *pIntelDeviceInfoHeader, void *pIntelDeviceInfoBuffer );


/*****************************************************************************************
 * checkDxExtensionVersion
 *
 *      Returns the EXTENSION_INTERFACE_VERSION supported by the driver
 *      EXTENSION_INTERFACE_VERSION_1_0 supports extensions for pixel synchronization and
 *      instant access of graphics memory
 *
 *****************************************************************************************/
unsigned int checkDxExtensionVersion();


/*****************************************************************************************
* getCPUInfo
*
*      Parses CPUID output to find the brand and vendor strings.
*
*****************************************************************************************/
void getCPUInfo(std::string* cpubrand, std::string* cpuvendor);


/*****************************************************************************************
* getGTGeneration
*
*      Returns the generation by parsing the device id. The first two digits of the hex
*      number generally denote the generation. Sandybridge and Ivybridge share the same
*      numbers so they must be further parsed.
*
*      Comparison of the deviceIds (for example to see if a device is more recent than
*      another) does not always work.
*
*****************************************************************************************/
PRODUCT_FAMILY getGTGeneration(unsigned int deviceId);




#define GGF_SUCCESS 0
#define GGF_ERROR					-1
#define GGF_E_UNSUPPORTED_HARDWARE	-2
#define GGF_E_UNSUPPORTED_DRIVER	-3
#define GGF_E_D3D_ERROR				-4