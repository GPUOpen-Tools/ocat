/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#include <conio.h>
#include <stdio.h>
#include "DeviceId.h"
#include <string>

#include "D3D11.h"
#include "ID3D10Extensions.h"
#include <intrin.h>


// Define settings to reflect Fidelity abstraction levels you need
typedef enum
{
	NotCompatible,  // Found GPU is not compatible with the app
	Low,
	Medium,
	MediumPlus,
	High,
	Undefined  // No predefined setting found in cfg file. 
			   // Use a default level for unknown video cards.
}
PresetLevel;

const char *PRODUCT_FAMILY_STRING[] =
{
	"Sandy Bridge",
	"Ivy Bridge",
	"Haswell",
	"ValleyView",
	"Broadwell",
	"Cherryview",
	"Skylake",
	"Kabylake",
	"Coffeelake"
};
const unsigned int NUM_PRODUCT_FAMILIES = sizeof(PRODUCT_FAMILY_STRING) / sizeof(PRODUCT_FAMILY_STRING[0]);

PresetLevel getDefaultFidelityPresets(unsigned int VendorId, unsigned int DeviceId);

/*****************************************************************************************
* main
*
*     Function represents the game or application. The application checks for graphics
*     capabilities here and makes whatever decisions it needs to based on the results.
*
*****************************************************************************************/
int main( int argc, char* argv[] )
{
	//
	// First we need to check if it is an Intel processor. If not then an alternate
	// path should be taken. This is also a good place to check for other CPU capabilites
	// such as AVX support.
	//
	std::string CPUBrand;
	std::string CPUVendor;

	getCPUInfo(&CPUBrand, &CPUVendor);
	if (CPUVendor != "GenuineIntel") {
		printf("Not an Intel CPU");
		_getch();
		return 0;
	}

	//
	// The brand string can be parsed to discover some information about the type of
	// processor.
	//
	printf( "CPU Brand: %s\n", CPUBrand.c_str() );

	std::size_t found = CPUBrand.find("i7");
	if (found != std::string::npos)
	{
		printf( "i7 Brand Found\n" );
	}

	found = CPUBrand.find("i5");
	if (found != std::string::npos)
	{
		printf( "i5 Brand Found\n" );
	}

	found = CPUBrand.find("i3");
	if (found != std::string::npos)
	{
		printf( "i3 Brand Found\n" );
	}

	found = CPUBrand.find("Celeron");
	if (found != std::string::npos)
	{
		printf( "Celeron Brand Found\n" );
	}

	found = CPUBrand.find("Pentium");
	if (found != std::string::npos)
	{
		printf( "Pentium Brand Found\n" );
	}

	//
	// Some information about the gfx adapters is exposed through Windows and DXGI. If
	// the machine has multiple gfx adapters or no Intel gfx this is where that can be
	// detected.
	//
	unsigned int VendorId, DeviceId;
	unsigned __int64 VideoMemory;
	std::wstring GFXBrand;

	if (false == getGraphicsDeviceInfo(&VendorId, &DeviceId, &VideoMemory, &GFXBrand))
	{
		printf("Intel Graphics Adapter not detected\n");
		return -1;
	}

	printf("Primary Graphics Device\n");
	printf("-----------------------\n");
	printf("Vendor( 0x%x ) : Device ( 0x%x )\n", VendorId, DeviceId);
	printf("Video Memory: %I64u MB\n", VideoMemory / (1024 * 1024));
	printf("Graphics Brand: %S\n", GFXBrand.c_str());

	//
	// Similar to the CPU brand, we can also parse the GPU brand string for information like whether
	// is an Iris or Iris Pro part.
	//
	std::string TempGPUBrand;
	char* ConvertArray = new char[(int)GFXBrand.size()];
	WideCharToMultiByte(CP_UTF8, 0, &GFXBrand[0], (int)GFXBrand.size(), &ConvertArray[0], (int)GFXBrand.size(), NULL, NULL);
	TempGPUBrand = ConvertArray;
	found = TempGPUBrand.find("Iris");
	if (found != std::string::npos)
	{
		found = TempGPUBrand.find("Pro");
		if (found != std::string::npos)
		{
			printf("Iris Pro Graphics Brand Found\n");
		}
		else
		{
			printf("Iris Graphics Brand Found\n");
		}
	}
	delete[] ConvertArray;

	//
	// The deviceID can be used as an index into the configuration file to load other information.
	//
	PresetLevel defPresets;
	defPresets = getDefaultFidelityPresets(VendorId, DeviceId);

	if (defPresets == NotCompatible) {
		printf("Default Presets = NotCompatible\n");
	}
	else if (defPresets == Low) {
		printf("Default Presets = Low\n");
	}
	else if (defPresets == Medium) {
		printf("Default Presets = Medium\n");
	}
	else if (defPresets == MediumPlus) {
		printf("Default Presets = Medium+\n");
	}
    printf("\n");

	//
    // Check for DX extensions
	//
    unsigned int extensionVersion = checkDxExtensionVersion();
	if (extensionVersion >= ID3D10::EXTENSION_INTERFACE_VERSION_1_0) {
		printf("Supports Intel Iris Graphics extensions: \n\tpixel sychronization \n\tinstant access of graphics memory\n\n");
	}
	else {
		printf("Does not support Intel Iris Graphics extensions\n\n");
	}

	//
	// In DirectX, Intel exposes additional information through the driver that can be obtained
	// querying a special DX counter
	//
	IntelDeviceInfoHeader intelDeviceInfoHeader = { 0 };
	unsigned char intelDeviceInfoBuffer[1024];

	long getStatus = getIntelDeviceInfo( VendorId, &intelDeviceInfoHeader, &intelDeviceInfoBuffer );

	if ( getStatus == GGF_SUCCESS )
	{
		printf("Intel Device Info Version  %d (%d bytes)\n", intelDeviceInfoHeader.Version, intelDeviceInfoHeader.Size );

		if ( intelDeviceInfoHeader.Version == 2 )
		{
			IntelDeviceInfoV2 intelDeviceInfo;

			memcpy( &intelDeviceInfo, intelDeviceInfoBuffer, intelDeviceInfoHeader.Size );

			int myGeneration = intelDeviceInfo.GTGeneration;
			int generationIndex = myGeneration - IGFX_SANDYBRIDGE;
			const char *myGenerationString;

			if (myGeneration < IGFX_SANDYBRIDGE) {
				myGenerationString = "Old unrecognized generation";
			}
			else if (myGeneration > (NUM_PRODUCT_FAMILIES + IGFX_SANDYBRIDGE)) {
				myGenerationString = "New unrecognized generation";
			}
			else {
				myGenerationString = PRODUCT_FAMILY_STRING[generationIndex];
			}

			printf("GT Generation:             %s (0x%x)\n", myGenerationString, intelDeviceInfo.GTGeneration );
			printf("GPU Max Frequency:         %d mhz\n", intelDeviceInfo.GPUMaxFreq );
			printf("GPU Min Frequency:         %d mhz\n", intelDeviceInfo.GPUMinFreq );
			printf("EU Count:                  %d\n", intelDeviceInfo.EUCount );
			printf("Package TDP:               %d watts\n", intelDeviceInfo.PackageTDP );
			printf("Max Fill Rate:             %d pixel/clk\n", intelDeviceInfo.MaxFillRate );
		}
		else if (intelDeviceInfoHeader.Version == 1)
		{
			IntelDeviceInfoV1 intelDeviceInfo;

			memcpy( &intelDeviceInfo, intelDeviceInfoBuffer, intelDeviceInfoHeader.Size );

			printf("GPU Max Frequency:         %d mhz\n", intelDeviceInfo.GPUMaxFreq );
			printf("GPU Min Frequency:         %d mhz\n", intelDeviceInfo.GPUMinFreq );
		}
		else
		{
			printf("Unrecognized Intel Device Header version\n");
		}
	}
	else if ( getStatus == GGF_E_UNSUPPORTED_HARDWARE )
	{
		printf("GPU min/max turbo frequency: unknown\n");
		printf("GPU does not support frequency query\n");
	}
	else if( getStatus == GGF_E_UNSUPPORTED_DRIVER )
	{
		printf("Intel device information: unknown\n");
		printf("Driver does not support device information query\n");
	}
	else
	{
		printf("Intel device information: unknown\n");
		printf("Error retrieving frequency information\n");
    }

    printf("\n");
    printf("Press any key to exit...\n");

	_getch();
    return 0;
}

