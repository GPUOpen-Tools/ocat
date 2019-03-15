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

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <vector>

#include "Rendering/OverlayBitmap.h"
#include "Rendering/ConstantBuffer.h"

namespace GameOverlay {

class d3d12_renderer 
{
public:
  d3d12_renderer(ID3D12CommandQueue *commandqueue, IDXGISwapChain3 *swapchain);
  d3d12_renderer(ID3D12CommandQueue *commandqueue,
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetHeap,
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets,
    UINT rtvHeapDescriptorSize, int bufferCount, int backBufferWidth, int backBufferHeight);
  ~d3d12_renderer();

  bool on_present();
  bool on_present(int backBufferIndex);

  D3D12_VIEWPORT GetViewport() { return viewPort_; }

private:
  bool CreateCMDList();
  bool CreateRenderTargets();
  bool CreateFrameFences();
  bool CreateRootSignature();
  bool CreatePipelineStateObject();
  bool CreateOverlayTextures();
  bool CreateConstantBuffer();
  void UpdateConstantBuffer(const ConstantBuffer & constantBuffer);

  void UpdateOverlayTexture();
  void UpdateOverlayPosition();
  void DrawOverlay(int currentIndex);

  void WaitForCompletion();

  std::unique_ptr<OverlayBitmap> overlayBitmap_;
  Microsoft::WRL::ComPtr<ID3D12Device> device_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain_;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandPool_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetHeap_;
  UINT rtvHeapDescriptorSize_;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets_;

  Microsoft::WRL::ComPtr<ID3D12Resource> displayTexture_;
  Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT uploadFootprint_;
  void *uploadDataPtr_;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> displayHeap_;

  Microsoft::WRL::ComPtr<ID3D12Resource> viewportOffsetCB_;
  D3D12_VIEWPORT viewPort_;
  D3D12_RECT rectScissor_;

  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  UINT64 fenceValue_;
  HANDLE fenceEvent_;

  std::vector<Microsoft::WRL::ComPtr<ID3D12Fence>> frameFences_;
  std::vector<HANDLE> frameFenceEvents_;
  std::vector<UINT64> frameFenceValues_;
  UINT64 currFenceValue_ = 0;

  int bufferCount_ = 0;
  bool initSuccessfull_ = false;
};
}
