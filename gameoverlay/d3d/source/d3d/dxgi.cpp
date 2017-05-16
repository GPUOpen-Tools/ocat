// Copyright 2016 Patrick Mours.All rights reserved.
//
// https://github.com/crosire/gameoverlay
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met :
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT
// SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include <wrl.h>
#include "MessageLog.h"
#include "dxgi_swapchain.hpp"
#include "hook_manager.hpp"
#include "ProcessHelper.h"

using namespace Microsoft::WRL;

template <typename T>
void hook_factory_object(T **factoryTarget)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "hook_factory_object");

  IDXGIFactory *const factory = static_cast<IDXGIFactory *>(*factoryTarget);
  // Only install hooks for IDXGIFactory because the same function is called for IDXGIFactory1
  gameoverlay::install_hook(VTABLE(factory), 10, reinterpret_cast<gameoverlay::hook::address>(
                                                     &IDXGIFactory_CreateSwapChain));

  // IDXGIFactory2
  {
    ComPtr<IDXGIFactory2> factory2;
    g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "Query interface for IDXGIFactory2");
    if (SUCCEEDED(factory->QueryInterface(factory2.GetAddressOf()))) {
      g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "Query interface for IDXGIFactory2 success");
      gameoverlay::install_hook(
          VTABLE(factory2.Get()), 15,
          reinterpret_cast<gameoverlay::hook::address>(&IDXGIFactory2_CreateSwapChainForHwnd));
      gameoverlay::install_hook(VTABLE(factory2.Get()), 16,
                                reinterpret_cast<gameoverlay::hook::address>(
                                    &IDXGIFactory2_CreateSwapChainForCoreWindow));
      gameoverlay::install_hook(VTABLE(factory2.Get()), 24,
                                reinterpret_cast<gameoverlay::hook::address>(
                                    &IDXGIFactory2_CreateSwapChainForComposition));
    }
    else {
      g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi", "Query interface for IDXGIFactory2 failed");
    }
  }
}
template <typename T>
void hook_swapchain_object(IUnknown *device, T **swapchainTarget)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "hook_swapchain_object");
  T *const swapchain = *swapchainTarget;

  DXGI_SWAP_CHAIN_DESC desc;
  swapchain->GetDesc(&desc);

  if ((desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) == 0) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "dxgi", "no DXGI_USAGE_RENDER_TARGET_OUTPUT");
    return;
  }

  // D3D11
  {
    ID3D11Device *d3d11Device = nullptr;
    HRESULT hr = device->QueryInterface(&d3d11Device);
    if (SUCCEEDED(hr)) {
      g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "query d3d11 device interface succeded");
      // Reference to d3d11Device is released during destruction of DXGISwapChain object
      *swapchainTarget = new DXGISwapChain(d3d11Device, swapchain);
      return;
    }
    else {
      g_messageLog.Log(MessageLog::LOG_WARNING, "dxgi", "query d3d11 device interface failed", hr);
    }
  }

  // D3D12
  {
    ID3D12CommandQueue *d3d12CommandQueue = nullptr;
    HRESULT hr = device->QueryInterface(&d3d12CommandQueue);
    if (SUCCEEDED(hr)) {
      g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "query d3d12 device interface succeded");
      ComPtr<IDXGISwapChain3> swapChain3;
      hr = swapchain->QueryInterface(swapChain3.GetAddressOf());
      if (SUCCEEDED(hr)) {
        g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "query IDXGISwapChain3 interface succeded");
        // Reference to d3d12CommandQueue is released during destruction of DXGISwapChain object
        *swapchainTarget = new DXGISwapChain(d3d12CommandQueue, swapchain);
        return;
      }
      else {
        g_messageLog.Log(MessageLog::LOG_WARNING, "dxgi", "query IDXGISwapChain3 interface failed",
                         hr);
        d3d12CommandQueue->Release();
      }
    }
    else {
      g_messageLog.Log(MessageLog::LOG_WARNING, "dxgi", "query d3d12 device interface failed", hr);
    }
  }

  g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi", "no swap chain created");
}

// IDXGIFactory
HRESULT STDMETHODCALLTYPE IDXGIFactory_CreateSwapChain(IDXGIFactory *pFactory, IUnknown *pDevice,
                                                       DXGI_SWAP_CHAIN_DESC *pDesc,
                                                       IDXGISwapChain **ppSwapChain)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "IDXGIFactory_CreateSwapChain");
  if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "Error - invalid IDXGIFactory_CreateSwapChain call");
    return DXGI_ERROR_INVALID_CALL;
  }

  const HRESULT hr = gameoverlay::find_hook_trampoline(&IDXGIFactory_CreateSwapChain)(
      pFactory, pDevice, pDesc, ppSwapChain);

  if (SUCCEEDED(hr)) {
    hook_swapchain_object(pDevice, ppSwapChain);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "IDXGIFactory_CreateSwapChain find_hook_trampoline failed", hr);
  }

  return hr;
}

