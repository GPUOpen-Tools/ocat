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
// DeviceId.cpp : Implements the GPU Device detection and graphics settings
//                configuration functions.
//

#include <wrl.h>
#include "DeviceId.h"
#include <stdio.h>
#include <string>

#include <InitGuid.h>
#include <D3D11.h>
#include <D3DCommon.h>
#ifdef _WIN32_WINNT_WIN10 
#include <DXGI1_4.h>
#include <d3d11_3.h>
#else
#include <DXGI1_3.h>
#endif

#include <oleauto.h>
#include <wbemidl.h>
#include <ObjBase.h>

/*****************************************************************************************
 * getGraphicsDeviceInfo
 *
 *     Function to retrieve information about the primary graphics driver. This includes
 *     the device's Vendor ID, Device ID, and available memory.
 *
 *****************************************************************************************/

bool getGraphicsDeviceInfo( unsigned int* VendorId,
							unsigned int* DeviceId,
							unsigned __int64* VideoMemory,
							std::wstring* GFXBrand)
{
	if ((VendorId == NULL) || (DeviceId == NULL)) {
		return false;
	}

	//
	// DXGI is supported on Windows Vista and later. Define a function pointer to the
	// CreateDXGIFactory function. DXGIFactory1 is supported by Windows Store Apps so
	// try that first.
	//
	HMODULE hDXGI = LoadLibrary(L"dxgi.dll");
	if (hDXGI == NULL) {
		return false;
	}

	typedef HRESULT(WINAPI*LPCREATEDXGIFACTORY)(REFIID riid, void** ppFactory);

	LPCREATEDXGIFACTORY pCreateDXGIFactory = (LPCREATEDXGIFACTORY)GetProcAddress(hDXGI, "CreateDXGIFactory1");
	if (pCreateDXGIFactory == NULL) {
		pCreateDXGIFactory = (LPCREATEDXGIFACTORY)GetProcAddress(hDXGI, "CreateDXGIFactory");

		if (pCreateDXGIFactory == NULL) {
			FreeLibrary(hDXGI);
			return false;
		}
	}

	//
	// We have the CreateDXGIFactory function so use it to actually create the factory and enumerate
	// through the adapters. Here, we are specifically looking for the Intel gfx adapter. 
	//
	IDXGIAdapter*     pAdapter;
	IDXGIFactory*     pFactory;
	DXGI_ADAPTER_DESC AdapterDesc;
	if (FAILED((*pCreateDXGIFactory)(__uuidof(IDXGIFactory), (void**)(&pFactory)))) {
		FreeLibrary(hDXGI);
		return false;
	}

	if (FAILED(pFactory->EnumAdapters(0, (IDXGIAdapter**)&pAdapter))) {
		FreeLibrary(hDXGI);
		return false;
	}

	unsigned int intelAdapterIndex = 0;
	while (pFactory->EnumAdapters(intelAdapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
		pAdapter->GetDesc(&AdapterDesc);
		if (AdapterDesc.VendorId == 0x8086) {
			break;
		}
		intelAdapterIndex++;
	}

	if (pAdapter == NULL) {
		FreeLibrary(hDXGI);
		return false;
	}
	*VendorId = AdapterDesc.VendorId;
	*DeviceId = AdapterDesc.DeviceId;
	*GFXBrand = AdapterDesc.Description;

	//
	// On UMA hardware like Intel iGPUs, all graphics memory is shared. The dedicated
	// value is populated by a default 128 dummy value in Intel drivers to prevent crashes in 
	// legacy apps which aren't aware of UMA hardware (other vendors may use a different threshold). 
	//
	// If we are using DX11.3 the FeatureDataOptions2 interface should be available. Checking the UMA flag 
	// is recommended over comparing dedicated against a UMA threshold users can regedit override.
	//
	HRESULT hr = E_FAIL;
	*VideoMemory = 0;

#ifdef _WIN32_WINNT_WIN10
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice = NULL;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext = NULL;
	D3D_FEATURE_LEVEL featureLevel;
	ZeroMemory(&featureLevel, sizeof(D3D_FEATURE_LEVEL));
			
	if (SUCCEEDED(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, NULL, NULL, NULL,
		D3D11_SDK_VERSION, &pDevice, &featureLevel, &pImmediateContext))) {

		D3D11_FEATURE_DATA_D3D11_OPTIONS2 FeatureData;
		ZeroMemory(&FeatureData, sizeof(FeatureData));
		Microsoft::WRL::ComPtr<ID3D11Device3> pDevice3;

		//
		// Set the maximum available video memory
		//
		if (SUCCEEDED(pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&pDevice3))) {
			if (SUCCEEDED(pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &FeatureData, sizeof(FeatureData)))) {
				*VideoMemory = (unsigned __int64)((FeatureData.UnifiedMemoryArchitecture == FALSE) ?
					AdapterDesc.DedicatedVideoMemory : AdapterDesc.SharedSystemMemory);
			}
		}
	}
