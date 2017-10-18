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

#include "DXGIWrapper.h"

#include <sstream> //for std::stringstream 
#include <string>  //for std::string


HRESULT WrappedIUnknown::QueryInterface(REFIID riid, void ** ppvObject)
{
  if (riid == __uuidof(IDXGIAdapter))
  {
    Log("Queried IDXGIAdapter");
    return QueryWrappedInterface<WrappedIDXGIAdapter, IDXGIAdapter>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIAdapter1))
  {
    Log("Queried IDXGIAdapter1");
    return QueryWrappedInterface<WrappedIDXGIAdapter1, IDXGIAdapter1>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIAdapter2))
  {
    Log("Queried IDXGIAdapter2");
    return QueryWrappedInterface<WrappedIDXGIAdapter2, IDXGIAdapter2>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIAdapter3))
  {
    Log("Queried IDXGIAdapter3");
    return QueryWrappedInterface<WrappedIDXGIAdapter3, IDXGIAdapter3>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIAdapter4))
  {
    Log("Queried IDXGIAdapter4");
    return QueryWrappedInterface<WrappedIDXGIAdapter4, IDXGIAdapter4>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory))
  {
    Log("Queried IDXGIFactory");
    return QueryWrappedInterface<WrappedIDXGIFactory, IDXGIFactory>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory1))
  {
    Log("Queried IDXGIFactory1");
    return QueryWrappedInterface<WrappedIDXGIFactory1, IDXGIFactory1>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory2))
  {
    Log("Queried IDXGIFactory2");
    return QueryWrappedInterface<WrappedIDXGIFactory2, IDXGIFactory2>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory3))
  {
    Log("Queried IDXGIFactory3");
    return QueryWrappedInterface<WrappedIDXGIFactory3, IDXGIFactory3>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory4))
  {
    Log("Queried IDXGIFactory4");
    return QueryWrappedInterface<WrappedIDXGIFactory4, IDXGIFactory4>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGIFactory5))
  {
    Log("Queried IDXGIFactory5");
    return QueryWrappedInterface<WrappedIDXGIFactory5, IDXGIFactory5>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGISwapChain))
  {
    Log("Queried IDXGISwapChain");
    return QueryWrappedInterface<WrappedIDXGISwapChain, IDXGISwapChain>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGISwapChain1))
  {
    Log("Queried IDXGISwapChain1");
    return QueryWrappedInterface<WrappedIDXGISwapChain1, IDXGISwapChain1>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGISwapChain2))
  {
    Log("Queried IDXGISwapChain2");
    return QueryWrappedInterface<WrappedIDXGISwapChain2, IDXGISwapChain2>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGISwapChain3))
  {
    Log("Queried IDXGISwapChain3");
    return QueryWrappedInterface<WrappedIDXGISwapChain3, IDXGISwapChain3>(unknown_, riid, ppvObject);
  }
  else if (riid == __uuidof(IDXGISwapChain4))
  {
    Log("Queried IDXGISwapChain4");
    return QueryWrappedInterface<WrappedIDXGISwapChain4, IDXGISwapChain4>(unknown_, riid, ppvObject);
  }
  // Log non wrapped interfaces
  else if (riid == __uuidof(IDXGIDecodeSwapChain))
  {
    Log("Queried IDXGIDecodeSwapChain");
  }
  else if (riid == __uuidof(IDXGIDevice))
  {
    Log("Queried IDXGIDevice");
  }
  else if (riid == __uuidof(IDXGIDevice1))
  {
    Log("Queried IDXGIDevice1");
  }
  else if (riid == __uuidof(IDXGIDevice2))
  {
    Log("Queried IDXGIDevice2");
  }
  else if (riid == __uuidof(IDXGIDevice3))
  {
    Log("Queried IDXGIDevice3");
  }
  else if (riid == __uuidof(IDXGIDevice4))
  {
    Log("Queried IDXGIDevice4");
  }
  else if (riid == __uuidof(IDXGIFactoryMedia))
  {
    Log("Queried IDXGIFactoryMedia");
  }
  else if (riid == __uuidof(IDXGIKeyedMutex))
  {
    Log("Queried IDXGIKeyedMutex");
  }
  else if (riid == __uuidof(IDXGIOutput))
  {
    Log("Queried IDXGIOutput");
  }
  else if (riid == __uuidof(IDXGIOutput1))
  {
    Log("Queried IDXGIOutput1");
  }
  else if (riid == __uuidof(IDXGIOutput2))
  {
    Log("Queried IDXGIOutput2");
  }
  else if (riid == __uuidof(IDXGIOutput3))
  {
    Log("Queried IDXGIOutput3");
  }
  else if (riid == __uuidof(IDXGIOutput4))
  {
    Log("Queried IDXGIOutput4");
  }
  else if (riid == __uuidof(IDXGIOutput5))
  {
    Log("Queried IDXGIOutput5");
  }
  else if (riid == __uuidof(IDXGIOutput6))
  {
    Log("Queried IDXGIOutput6");
  }
  else if (riid == __uuidof(IDXGIOutputDuplication))
  {
    Log("Queried IDXGIOutputDuplication");
  }
  else if (riid == __uuidof(IDXGIResource))
  {
    Log("Queried IDXGIResource");
  }
  else if (riid == __uuidof(IDXGIResource1))
  {
    Log("Queried IDXGIResource1");
  }
  else if (riid == __uuidof(IDXGISurface))
  {
    Log("Queried IDXGISurface");
  }
  else if (riid == __uuidof(IDXGISurface1))
  {
    Log("Queried IDXGISurface1");
  }
  else if (riid == __uuidof(IDXGISurface2))
  {
    Log("Queried IDXGISurface2");
  }
  else if (riid == __uuidof(IDXGISwapChainMedia))
  {
    Log("Queried IDXGISwapChainMedia");
  }
  else
  {
    LogUUID(riid, L"Queried unknown interface ");
  }
  return unknown_->QueryInterface(riid, ppvObject);
}

