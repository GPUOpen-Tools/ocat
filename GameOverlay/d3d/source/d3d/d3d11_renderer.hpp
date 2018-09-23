// Copyright 2016 Patrick Mours.All rights reserved.
//
// https://github.com/crosire/GameOverlay
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


#include <d3d11.h>
#include <wrl.h>
#include <vector>

#include "Recording/PerformanceCounter.hpp"
#include "Rendering/OverlayBitmap.h"

namespace GameOverlay {
	enum class InitializationStatus
	{
		DEFERRED_CONTEXT_INITIALIZED,
		IMMEDIATE_CONTEXT_INITIALIZED,
		UNINITIALIZED
	};

class d3d11_renderer final 
{
public:
  d3d11_renderer(ID3D11Device *device, IDXGISwapChain *swapchain);
  d3d11_renderer(ID3D11Device *device,
	  std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> renderTargets_,
	  int backBufferWidth, int backBufferHeight);
  ~d3d11_renderer();


  bool on_present();
  bool on_present(int backBufferIndex);

  D3D11_VIEWPORT GetViewport() { return viewPort_; }

private:

  bool CreateOverlayRenderTarget();
  bool CreateOverlayTexture();
  bool CreateOverlayResources(int backBufferWidth, int backBufferHeight);
  bool RecordOverlayCommandList();

  void CopyOverlayTexture();
  bool UpdateOverlayPosition();
  void UpdateOverlayTexture();

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain_;

  Microsoft::WRL::ComPtr<ID3D11VertexShader> overlayVS_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> overlayPS_;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> displayTexture_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> displaySRV_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> viewportOffsetCB_;
  std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> renderTargets_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;
  std::vector<Microsoft::WRL::ComPtr<ID3D11CommandList>> overlayCommandList_;
  D3D11_VIEWPORT viewPort_;
  std::unique_ptr<OverlayBitmap> overlayBitmap_;

  InitializationStatus status = InitializationStatus::UNINITIALIZED;
};
}