#endif // _WIN32_WINNT_WIN10

	if (hr == E_FAIL || *VideoMemory <= 0) // Pre-DX11.3 fallback
	{
		//
		// We check whether dedicated is greater than a threshold to determine if we're on a UMA GPU.
		// NOTE: This isn't 100% foolproof. Users can override this value to anywhere from 0 and 512MB 
		// with the reg key: HKEY_LOCAL_MACHINE\Software\Intel\GMM\DedicatedSegmentSize
		//
		const int ASSUME_UMA_THRESHOLD = 512 * (1024 * 1024);
		*VideoMemory = (unsigned __int64)((AdapterDesc.DedicatedVideoMemory >= ASSUME_UMA_THRESHOLD) ?
			AdapterDesc.DedicatedVideoMemory : AdapterDesc.SharedSystemMemory);
	}

	pAdapter->Release();
	FreeLibrary(hDXGI);
	return true;
}

/******************************************************************************************************************************************
 * getIntelDeviceInfo
 *
 * Description:
 *       Gets device information which is stored in a D3D counter. First, a D3D device must be created, the Intel counter located, and
 *       finally queried.
 *
 *       Supported device info: GPU Max Frequency, GPU Min Frequency, GT Generation, EU Count, Package TDP, Max Fill Rate
 * 
 * Parameters:
 *         unsigned int VendorId                         - [in]     - Input:  system's vendor id
 *         IntelDeviceInfoHeader *pIntelDeviceInfoHeader - [in/out] - Input:  allocated IntelDeviceInfoHeader *
 *                                                                    Output: Intel device info header, if found
 *         void *pIntelDeviceInfoBuffer                  - [in/out] - Input:  allocated void *
 *                                                                    Output: IntelDeviceInfoV[#], cast based on IntelDeviceInfoHeader
 * Return:
 *         GGF_SUCCESS: Able to find Data is valid
 *         GGF_E_UNSUPPORTED_HARDWARE: Unsupported hardware, data is invalid
 *         GGF_E_UNSUPPORTED_DRIVER: Unsupported driver on Intel, data is invalid
 *
 *****************************************************************************************************************************************/