ULONG WrappedIUnknown::AddRef(void)
{
  Log("AddRef");
  return unknown_->AddRef();
}

ULONG WrappedIUnknown::Release(void)
{
  Log("Release");
  return unknown_->Release();
}

WrappedIDXGIObject::WrappedIDXGIObject(const std::string & name, IDXGIObject * object)
  : WrappedIUnknown(name, object) {}

HRESULT WrappedIDXGIObject::SetPrivateData(REFGUID Name, UINT DataSize, const void * pData)
{
  Log("SetPrivateData");
  return Get<IDXGIObject>()->SetPrivateData(Name, DataSize, pData);
}

HRESULT WrappedIDXGIObject::SetPrivateDataInterface(REFGUID Name, const IUnknown * pUnknown)
{
  Log("SetPrivateDataInterface");
  return Get<IDXGIObject>()->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT WrappedIDXGIObject::GetPrivateData(REFGUID Name, UINT * pDataSize, void * pData)
{
  Log("GetPrivateData");
  return Get<IDXGIObject>()->GetPrivateData(Name, pDataSize, pData);
}

HRESULT WrappedIDXGIObject::GetParent(REFIID riid, void ** ppParent)
{
  Log("GetParent");
  return Get<IDXGIObject>()->GetParent(riid, ppParent);
}

WrappedIDXGIAdapter::WrappedIDXGIAdapter(IDXGIAdapter * adapter)
  : WrappedIDXGIObject("WrappedIDXGIAdapter", adapter) {}

WrappedIDXGIAdapter::WrappedIDXGIAdapter(const std::string & name, IDXGIAdapter * adapter)
  : WrappedIDXGIObject(name, adapter) {}

HRESULT WrappedIDXGIAdapter::EnumOutputs(UINT Output, IDXGIOutput ** ppOutput)
{
  Log("EnumOutputs");
  return Get<IDXGIAdapter>()->EnumOutputs(Output, ppOutput);
}

HRESULT WrappedIDXGIAdapter::GetDesc(DXGI_ADAPTER_DESC * pDesc)
{
  Log("GetDesc");
  return Get<IDXGIAdapter>()->GetDesc(pDesc);
}

HRESULT WrappedIDXGIAdapter::CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER * pUMDVersion)
{
  Log("CheckInterfaceSupport");
  return Get<IDXGIAdapter>()->CheckInterfaceSupport(InterfaceName, pUMDVersion);
}

