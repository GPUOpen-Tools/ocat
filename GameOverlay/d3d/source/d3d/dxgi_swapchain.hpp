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

#pragma once

#include <dxgi1_5.h>
#include <wrl.h>
#include <memory>
#include "d3d11_renderer.hpp"
#include "d3d12_renderer.hpp"

struct __declspec(uuid("1F445F9F-9887-4C4C-9055-4E3BADAFCCA8")) DXGISwapChain;

struct DXGISwapChain : IDXGISwapChain4 {
  DXGISwapChain(ID3D11Device *device, IDXGISwapChain *swapChain);
  DXGISwapChain(ID3D11Device *device, IDXGISwapChain1 *swapChain);
  DXGISwapChain(ID3D12CommandQueue *commandQueue, IDXGISwapChain *swapChain);

#pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObj) override;
  virtual ULONG STDMETHODCALLTYPE AddRef() override;
  virtual ULONG STDMETHODCALLTYPE Release() override;
#pragma endregion
#pragma region IDXGIObject
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize,
                                                   const void *pData) override;
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name,
                                                            const IUnknown *pUnknown) override;
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize,
                                                   void *pData) override;
  virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent) override;
#pragma endregion
#pragma region IDXGIDeviceSubObject
  virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice) override;
#pragma endregion
#pragma region IDXGISwapChain
  virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override;
  virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) override;
  virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen,
                                                       IDXGIOutput *pTarget) override;
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen,
                                                       IDXGIOutput **ppTarget) override;
  virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) override;
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height,
                                                  DXGI_FORMAT NewFormat,
                                                  UINT SwapChainFlags) override;
  virtual HRESULT STDMETHODCALLTYPE
  ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) override;
  virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput) override;
  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) override;
  virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount) override;
#pragma endregion
#pragma region IDXGISwapChain1
  virtual HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc) override;
  virtual HRESULT STDMETHODCALLTYPE
  GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) override;
  virtual HRESULT STDMETHODCALLTYPE GetHwnd(HWND *pHwnd) override;
  virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void **ppUnk) override;
  virtual HRESULT STDMETHODCALLTYPE
  Present1(UINT SyncInterval, UINT PresentFlags,
           const DXGI_PRESENT_PARAMETERS *pPresentParameters) override;
  virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override;
  virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput) override;
  virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA *pColor) override;
  virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA *pColor) override;
  virtual HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override;
  virtual HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION *pRotation) override;
#pragma endregion
#pragma region IDXGISwapChain2
  virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) override;
  virtual HRESULT STDMETHODCALLTYPE GetSourceSize(UINT *pWidth, UINT *pHeight) override;
  virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override;
  virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT *pMaxLatency) override;
  virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() override;
  virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) override;
  virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix) override;
#pragma endregion
#pragma region IDXGISwapChain3
  virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() override;
  virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace,
                                                           UINT *pColorSpaceSupport) override;
  virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace) override;
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(UINT BufferCount, UINT Width, UINT Height,
                                                   DXGI_FORMAT Format, UINT SwapChainFlags,
                                                   const UINT *pCreationNodeMask,
                                                   IUnknown *const *ppPresentQueue) override;
#pragma endregion
#pragma region IDXGISwapChain4
  virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type, UINT Size,
                                                   _In_reads_opt_(Size) void *pMetaData) override;
#pragma endregion

 private:
  enum D3DVersion { D3DVersion_11 = 11, D3DVersion_12 = 12, D3DVersion_Undefined };
  enum SwapChainVersion {SWAPCHAIN_0, SWAPCHAIN_1, SWAPCHAIN_2, SWAPCHAIN_3, SWAPCHAIN_4};

  DXGISwapChain(const DXGISwapChain &) = delete;
  DXGISwapChain &operator=(const DXGISwapChain &) = delete;

  IDXGISwapChain *swapChain_ = nullptr;

  // either d3d11 or d3d12 is used
  std::unique_ptr<GameOverlay::d3d11_renderer> d3d11Renderer_;
  std::unique_ptr<GameOverlay::d3d12_renderer> d3d12Renderer_;
  Microsoft::WRL::ComPtr<ID3D11Device> const d3d11Device_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> const d3d12CommandQueue_;

  D3DVersion d3dVersion_ = D3DVersion_Undefined;
  SwapChainVersion swapChainVersion_ = SWAPCHAIN_0;
};