long getIntelDeviceInfo( unsigned int VendorId, IntelDeviceInfoHeader *pIntelDeviceInfoHeader, void *pIntelDeviceInfoBuffer )
{
	if ( pIntelDeviceInfoBuffer == NULL ) {
		return GGF_ERROR;
	}

	if ( VendorId != INTEL_VENDOR_ID ) {
		return GGF_E_UNSUPPORTED_HARDWARE;
	}

	//
	// First create the Device, must be SandyBridge or later to create D3D11 device
	//
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice = NULL;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext = NULL;
	HRESULT hr = NULL;

	hr = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
							D3D11_SDK_VERSION, &pDevice, NULL, &pImmediateContext);

	if ( FAILED(hr) )
	{
		printf("D3D11CreateDevice failed\n");

		return GGF_ERROR;
	}
	
	//
	// Query the device to find the number of device dependent counters. If LastDeviceDependentCounter
	// is 0 then there are no device dependent counters supported on this device.
	//
	D3D11_COUNTER_INFO counterInfo;
	int numDependentCounters;

	ZeroMemory( &counterInfo, sizeof(D3D11_COUNTER_INFO) );

	pDevice->CheckCounterInfo( &counterInfo );

	if ( counterInfo.LastDeviceDependentCounter == 0 )
	{
		printf("No device dependent counters\n");

		return GGF_E_UNSUPPORTED_DRIVER;
	}

	numDependentCounters = (counterInfo.LastDeviceDependentCounter - D3D11_COUNTER_DEVICE_DEPENDENT_0) + 1;

	//
	// Since there is at least 1 counter, we search for the apporpriate counter - INTEL_DEVICE_INFO_COUNTERS
	//
	D3D11_COUNTER_DESC pIntelCounterDesc;
	UINT uiSlotsRequired, uiNameLength, uiUnitsLength, uiDescLength;
	LPSTR sName, sUnits, sDesc;

	ZeroMemory(&pIntelCounterDesc, sizeof(D3D11_COUNTER_DESC));
	
	for ( int i = 0; i < numDependentCounters; ++i )
	{
		D3D11_COUNTER_DESC counterDescription;
		D3D11_COUNTER_TYPE counterType;

		counterDescription.Counter = static_cast<D3D11_COUNTER>(i + D3D11_COUNTER_DEVICE_DEPENDENT_0);
		counterDescription.MiscFlags = 0;
		uiSlotsRequired = uiNameLength = uiUnitsLength = uiDescLength = 0;
		sName = sUnits = sDesc = NULL;

		if( SUCCEEDED( hr = pDevice->CheckCounter( &counterDescription, &counterType, &uiSlotsRequired, NULL, &uiNameLength, NULL, &uiUnitsLength, NULL, &uiDescLength ) ) )
		{
			if (uiNameLength == 0 || uiUnitsLength == 0 || uiDescLength == 0)
			{
				return GGF_ERROR;
			}

			LPSTR sName  = new char[uiNameLength];
			LPSTR sUnits = new char[uiUnitsLength];
			LPSTR sDesc  = new char[uiDescLength];
			
			if( SUCCEEDED( hr = pDevice->CheckCounter( &counterDescription, &counterType, &uiSlotsRequired, sName, &uiNameLength, sUnits, &uiUnitsLength, sDesc, &uiDescLength ) ) )
			{
				if ( strcmp( sName, INTEL_DEVICE_INFO_COUNTERS ) == 0 )
				{
					int IntelCounterMajorVersion = 0;
					int IntelCounterSize = 0;

					pIntelCounterDesc.Counter = counterDescription.Counter;

					sscanf_s( sDesc, "Version %d", &IntelCounterMajorVersion);
					
					if (IntelCounterMajorVersion == 2 )
					{
						IntelCounterSize = sizeof( IntelDeviceInfoV2 );
					}
					else // Fall back to version 1.0
					{
						IntelCounterMajorVersion = 1;
						IntelCounterSize = sizeof( IntelDeviceInfoV1 );
					}

					pIntelDeviceInfoHeader->Version = IntelCounterMajorVersion;
					pIntelDeviceInfoHeader->Size = IntelCounterSize;
				}
			}
			
			delete[] sName;
			delete[] sUnits;
			delete[] sDesc;
		}
	}

	//
	// Check if the Device Info Counter was found. If not, then it means
	// the driver doesn't support this counter.
	//
	if ( pIntelCounterDesc.Counter == NULL )
	{
		printf("Could not find counter\n");

		return GGF_E_UNSUPPORTED_DRIVER;
	}
	
	//
	// Now we know the driver supports the counter we are interested in. So create it and
	// capture the data we want.
	//
	Microsoft::WRL::ComPtr<ID3D11Counter> pIntelCounter = NULL;

	hr = pDevice->CreateCounter(&pIntelCounterDesc, &pIntelCounter);
	if ( FAILED(hr) )
	{
		printf("CreateCounter failed\n");

		return GGF_E_D3D_ERROR;
	}

	pImmediateContext->Begin(pIntelCounter.Get());
	pImmediateContext->End(pIntelCounter.Get());

	hr = pImmediateContext->GetData( pIntelCounter.Get(), NULL, 0, 0 );
	if ( FAILED(hr) || hr == S_FALSE )
	{
		printf("Getdata failed \n");

		return GGF_E_D3D_ERROR;
	}
	
	//
	// GetData will return the address of the data, not the actual data.
	//
	DWORD pData[2] = {0};
	hr = pImmediateContext->GetData(pIntelCounter.Get(), pData, 2 * sizeof(DWORD), 0);
	if ( FAILED(hr) || hr == S_FALSE )
	{
		printf("Getdata failed \n");
		return GGF_E_D3D_ERROR;
	}

	//
	// Copy data to passed in parameter and clean up
	//
	void *pDeviceInfoBuffer = *(void**)pData;
	memcpy( pIntelDeviceInfoBuffer, pDeviceInfoBuffer, pIntelDeviceInfoHeader->Size );

	return GGF_SUCCESS;
}