WrappedIDXGIAdapter1::WrappedIDXGIAdapter1(IDXGIAdapter1 * adapter)
  : WrappedIDXGIAdapter("WrappedIDXGIAdapter1", adapter) {}

WrappedIDXGIAdapter1::WrappedIDXGIAdapter1(const std::string & name, IDXGIAdapter1 * adapter)
  : WrappedIDXGIAdapter(name, adapter) {}

HRESULT WrappedIDXGIAdapter1::GetDesc1(DXGI_ADAPTER_DESC1 * pDesc)
{
  Log("GetDesc1");
  return Get<IDXGIAdapter1>()->GetDesc1(pDesc);
}

WrappedIDXGIFactory::WrappedIDXGIFactory(IDXGIFactory * factory)
  : WrappedIDXGIObject("WrappedIDXGIFactory", factory) {}

WrappedIDXGIFactory::WrappedIDXGIFactory(const std::string & name, IDXGIFactory * factory)
  : WrappedIDXGIObject(name, factory) {}

HRESULT WrappedIDXGIFactory::EnumAdapters(UINT Adapter, IDXGIAdapter ** ppAdapter)
{
  Log("EnumAdapters");
  IDXGIAdapter* adapter;
  HRESULT hr = Get<IDXGIFactory>()->EnumAdapters(Adapter, &adapter);
  if (SUCCEEDED(hr))
  {
    WrappedIDXGIAdapter* wrappedAdapter = new WrappedIDXGIAdapter(adapter);
    *ppAdapter = (IDXGIAdapter*)wrappedAdapter;
  }
  return hr;
}

HRESULT WrappedIDXGIFactory::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
  Log("MakeWindowAssociation");
  return Get<IDXGIFactory>()->MakeWindowAssociation(WindowHandle, Flags);
}

HRESULT WrappedIDXGIFactory::GetWindowAssociation(HWND * pWindowHandle)
{
  Log("GetWindowAssociation");
  return Get<IDXGIFactory>()->GetWindowAssociation(pWindowHandle);
}

HRESULT WrappedIDXGIFactory::CreateSwapChain(IUnknown * pDevice, DXGI_SWAP_CHAIN_DESC * pDesc, IDXGISwapChain ** ppSwapChain)
{
  Log("CreateSwapChain");
  return Get<IDXGIFactory>()->CreateSwapChain(pDevice, pDesc, ppSwapChain);
}

HRESULT WrappedIDXGIFactory::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter ** ppAdapter)
{
  Log("CreateSoftwareAdapter");
  IDXGIAdapter* adapter;
  HRESULT hr = Get<IDXGIFactory>()->CreateSoftwareAdapter(Module, &adapter);
  if (SUCCEEDED(hr))
  {
    WrappedIDXGIAdapter* wrappedAdapter = new WrappedIDXGIAdapter(adapter);
    *ppAdapter = (IDXGIAdapter*)wrappedAdapter;
  }
  return hr;
}

WrappedIDXGIFactory1::WrappedIDXGIFactory1(IDXGIFactory1 * factory)
  : WrappedIDXGIFactory("WrappedIDXGIFactory1", factory) {}

WrappedIDXGIFactory1::WrappedIDXGIFactory1(const std::string & name, IDXGIFactory1 * factory)
  : WrappedIDXGIFactory(name, factory) {}

HRESULT WrappedIDXGIFactory1::EnumAdapters1(UINT Adapter, IDXGIAdapter1 ** ppAdapter)
{
  Log("EnumAdapters1");
  IDXGIAdapter1* adapter;
  HRESULT hr = Get<IDXGIFactory1>()->EnumAdapters1(Adapter, &adapter);
  if (SUCCEEDED(hr))
  {
    WrappedIDXGIAdapter1* wrappedAdapter = new WrappedIDXGIAdapter1(adapter);
    *ppAdapter = (IDXGIAdapter1*)wrappedAdapter;
  }
  return hr;
}

BOOL WrappedIDXGIFactory1::IsCurrent(void)
{
  Log("IsCurrent");
  return Get<IDXGIFactory1>()->IsCurrent();
}

