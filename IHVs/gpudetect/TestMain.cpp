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

#ifndef STRICT
#define STRICT
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>
#ifdef _WIN32_WINNT_WIN10
#include <d3d11_3.h>
#endif

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "GPUDetect.h"


// For parsing arguments
bool isnumber( char* s )
{
	for( ; *s != '\0'; ++s )
	{
		if( !isdigit( *s ) )
		{
			return false;
		}
	}
	return true;
}

/*******************************************************************************
 * printError
 *
 *     Prints the error on screen in a nice format.
 *
 ******************************************************************************/
void printError( int errorCode )
{
	fprintf( stderr, "Error: " );

	if( errorCode % GPUDETECT_ERROR_GENERAL_DXGI == 0 )
	{
		fprintf( stderr, "DXGI: " );
	}

	if( errorCode % GPUDETECT_ERROR_GENERAL_DXGI_COUNTER == 0 )
	{
		fprintf( stderr, "DXGI Counter: " );
	}

	if( errorCode % GPUDETECT_ERROR_REG_GENERAL_FAILURE == 0 )
	{
		fprintf( stderr, "Registry: " );
	}

	switch( errorCode )
	{
	case GPUDETECT_ERROR_DXGI_LOAD:
		fprintf( stderr, "Could not load DXGI Library\n" );
		break;

	case GPUDETECT_ERROR_DXGI_ADAPTER_CREATION:
		fprintf( stderr, "Could not create DXGI adapter\n" );
		break;

	case GPUDETECT_ERROR_DXGI_FACTORY_CREATION:
		fprintf( stderr, "Could not create DXGI factory\n" );
		break;

	case GPUDETECT_ERROR_DXGI_DEVICE_CREATION:
		fprintf( stderr, "Could not create DXGI device\n" );
		break;

	case GPUDETECT_ERROR_DXGI_GET_ADAPTER_DESC:
		fprintf( stderr, "Could not get DXGI adapter\n" );
		break;

	case GPUDETECT_ERROR_DXGI_BAD_COUNTER:
		fprintf( stderr, "Invalid DXGI counter data\n" );
		break;

	case GPUDETECT_ERROR_DXGI_COUNTER_CREATION:
		fprintf( stderr, "Could not create DXGI counter\n" );
		break;

	case GPUDETECT_ERROR_DXGI_COUNTER_GET_DATA:
		fprintf( stderr, "Could not get DXGI counter data\n" );
		break;

	case GPUDETECT_ERROR_REG_NO_D3D_KEY:
		fprintf( stderr, "D3D driver info was not located in the expected location in the registry\n" );
		break;

	case GPUDETECT_ERROR_REG_MISSING_DRIVER_INFO:
		fprintf( stderr, "Could not find a D3D driver matching the device ID and vendor ID of this adapter\n" );
		break;

	case GPUDETECT_ERROR_BAD_DATA:
		fprintf( stderr, "Bad input data for function or precondition not met\n" );
		break;

	case GPUDETECT_ERROR_NOT_SUPPORTED:
		fprintf( stderr, "Not supported\n" );
		break;

	default:
		fprintf( stderr, "Unknown error\n" );
		break;
	}
}

/*******************************************************************************
 * main
 *
 *     Function represents the game or application. The application checks for
 *     graphics capabilities here and makes whatever decisions it needs to
 *     based on the results.
 *
 ******************************************************************************/
