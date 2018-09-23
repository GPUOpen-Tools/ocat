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

#include <wrl.h>
#include "dxgi_swapchain.hpp"
#include "Logging/MessageLog.h"
#include <dxgi1_6.h>

using namespace Microsoft::WRL;

template<typename WrappedType, typename BaseType>
HRESULT QueryWrappedInterface(IUnknown * unknown, REFIID riid, void ** ppvObject)
{
  BaseType* unmodifiedAdapter;
  HRESULT hr = unknown->QueryInterface(riid, (void**)&unmodifiedAdapter);
  WrappedType* wrappedAdapter = new WrappedType(unmodifiedAdapter);
  *ppvObject = wrappedAdapter;
  return hr;
}

class WrappedIUnknown
{
public:
  WrappedIUnknown(const std::string& name, IUnknown* unknown);
  void LogUUID(REFIID riid, const std::wstring& message);
  void LogWrapped(IUnknown* unknown);
  void Log(const std::string& message);
  void Log(const std::wstring& message);

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef(void);
  virtual ULONG STDMETHODCALLTYPE Release(void);

  template<typename DXGIType>
  DXGIType* Get()
  {
    return static_cast<DXGIType*>(unknown_);
  }

private:
  std::string name_;
  IUnknown* unknown_;
};

class WrappedIDXGIObject : public WrappedIUnknown
{
public:
  WrappedIDXGIObject(const std::string& name, IDXGIObject* object);
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void * pData);
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown * pUnknown);
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT * pDataSize, void * pData);
  virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void ** ppParent);
};

class WrappedIDXGIDeviceSubObject : public WrappedIDXGIObject
{
public:
  WrappedIDXGIDeviceSubObject(const std::string& name, IDXGIDeviceSubObject* object);
  virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice);
};

class WrappedIDXGIAdapter : public WrappedIDXGIObject
{
public:
  WrappedIDXGIAdapter(IDXGIAdapter* adapter);
  WrappedIDXGIAdapter(const std::string& name, IDXGIAdapter* adapter);
  virtual HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, IDXGIOutput ** ppOutput);
  virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_ADAPTER_DESC * pDesc);
  virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER * pUMDVersion);
};

class WrappedIDXGIAdapter1 : public WrappedIDXGIAdapter
{
public:
  WrappedIDXGIAdapter1(IDXGIAdapter1* adapter);
  WrappedIDXGIAdapter1(const std::string& name, IDXGIAdapter1* adapter);
  virtual HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_ADAPTER_DESC1 * pDesc);
};

class WrappedIDXGIAdapter2 : public WrappedIDXGIAdapter1
{
public:
  WrappedIDXGIAdapter2(IDXGIAdapter2* adapter);
  WrappedIDXGIAdapter2(const std::string& name, IDXGIAdapter2* adapter);
  virtual HRESULT STDMETHODCALLTYPE GetDesc2(DXGI_ADAPTER_DESC2 *pDesc);
};

class WrappedIDXGIAdapter3 : public WrappedIDXGIAdapter2
{
public:
  WrappedIDXGIAdapter3(IDXGIAdapter3* adapter);
  WrappedIDXGIAdapter3(const std::string& name, IDXGIAdapter3* adapter);
  virtual HRESULT STDMETHODCALLTYPE QueryVideoMemoryInfo(UINT NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, DXGI_QUERY_VIDEO_MEMORY_INFO *pVideoMemoryInfo);
  virtual HRESULT STDMETHODCALLTYPE RegisterHardwareContentProtectionTeardownStatusEvent(HANDLE hEvent, DWORD *pdwCookie);
  virtual HRESULT STDMETHODCALLTYPE RegisterVideoMemoryBudgetChangeNotificationEvent(HANDLE hEvent, DWORD *pdwCookie);
  virtual HRESULT STDMETHODCALLTYPE SetVideoMemoryReservation(UINT NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, UINT64 Reservation);
  virtual void STDMETHODCALLTYPE UnregisterHardwareContentProtectionTeardownStatus(DWORD dwCookie);
  virtual void STDMETHODCALLTYPE UnregisterVideoMemoryBudgetChangeNotification(DWORD dwCookie);
};

class WrappedIDXGIAdapter4 : public WrappedIDXGIAdapter3
{
public:
  WrappedIDXGIAdapter4(IDXGIAdapter4* adapter);
  WrappedIDXGIAdapter4(const std::string& name, IDXGIAdapter4* adapter);
  virtual HRESULT STDMETHODCALLTYPE GetDesc3(DXGI_ADAPTER_DESC3 *pDesc);
};

class WrappedIDXGIFactory : public WrappedIDXGIObject
{
public:
  WrappedIDXGIFactory(IDXGIFactory* factory);
  WrappedIDXGIFactory(const std::string& name, IDXGIFactory* factory);
  virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter ** ppAdapter);
  virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags);
  virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND * pWindowHandle);
  virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown * pDevice, DXGI_SWAP_CHAIN_DESC * pDesc, IDXGISwapChain ** ppSwapChain);
  virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter ** ppAdapter);
};