WrappedIDXGIFactory2::WrappedIDXGIFactory2(IDXGIFactory2 * factory)
  : WrappedIDXGIFactory1("WrappedIDXGIFactory2", factory) {}

WrappedIDXGIFactory2::WrappedIDXGIFactory2(const std::string & name, IDXGIFactory2 * factory)
  : WrappedIDXGIFactory1(name, factory) {}

BOOL WrappedIDXGIFactory2::IsWindowedStereoEnabled(void)
{
  Log("IsWindowedStereoEnabled");
  return Get<IDXGIFactory2>()->IsWindowedStereoEnabled();
}

HRESULT WrappedIDXGIFactory2::CreateSwapChainForHwnd(IUnknown * pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 * pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain)
{
  Log("CreateSwapChainForHwnd");
  return Get<IDXGIFactory2>()->CreateSwapChainForHwnd(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

HRESULT WrappedIDXGIFactory2::CreateSwapChainForCoreWindow(IUnknown * pDevice, IUnknown * pWindow, const DXGI_SWAP_CHAIN_DESC1 * pDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain)
{
  Log("CreateSwapChainForCoreWindow");
  return Get<IDXGIFactory2>()->CreateSwapChainForCoreWindow(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

HRESULT WrappedIDXGIFactory2::GetSharedResourceAdapterLuid(HANDLE hResource, LUID * pLuid)
{
  Log("GetSharedResourceAdapterLuid");
  return Get<IDXGIFactory2>()->GetSharedResourceAdapterLuid(hResource, pLuid);
}

HRESULT WrappedIDXGIFactory2::RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD * pdwCookie)
{
  Log("RegisterStereoStatusWindow");
  return Get<IDXGIFactory2>()->RegisterStereoStatusWindow(WindowHandle, wMsg, pdwCookie);
}

HRESULT WrappedIDXGIFactory2::RegisterStereoStatusEvent(HANDLE hEvent, DWORD * pdwCookie)
{
  Log("RegisterStereoStatusEvent");
  return Get<IDXGIFactory2>()->RegisterStereoStatusEvent(hEvent, pdwCookie);
}

void WrappedIDXGIFactory2::UnregisterStereoStatus(DWORD dwCookie)
{
  Log("UnregisterStereoStatus");
  Get<IDXGIFactory2>()->UnregisterStereoStatus(dwCookie);
}

HRESULT WrappedIDXGIFactory2::RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD * pdwCookie)
{
  Log("RegisterOcclusionStatusWindow");
  return Get<IDXGIFactory2>()->RegisterOcclusionStatusWindow(WindowHandle, wMsg, pdwCookie);
}

HRESULT WrappedIDXGIFactory2::RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD * pdwCookie)
{
  Log("RegisterOcclusionStatusEvent");
  return Get<IDXGIFactory2>()->RegisterOcclusionStatusEvent(hEvent, pdwCookie);
}

void WrappedIDXGIFactory2::UnregisterOcclusionStatus(DWORD dwCookie)
{
  Log("UnregisterOcclusionStatus");
  Get<IDXGIFactory2>()->UnregisterOcclusionStatus(dwCookie);
}

HRESULT WrappedIDXGIFactory2::CreateSwapChainForComposition(IUnknown * pDevice, const DXGI_SWAP_CHAIN_DESC1 * pDesc, IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 ** ppSwapChain)
{
  Log("CreateSwapChainForComposition");
  return Get<IDXGIFactory2>()->CreateSwapChainForComposition(pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

WrappedIUnknown::WrappedIUnknown(const std::string & name, IUnknown * unknown)
  : name_(name), unknown_(unknown)
{
  LogWrapped(unknown);
}

void WrappedIUnknown::LogUUID(REFIID riid, const std::wstring& message)
{
  LPWSTR str;
  HRESULT riidHR = StringFromIID(riid, &str);
  if (SUCCEEDED(riidHR))
  {
    Log(message + L" - UUID " + std::wstring(str));
    CoTaskMemFree(str);
  }
}

void WrappedIUnknown::LogWrapped(IUnknown * unknown)
{
  const void * address = static_cast<const void*>(unknown);
  std::stringstream stream;
  stream << "Wrapped around [" << address << "]";
  Log(stream.str());
}

void WrappedIUnknown::Log(const std::string & message)
{
  // For debugging purposes to distinguish different objects
  const void * address = static_cast<const void*>(this);
  std::stringstream stream;
  stream << "[" << address << "] -> " << message;
  g_messageLog.LogInfo(name_, stream.str());
}

void WrappedIUnknown::Log(const std::wstring & message)
{
  // For debugging purposes to distinguish different objects
  const void * address = static_cast<const void*>(this);
  std::wstringstream stream;
  stream << "[" << address << "] -> " << message;
  g_messageLog.LogInfo(name_, stream.str());
}

WrappedIDXGIFactory3::WrappedIDXGIFactory3(IDXGIFactory3 * factory)
  : WrappedIDXGIFactory2("WrappedIDXGIFactory3", factory) {}

WrappedIDXGIFactory3::WrappedIDXGIFactory3(const std::string & name, IDXGIFactory3 * factory)
  : WrappedIDXGIFactory2(name, factory) {}

UINT WrappedIDXGIFactory3::GetCreationFlags()
{
  Log("GetCreationFlags");
  return Get<IDXGIFactory3>()->GetCreationFlags();
}

WrappedIDXGIFactory4::WrappedIDXGIFactory4(IDXGIFactory4 * factory)
  : WrappedIDXGIFactory3("WrappedIDXGIFactory4", factory) {}

WrappedIDXGIFactory4::WrappedIDXGIFactory4(const std::string & name, IDXGIFactory4 * factory)
  : WrappedIDXGIFactory3(name, factory) {}

HRESULT WrappedIDXGIFactory4::EnumAdapterByLuid(LUID AdapterLuid, REFIID riid, void ** ppvAdapter)
{
  Log("EnumAdapterByLuid");
  return Get<IDXGIFactory4>()->EnumAdapterByLuid(AdapterLuid, riid, ppvAdapter);
}

HRESULT WrappedIDXGIFactory4::EnumWarpAdapter(REFIID riid, void ** ppvAdapter)
{
  Log("EnumWarpAdapter");
  return Get<IDXGIFactory4>()->EnumWarpAdapter(riid, ppvAdapter);
}

WrappedIDXGIFactory5::WrappedIDXGIFactory5(IDXGIFactory5 * factory)
  : WrappedIDXGIFactory4("WrappedIDXGIFactory5", factory) {}

WrappedIDXGIFactory5::WrappedIDXGIFactory5(const std::string & name, IDXGIFactory5 * factory)
  : WrappedIDXGIFactory4(name, factory) {}

HRESULT WrappedIDXGIFactory5::CheckFeatureSupport(DXGI_FEATURE Feature, void * pFeatureSupportData, UINT FeatureSupportDataSize)
{
  Log("CheckFeatureSupport");
  return Get<IDXGIFactory5>()->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

WrappedIDXGIAdapter2::WrappedIDXGIAdapter2(IDXGIAdapter2 * adapter)
  : WrappedIDXGIAdapter1("WrappedIDXGIAdapter2", adapter) {}

WrappedIDXGIAdapter2::WrappedIDXGIAdapter2(const std::string & name, IDXGIAdapter2 * adapter)
  : WrappedIDXGIAdapter1(name, adapter) {}

HRESULT WrappedIDXGIAdapter2::GetDesc2(DXGI_ADAPTER_DESC2 * pDesc)
{
  Log("GetDesc2");
  return Get<IDXGIAdapter2>()->GetDesc2(pDesc);
}

WrappedIDXGIAdapter3::WrappedIDXGIAdapter3(IDXGIAdapter3 * adapter)
  : WrappedIDXGIAdapter2("WrappedIDXGIAdapter3", adapter) {}

WrappedIDXGIAdapter3::WrappedIDXGIAdapter3(const std::string & name, IDXGIAdapter3 * adapter)
  : WrappedIDXGIAdapter2(name, adapter) {}

HRESULT WrappedIDXGIAdapter3::QueryVideoMemoryInfo(UINT NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, DXGI_QUERY_VIDEO_MEMORY_INFO * pVideoMemoryInfo)
{
  Log("QueryVideoMemoryInfo");
  return Get<IDXGIAdapter3>()->QueryVideoMemoryInfo(NodeIndex, MemorySegmentGroup, pVideoMemoryInfo);
}

HRESULT WrappedIDXGIAdapter3::RegisterHardwareContentProtectionTeardownStatusEvent(HANDLE hEvent, DWORD * pdwCookie)
{
  Log("RegisterHardwareContentProtectionTeardownStatusEvent");
  return Get<IDXGIAdapter3>()->RegisterHardwareContentProtectionTeardownStatusEvent(hEvent, pdwCookie);
}

HRESULT WrappedIDXGIAdapter3::RegisterVideoMemoryBudgetChangeNotificationEvent(HANDLE hEvent, DWORD * pdwCookie)
{
  Log("RegisterVideoMemoryBudgetChangeNotificationEvent");
  return Get<IDXGIAdapter3>()->RegisterVideoMemoryBudgetChangeNotificationEvent(hEvent, pdwCookie);
}

HRESULT WrappedIDXGIAdapter3::SetVideoMemoryReservation(UINT NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, UINT64 Reservation)
{
  Log("SetVideoMemoryReservation");
  return Get<IDXGIAdapter3>()->SetVideoMemoryReservation(NodeIndex, MemorySegmentGroup, Reservation);
}

void WrappedIDXGIAdapter3::UnregisterHardwareContentProtectionTeardownStatus(DWORD dwCookie)
{
  Log("UnregisterHardwareContentProtectionTeardownStatus");
  return Get<IDXGIAdapter3>()->UnregisterHardwareContentProtectionTeardownStatus(dwCookie);
}

void WrappedIDXGIAdapter3::UnregisterVideoMemoryBudgetChangeNotification(DWORD dwCookie)
{
  Log("UnregisterVideoMemoryBudgetChangeNotification");
  return Get<IDXGIAdapter3>()->UnregisterVideoMemoryBudgetChangeNotification(dwCookie);
}

WrappedIDXGIAdapter4::WrappedIDXGIAdapter4(IDXGIAdapter4 * adapter)
  : WrappedIDXGIAdapter3("WrappedIDXGIAdapter4", adapter) {}

WrappedIDXGIAdapter4::WrappedIDXGIAdapter4(const std::string & name, IDXGIAdapter4 * adapter)
  : WrappedIDXGIAdapter3(name, adapter) {}

HRESULT WrappedIDXGIAdapter4::GetDesc3(DXGI_ADAPTER_DESC3 * pDesc)
{
  Log("GetDesc3");
  return Get<IDXGIAdapter4>()->GetDesc3(pDesc);
}

WrappedIDXGIDeviceSubObject::WrappedIDXGIDeviceSubObject(const std::string & name, IDXGIDeviceSubObject * object)
  : WrappedIDXGIObject(name, object) {}

HRESULT WrappedIDXGIDeviceSubObject::GetDevice(REFIID riid, void ** ppDevice)
{
  Log("GetDevice");
  return Get<IDXGIDeviceSubObject>()->GetDevice(riid, ppDevice);
}

WrappedIDXGISwapChain::WrappedIDXGISwapChain(IDXGISwapChain * swapchain)
  : WrappedIDXGIDeviceSubObject("WrappedIDXGISwapChain", swapchain) {}

WrappedIDXGISwapChain::WrappedIDXGISwapChain(const std::string & name, IDXGISwapChain * swapchain)
  : WrappedIDXGIDeviceSubObject(name, swapchain) {}

HRESULT WrappedIDXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
  Log("Present");
  return Get<IDXGISwapChain>()->Present(SyncInterval, Flags);
}

HRESULT WrappedIDXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void ** ppSurface)
{
  Log("GetBuffer");
  return Get<IDXGISwapChain>()->GetBuffer(Buffer, riid, ppSurface);
}

HRESULT WrappedIDXGISwapChain::SetFullscreenState(BOOL Fullscreen, IDXGIOutput * pTarget)
{
  Log("SetFullscreenState");
  return Get<IDXGISwapChain>()->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT WrappedIDXGISwapChain::GetFullscreenState(BOOL * pFullscreen, IDXGIOutput ** ppTarget)
{
  Log("GetFullscreenState");
  return Get<IDXGISwapChain>()->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT WrappedIDXGISwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC * pDesc)
{
  Log("GetDesc");
  return Get<IDXGISwapChain>()->GetDesc(pDesc);
}

HRESULT WrappedIDXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
  Log("ResizeBuffers");
  return Get<IDXGISwapChain>()->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT WrappedIDXGISwapChain::ResizeTarget(const DXGI_MODE_DESC * pNewTargetParameters)
{
  Log("ResizeTarget");
  return Get<IDXGISwapChain>()->ResizeTarget(pNewTargetParameters);
}

HRESULT WrappedIDXGISwapChain::GetContainingOutput(IDXGIOutput ** ppOutput)
{
  Log("GetContainingOutput");
  return Get<IDXGISwapChain>()->GetContainingOutput(ppOutput);
}

HRESULT WrappedIDXGISwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS * pStats)
{
  Log("GetFrameStatistics");
  return Get<IDXGISwapChain>()->GetFrameStatistics(pStats);
}

HRESULT WrappedIDXGISwapChain::GetLastPresentCount(UINT * pLastPresentCount)
{
  Log("GetLastPresentCount");
  return Get<IDXGISwapChain>()->GetLastPresentCount(pLastPresentCount);
}

WrappedIDXGISwapChain1::WrappedIDXGISwapChain1(IDXGISwapChain1 * swapchain)
  : WrappedIDXGISwapChain("WrappedIDXGISwapChain1", swapchain) {}

WrappedIDXGISwapChain1::WrappedIDXGISwapChain1(const std::string & name, IDXGISwapChain1 * swapchain)
  : WrappedIDXGISwapChain(name, swapchain) {}

HRESULT WrappedIDXGISwapChain1::GetDesc1(DXGI_SWAP_CHAIN_DESC1 * pDesc)
{
  Log("GetDesc1");
  return Get<IDXGISwapChain1>()->GetDesc1(pDesc);
}

HRESULT WrappedIDXGISwapChain1::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pDesc)
{
  Log("GetFullscreenDesc");
  return Get<IDXGISwapChain1>()->GetFullscreenDesc(pDesc);
}

HRESULT WrappedIDXGISwapChain1::GetHwnd(HWND * pHwnd)
{
  Log("GetHwnd");
  return Get<IDXGISwapChain1>()->GetHwnd(pHwnd);
}

HRESULT WrappedIDXGISwapChain1::GetCoreWindow(REFIID refiid, void ** ppUnk)
{
  Log("GetCoreWindow");
  return Get<IDXGISwapChain1>()->GetCoreWindow(refiid, ppUnk);
}

HRESULT WrappedIDXGISwapChain1::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS * pPresentParameters)
{
  Log("Present1");
  return Get<IDXGISwapChain1>()->Present1(SyncInterval, PresentFlags, pPresentParameters);
}