// IDXGIFactory1
HRESULT STDMETHODCALLTYPE IDXGIFactory1_CreateSwapChain(IDXGIFactory1 *pFactory, IUnknown *pDevice,
                                                        DXGI_SWAP_CHAIN_DESC *pDesc,
                                                        IDXGISwapChain **ppSwapChain)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "IDXGIFactory1_CreateSwapChain");
  if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "Error - invalid IDXGIFactory1_CreateSwapChain call");
    return DXGI_ERROR_INVALID_CALL;
  }

  const HRESULT hr = gameoverlay::find_hook_trampoline(&IDXGIFactory1_CreateSwapChain)(
      pFactory, pDevice, pDesc, ppSwapChain);

  if (SUCCEEDED(hr)) {
    hook_swapchain_object(pDevice, ppSwapChain);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "IDXGIFactory1_CreateSwapChain find_hook_trampoline failed", hr);
  }

  return hr;
}

// IDXGIFactory2
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForHwnd(
    IDXGIFactory2 *pFactory, IUnknown *pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput,
    IDXGISwapChain1 **ppSwapChain)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "IDXGIFactory2_CreateSwapChainForHwnd");
  if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "Error - invalid IDXGIFactory2_CreateSwapChainForHwnd call");
    return DXGI_ERROR_INVALID_CALL;
  }

  const HRESULT hr = gameoverlay::find_hook_trampoline(&IDXGIFactory2_CreateSwapChainForHwnd)(
      pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

  if (SUCCEEDED(hr)) {
    hook_swapchain_object(pDevice, ppSwapChain);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "IDXGIFactory2_CreateSwapChainForHwnd find_hook_trampoline failed", hr);
  }

  return hr;
}
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForCoreWindow(
    IDXGIFactory2 *pFactory, IUnknown *pDevice, IUnknown *pWindow,
    const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput,
    IDXGISwapChain1 **ppSwapChain)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "IDXGIFactory2_CreateSwapChainForCoreWindow");
  if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "Error - invalid IDXGIFactory2_CreateSwapChainForCoreWindow call");
    return DXGI_ERROR_INVALID_CALL;
  }

  const HRESULT hr = gameoverlay::find_hook_trampoline(&IDXGIFactory2_CreateSwapChainForCoreWindow)(
      pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

  if (SUCCEEDED(hr)) {
    hook_swapchain_object(pDevice, ppSwapChain);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "IDXGIFactory2_CreateSwapChainForCoreWindow find_hook_trampoline failed", hr);
  }

  return hr;
}
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForComposition(
    IDXGIFactory2 *pFactory, IUnknown *pDevice, const DXGI_SWAP_CHAIN_DESC1 *pDesc,
    IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "IDXGIFactory2_CreateSwapChainForComposition");
  if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "Error - invalid IDXGIFactory2_CreateSwapChainForComposition call");
    return DXGI_ERROR_INVALID_CALL;
  }

  const HRESULT hr =
      gameoverlay::find_hook_trampoline(&IDXGIFactory2_CreateSwapChainForComposition)(
          pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);

  if (SUCCEEDED(hr)) {
    hook_swapchain_object(pDevice, ppSwapChain);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "IDXGIFactory2_CreateSwapChainForComposition find_hook_trampoline failed", hr);
  }

  return hr;
}

EXTERN_C HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **ppFactory)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "CreateDXGIFactory");
  const HRESULT hr = gameoverlay::find_hook_trampoline(&CreateDXGIFactory)(riid, ppFactory);
  if (SUCCEEDED(hr)) {
    hook_factory_object(ppFactory);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi", "CreateDXGIFactory find_hook_trampoline failed",
                     hr);
  }

  return hr;
}
EXTERN_C HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void **ppFactory)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "CreateDXGIFactory1");
  const HRESULT hr = gameoverlay::find_hook_trampoline(&CreateDXGIFactory1)(riid, ppFactory);

  if (SUCCEEDED(hr)) {
    hook_factory_object(ppFactory);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "CreateDXGIFactory1 find_hook_trampoline failed", hr);
  }

  return hr;
}
EXTERN_C HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void **ppFactory)
{
  g_messageLog.Log(MessageLog::LOG_INFO, "dxgi", "CreateDXGIFactory2");
#ifdef _DEBUG
  flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  const HRESULT hr = gameoverlay::find_hook_trampoline(&CreateDXGIFactory2)(flags, riid, ppFactory);

  if (SUCCEEDED(hr)) {
    hook_factory_object(ppFactory);
  }
  else {
    g_messageLog.Log(MessageLog::LOG_ERROR, "dxgi",
                     "CreateDXGIFactory2 find_hook_trampoline failed", hr);
  }

  return hr;
}