/*****************************************************************************************
* getDefaultFidelityPresets
*
*     Function to find the default fidelity preset level, based on the type of
*     graphics adapter present.
*
*     The guidelines for graphics preset levels for Intel devices is a generic one
*     based on our observations with various contemporary games. You would have to
*     change it if your game already plays well on the older hardware even at high
*     settings.
*
*****************************************************************************************/

PresetLevel getDefaultFidelityPresets(unsigned int VendorId, unsigned int DeviceId)
{
	//
	// Look for a config file that qualifies devices from any vendor
	// The code here looks for a file with one line per recognized graphics
	// device in the following format:
	//
	// VendorIDHex, DeviceIDHex, CapabilityEnum      ;Commented name of card
	//

	FILE *fp = NULL;
	const char *cfgFileName;
	cfgFileName = "";

	switch (VendorId)
	{
	case 0x8086:
		cfgFileName = "IntelGfx.cfg";
		break;
		//case 0x1002:
		//    cfgFileName =  "ATI.cfg"; // not provided
		//    break;

		//case 0x10DE:
		//    cfgFileName = "Nvidia.cfg"; // not provided
		//    break;

	default:
		break;
	}

	fopen_s(&fp, cfgFileName, "r");

	PresetLevel presets = Undefined;

	if (fp)
	{
		char line[100];
		char* context = NULL;

		char* szVendorId = NULL;
		char* szDeviceId = NULL;
		char* szPresetLevel = NULL;

		//
		// read one line at a time till EOF
		//
		while (fgets(line, 100, fp))
		{
			//
			// Parse and remove the comment part of any line
			//
			int i; for (i = 0; line[i] && line[i] != ';'; i++); line[i] = '\0';

			//
			// Try to extract VendorId, DeviceId and recommended Default Preset Level
			//
			szVendorId = strtok_s(line, ",\n", &context);
			szDeviceId = strtok_s(NULL, ",\n", &context);
			szPresetLevel = strtok_s(NULL, ",\n", &context);

			if ((szVendorId == NULL) ||
				(szDeviceId == NULL) ||
				(szPresetLevel == NULL))
			{
				continue;  // blank or improper line in cfg file - skip to next line
			}

			unsigned int vId, dId;
			sscanf_s(szVendorId, "%x", &vId);
			sscanf_s(szDeviceId, "%x", &dId);

			//
			// If current graphics device is found in the cfg file, use the 
			// pre-configured default Graphics Presets setting.
			//
			if ((vId == VendorId) && (dId == DeviceId))
			{
				char s[10];
				sscanf_s(szPresetLevel, "%s", s, _countof(s));

				if (!_stricmp(s, "Low"))
					presets = Low;
				else if (!_stricmp(s, "Medium"))
					presets = Medium;
				else if (!_stricmp(s, "Medium+"))
					presets = MediumPlus;
				else if (!_stricmp(s, "High"))
					presets = High;
				else
					presets = NotCompatible;

				break;
			}
		}

		fclose(fp);  // Close open file handle
	}
	else
	{
		printf("%s not found! Presets undefined.\n", cfgFileName);
	}

	//
	// If the current graphics device was not listed in any of the config
	// files, or if config file not found, use Low settings as default.
	// This should be changed to reflect the desired behavior for unknown
	// graphics devices.
	//
	if (presets == Undefined) {
		presets = Low;
	}

	return presets;
}