BOOL WrappedIDXGISwapChain1::IsTemporaryMonoSupported(void)
{
  Log("IsTemporaryMonoSupported");
  return Get<IDXGISwapChain1>()->IsTemporaryMonoSupported();
}

HRESULT WrappedIDXGISwapChain1::GetRestrictToOutput(IDXGIOutput ** ppRestrictToOutput)
{
  Log("GetRestrictToOutput");
  return Get<IDXGISwapChain1>()->GetRestrictToOutput(ppRestrictToOutput);
}

HRESULT WrappedIDXGISwapChain1::SetBackgroundColor(const DXGI_RGBA * pColor)
{
  Log("SetBackgroundColor");
  return Get<IDXGISwapChain1>()->SetBackgroundColor(pColor);
}

HRESULT WrappedIDXGISwapChain1::GetBackgroundColor(DXGI_RGBA * pColor)
{
  Log("GetBackgroundColor");
  return Get<IDXGISwapChain1>()->GetBackgroundColor(pColor);
}

HRESULT WrappedIDXGISwapChain1::SetRotation(DXGI_MODE_ROTATION Rotation)
{
  Log("SetRotation");
  return Get<IDXGISwapChain1>()->SetRotation(Rotation);
}

HRESULT WrappedIDXGISwapChain1::GetRotation(DXGI_MODE_ROTATION * pRotation)
{
  Log("GetRotation");
  return Get<IDXGISwapChain1>()->GetRotation(pRotation);
}