/*****************************************************************************************
* getCPUInfo
*
*      Parses CPUID output to find the brand and vendor strings.
*
*****************************************************************************************/

void getCPUInfo(std::string* cpubrand, std::string* cpuvendor)
{

	// Get extended ids.
	int CPUInfo[4] = { -1 };
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];

	// Get the information associated with each extended ID.
	char CPUBrandString[0x40] = { 0 };
	char CPUVendorString[0x40] = { 0 };

	__cpuid(CPUInfo, 0);
	memcpy_s(CPUVendorString, sizeof(CPUInfo), &CPUInfo[1], sizeof(int));
	memcpy_s(CPUVendorString + 4, sizeof(CPUInfo), &CPUInfo[3], sizeof(int));
	memcpy_s(CPUVendorString + 8, sizeof(CPUInfo), &CPUInfo[2], sizeof(int));
	*cpuvendor = CPUVendorString;

	for (unsigned int i = 0x80000000; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);

		if (i == 0x80000002)
		{

			memcpy_s(CPUBrandString, sizeof(CPUInfo), CPUInfo, 4 * sizeof(int));
		}
		else if (i == 0x80000003)
		{
			memcpy_s(CPUBrandString + 16, sizeof(CPUInfo), CPUInfo, 4 * sizeof(int));
		}
		else if (i == 0x80000004)
		{
			memcpy_s(CPUBrandString + 32, sizeof(CPUInfo), CPUInfo, 4 * sizeof(int));
		}
	}
	*cpubrand = CPUBrandString;

}

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

PRODUCT_FAMILY getGTGeneration(unsigned int deviceId)
{
	unsigned int maskedDeviceId = deviceId & 0xFF00;

	//
	// Device is Sandybridge or Ivybridge
	//
	if (maskedDeviceId == 0x0100) {
		if ( 
			((deviceId & 0x0050) == 0x0050) ||
			((deviceId & 0x0060) == 0x0060)
		   ) {
			return IGFX_IVYBRIDGE;
			}
		if (
			((deviceId & 0x0010) == 0x0010) ||
			((deviceId & 0x00F0) == 0x0000)
			) {
			return IGFX_SANDYBRIDGE;
		}
	}

	if (maskedDeviceId == 0x0400 || maskedDeviceId == 0x0A00 || maskedDeviceId == 0x0D00) {
		return IGFX_HASWELL;
	}

	if (maskedDeviceId == 0x1600) {
		return IGFX_BROADWELL;
	}

	if (maskedDeviceId == 0x1900) {
		return IGFX_SKYLAKE;
	}
	
	if (maskedDeviceId == 0x5900) {
		return IGFX_KABYLAKE;
	}

	if (maskedDeviceId == 0x3E00) {
		return IGFX_COFFEELAKE;
	}
	
	return IGFX_UNKNOWN;
}

#include "ID3D10Extensions.h"
UINT checkDxExtensionVersion( )
{
    UINT extensionVersion = 0;
    ID3D10::CAPS_EXTENSION intelExtCaps;
    Microsoft::WRL::ComPtr<ID3D11Device> pDevice = NULL;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pImmediateContext = NULL;
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = NULL;

	ZeroMemory( &featureLevel, sizeof(D3D_FEATURE_LEVEL) );

	ID3D11DeviceContext **pContext = &pImmediateContext;

	// First create the Device
	hr = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
							D3D11_SDK_VERSION, &pDevice, &featureLevel, &pImmediateContext);

	if ( FAILED(hr) )
	{
		printf("D3D11CreateDevice failed\n");
	}
	ZeroMemory( &intelExtCaps, sizeof(ID3D10::CAPS_EXTENSION) );

	if ( pDevice )
	{
		if( S_OK == GetExtensionCaps( pDevice.Get(), &intelExtCaps ) )
		{
			extensionVersion = intelExtCaps.DriverVersion;
		}
	}

	return extensionVersion;
}