class WrappedIDXGIFactory1 : public WrappedIDXGIFactory
{
public:
  WrappedIDXGIFactory1(IDXGIFactory1* factory);
  WrappedIDXGIFactory1(const std::string& name, IDXGIFactory1* factory);
  virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, IDXGIAdapter1 ** ppAdapter);
  virtual BOOL STDMETHODCALLTYPE IsCurrent(void);
};

class WrappedIDXGIFactory2 : public WrappedIDXGIFactory1
{
public:
  WrappedIDXGIFactory2(IDXGIFactory2* factory);
  WrappedIDXGIFactory2(const std::string& name, IDXGIFactory2* factory);
  virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled(void);
  virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(IUnknown * pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 * pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain);
  virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(IUnknown * pDevice, IUnknown * pWindow, const DXGI_SWAP_CHAIN_DESC1 * pDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain);
  virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE hResource, LUID * pLuid);
  virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD * pdwCookie);
  virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE hEvent, DWORD * pdwCookie);
  virtual void STDMETHODCALLTYPE UnregisterStereoStatus(DWORD dwCookie);
  virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD * pdwCookie);
  virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD * pdwCookie);
  virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD dwCookie);
  virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(IUnknown * pDevice, const DXGI_SWAP_CHAIN_DESC1 * pDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain);
};

class WrappedIDXGIFactory3 : public WrappedIDXGIFactory2
{
public:
  WrappedIDXGIFactory3(IDXGIFactory3* factory);
  WrappedIDXGIFactory3(const std::string& name, IDXGIFactory3* factory);
  virtual UINT STDMETHODCALLTYPE GetCreationFlags();
};

class WrappedIDXGIFactory4 : public WrappedIDXGIFactory3
{
public:
  WrappedIDXGIFactory4(IDXGIFactory4* factory);
  WrappedIDXGIFactory4(const std::string& name, IDXGIFactory4* factory);
  virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID AdapterLuid, REFIID riid, void **ppvAdapter);
  virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID riid, void **ppvAdapter);
};

class WrappedIDXGIFactory5 : public WrappedIDXGIFactory4
{
public:
  WrappedIDXGIFactory5(IDXGIFactory5* factory);
  WrappedIDXGIFactory5(const std::string& name, IDXGIFactory5* factory);
  HRESULT CheckFeatureSupport(DXGI_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize);
};

class WrappedIDXGISwapChain : public WrappedIDXGIDeviceSubObject
{
public:
  WrappedIDXGISwapChain(IDXGISwapChain* swapchain);
  WrappedIDXGISwapChain(const std::string& name, IDXGISwapChain* swapchain);
  virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);
  virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void ** ppSurface);
  virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput * pTarget);
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL * pFullscreen, IDXGIOutput ** ppTarget);
  virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC * pDesc);
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
  virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC * pNewTargetParameters);
  virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput ** ppOutput);
  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS * pStats);
  virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT * pLastPresentCount);
};

class WrappedIDXGISwapChain1 : public WrappedIDXGISwapChain
{
public:
  WrappedIDXGISwapChain1(IDXGISwapChain1* swapchain);
  WrappedIDXGISwapChain1(const std::string& name, IDXGISwapChain1* swapchain);
  virtual HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 * pDesc);
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pDesc);
  virtual HRESULT STDMETHODCALLTYPE GetHwnd(HWND * pHwnd);
  virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void ** ppUnk);
  virtual HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS * pPresentParameters);
  virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void);
  virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput ** ppRestrictToOutput);
  virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA * pColor);
  virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA * pColor);
  virtual HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation);
  virtual HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION * pRotation);
};

class WrappedIDXGISwapChain2 : public WrappedIDXGISwapChain1
{
public:
  WrappedIDXGISwapChain2(IDXGISwapChain2* swapchain);
  WrappedIDXGISwapChain2(const std::string& name, IDXGISwapChain2* swapchain);
  virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height);
  virtual HRESULT STDMETHODCALLTYPE GetSourceSize(UINT * pWidth, UINT * pHeight);
  virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency);
  virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT * pMaxLatency);
  virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void);
  virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F * pMatrix);
  virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F * pMatrix);
};

class WrappedIDXGISwapChain3 : public WrappedIDXGISwapChain2
{
public:
  WrappedIDXGISwapChain3(IDXGISwapChain3* swapchain);
  WrappedIDXGISwapChain3(const std::string& name, IDXGISwapChain3* swapchain);
  virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void);
  virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT * pColorSpaceSupport);
  virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace);
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT * pCreationNodeMask, IUnknown * const * ppPresentQueue);
};

class WrappedIDXGISwapChain4 : public WrappedIDXGISwapChain3
{
public:
  WrappedIDXGISwapChain4(IDXGISwapChain4* swapchain);
  WrappedIDXGISwapChain4(const std::string& name, IDXGISwapChain4* swapchain);
  virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void * pMetaData);
};

// TODO Other DXGI interfaces like device, surface etc