WrappedIDXGISwapChain2::WrappedIDXGISwapChain2(IDXGISwapChain2 * swapchain)
  : WrappedIDXGISwapChain1("WrappedIDXGISwapChain2", swapchain) {}

WrappedIDXGISwapChain2::WrappedIDXGISwapChain2(const std::string & name, IDXGISwapChain2 * swapchain)
  : WrappedIDXGISwapChain1(name, swapchain) {}

HRESULT WrappedIDXGISwapChain2::SetSourceSize(UINT Width, UINT Height)
{
  Log("SetSourceSize");
  return Get<IDXGISwapChain2>()->SetSourceSize(Width, Height);
}

HRESULT WrappedIDXGISwapChain2::GetSourceSize(UINT * pWidth, UINT * pHeight)
{
  Log("GetSourceSize");
  return Get<IDXGISwapChain2>()->GetSourceSize(pWidth, pHeight);
}

HRESULT WrappedIDXGISwapChain2::SetMaximumFrameLatency(UINT MaxLatency)
{
  Log("SetMaximumFrameLatency");
  return Get<IDXGISwapChain2>()->SetMaximumFrameLatency(MaxLatency);
}

HRESULT WrappedIDXGISwapChain2::GetMaximumFrameLatency(UINT * pMaxLatency)
{
  Log("GetMaximumFrameLatency");
  return Get<IDXGISwapChain2>()->GetMaximumFrameLatency(pMaxLatency);
}