int main( int argc, char** argv )
{
	fprintf( stdout, "\n\n[ Intel GPUDetect ]\n" );
	fprintf( stdout, "Build Info: %s, %s\n", __DATE__, __TIME__ );

	int adapterIndex = 0;

	if( argc == 1 )
	{
		fprintf( stdout, "Usage: GPUDetect adapter_index\n" );
		fprintf( stdout, "Defaulting to adapter_index = %d\n", adapterIndex );
	}
	else if( argc == 2 && isnumber( argv[ 1 ] ))
	{
		adapterIndex = atoi( argv[ 1 ] );
		fprintf( stdout, "Choosing adapter_index = %d\n", adapterIndex );
	}
	else
	{
		fprintf( stdout, "Usage: GPUDetect adapter_index\n" );
		fprintf( stderr, "Error: unexpected arguments.\n" );
		return EXIT_FAILURE;
	}
	fprintf( stdout,  "\n" );


	IDXGIAdapter* adapter = nullptr;
	int initReturnCode = GPUDetect::InitAdapter( &adapter, adapterIndex );
	if( initReturnCode != EXIT_SUCCESS )
	{
		printError( initReturnCode );
		return EXIT_FAILURE;
	};

	ID3D11Device* device = nullptr;
	initReturnCode = GPUDetect::InitDevice( adapter, &device );
	if( initReturnCode != EXIT_SUCCESS )
	{
		printError( initReturnCode );
		adapter->Release();
		return EXIT_FAILURE;
	};

	GPUDetect::GPUData gpuData = {};
	initReturnCode = GPUDetect::InitExtensionInfo( &gpuData, adapter, device );
	if( initReturnCode != EXIT_SUCCESS )
	{
		printError( initReturnCode );
	}
	else
	{
		fprintf( stdout, "Adapter #%d\n", adapterIndex );
		fprintf( stdout, "-----------------------\n" );
		fprintf( stdout, "VendorID: 0x%x\n", gpuData.vendorID );
		fprintf( stdout, "DeviceID: 0x%x\n", gpuData.deviceID );
		fprintf( stdout, "Video Memory: %I64u MB\n", gpuData.videoMemory / ( 1024 * 1024 ) );
		fprintf( stdout, "Description: %S\n", gpuData.description );
		fprintf( stdout, "\n" );

		//
		//  Find and print driver version information
		//
		initReturnCode = GPUDetect::InitDxDriverVersion( &gpuData );
		if( gpuData.d3dRegistryDataAvailability )
		{
			fprintf( stdout, "\nDriver Information\n" );
			fprintf( stdout, "-----------------------\n" );

			char driverVersion[ 19 ] = {};
			GPUDetect::GetDriverVersionAsCString( &gpuData, driverVersion, _countof(driverVersion) );
			fprintf( stdout, "Driver Version: %s\n", driverVersion );

			// Print out decoded data
			fprintf( stdout, "Release Revision: %u\n", gpuData.driverInfo.driverReleaseRevision );
			fprintf( stdout, "Build Number: %u\n", gpuData.driverInfo.driverBuildNumber );
			fprintf( stdout, "\n" );
		}

		if( gpuData.vendorID == GPUDetect::INTEL_VENDOR_ID )
		{

# if 0
			//
			// This sample includes a .cfg file that maps known vendor and device IDs
			// to example quality presets.  This looks up the preset for the IDs
			// queried above.
			//
			const GPUDetect::PresetLevel defPresets = GPUDetect::GetDefaultFidelityPreset( &gpuData );
			switch( defPresets )
			{
				case GPUDetect::PresetLevel::NotCompatible: fprintf( stdout, "Default Fidelity Preset Level: NotCompatible\n" ); break;
				case GPUDetect::PresetLevel::Low:           fprintf( stdout, "Default Fidelity Preset Level: Low\n" );           break;
				case GPUDetect::PresetLevel::Medium:        fprintf( stdout, "Default Fidelity Preset Level: Medium\n" );        break;
				case GPUDetect::PresetLevel::MediumPlus:    fprintf( stdout, "Default Fidelity Preset Level: Medium+\n" );       break;
				case GPUDetect::PresetLevel::High:          fprintf( stdout, "Default Fidelity Preset Level: High\n" );          break;
				case GPUDetect::PresetLevel::Undefined:     fprintf( stdout, "Default Fidelity Preset Level: Undefined\n" );     break;
			}

			fprintf( stdout, "\n" );
#endif


			//
			// Check if Intel DirectX extensions are available on this system.
			//
			if( gpuData.intelExtensionAvailability )
			{
				fprintf( stdout, "Supports Intel Iris Graphics extensions:\n" );
				fprintf( stdout, "\tpixel synchronization\n" );
				fprintf( stdout, "\tinstant access of graphics memory\n" );
			}
			else
			{
				fprintf( stdout, "Does not support Intel Iris Graphics extensions\n" );
			}

			fprintf( stdout, "\n" );


			//
			// In DirectX, Intel exposes additional information through the driver that can be obtained
			// querying a special DX counter
			//

			// Populate the GPU architecture data with info from the counter, otherwise gpuDetect will use the value we got from the Dx11 extension
			initReturnCode = GPUDetect::InitCounterInfo( &gpuData, device );
			if( initReturnCode != EXIT_SUCCESS )
			{
				printError( initReturnCode );
			}
			else
			{
				fprintf( stdout, "Architecture (from DeviceID): %s\n", GPUDetect::GetIntelGPUArchitectureString( gpuData.architecture ));
				fprintf( stdout, "using %s graphics\n", GPUDetect::GetIntelGraphicsGenerationString( GPUDetect::GetIntelGraphicsGeneration( gpuData.architecture )) );

				//
				// Older versions of the IntelDeviceInfo query only return
				// GPUMaxFreq and GPUMinFreq, all other members will be zero.
				//
				if( gpuData.advancedCounterDataAvailability )
				{
					fprintf( stdout, "EU Count:          %u\n", gpuData.euCount );
					fprintf( stdout, "Package TDP:       %u W\n", gpuData.packageTDP );
					fprintf( stdout, "Max Fill Rate:     %u pixels/clock\n", gpuData.maxFillRate );
				}

				fprintf( stdout, "GPU Max Frequency: %u MHz\n", gpuData.maxFrequency );
				fprintf( stdout, "GPU Min Frequency: %u MHz\n", gpuData.minFrequency );
			}

			fprintf( stdout, "\n" );
		}
	}

	device->Release();
	adapter->Release();

	fprintf( stdout, "\n" );

	return 0;
}