HANDLE WrappedIDXGISwapChain2::GetFrameLatencyWaitableObject(void)
{
  Log("GetFrameLatencyWaitableObject");
  return Get<IDXGISwapChain2>()->GetFrameLatencyWaitableObject();
}

HRESULT WrappedIDXGISwapChain2::SetMatrixTransform(const DXGI_MATRIX_3X2_F * pMatrix)
{
  Log("SetMatrixTransform");
  return Get<IDXGISwapChain2>()->SetMatrixTransform(pMatrix);
}

HRESULT WrappedIDXGISwapChain2::GetMatrixTransform(DXGI_MATRIX_3X2_F * pMatrix)
{
  Log("GetMatrixTransform");
  return Get<IDXGISwapChain2>()->GetMatrixTransform(pMatrix);
}

WrappedIDXGISwapChain3::WrappedIDXGISwapChain3(IDXGISwapChain3 * swapchain)
  : WrappedIDXGISwapChain2("WrappedIDXGISwapChain3", swapchain) {}

WrappedIDXGISwapChain3::WrappedIDXGISwapChain3(const std::string & name, IDXGISwapChain3 * swapchain)
  : WrappedIDXGISwapChain2(name, swapchain) {}

UINT WrappedIDXGISwapChain3::GetCurrentBackBufferIndex(void)
{
  Log("GetCurrentBackBufferIndex");
  return Get<IDXGISwapChain3>()->GetCurrentBackBufferIndex();
}

HRESULT WrappedIDXGISwapChain3::CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT * pColorSpaceSupport)
{
  Log("CheckColorSpaceSupport");
  return Get<IDXGISwapChain3>()->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
}

HRESULT WrappedIDXGISwapChain3::SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
  Log("SetColorSpace1");
  return Get<IDXGISwapChain3>()->SetColorSpace1(ColorSpace);
}

HRESULT WrappedIDXGISwapChain3::ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT * pCreationNodeMask, IUnknown * const * ppPresentQueue)
{
  Log("ResizeBuffers1");
  return Get<IDXGISwapChain3>()->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

WrappedIDXGISwapChain4::WrappedIDXGISwapChain4(IDXGISwapChain4 * swapchain)
  : WrappedIDXGISwapChain3("WrappedIDXGISwapChain4", swapchain) {}

WrappedIDXGISwapChain4::WrappedIDXGISwapChain4(const std::string & name, IDXGISwapChain4 * swapchain)
  : WrappedIDXGISwapChain3(name, swapchain) {}

HRESULT WrappedIDXGISwapChain4::SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size, void * pMetaData)
{
  Log("SetHDRMetaData");
  return Get<IDXGISwapChain4>()->SetHDRMetaData(Type, Size, pMetaData);
}
