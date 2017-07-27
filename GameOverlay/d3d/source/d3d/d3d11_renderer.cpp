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

#include "d3d11_renderer.hpp"
#include <assert.h>
#include <vector>

#include "..\..\OverlayPS_Byte.h"
#include "..\..\OverlayVS_Byte.h"
#include "Recording\Capturing.h"
#include "Logging\MessageLog.h"

using Microsoft::WRL::ComPtr;

namespace GameOverlay {
d3d11_renderer::d3d11_renderer(ID3D11Device* device, IDXGISwapChain* swapchain)
    : device_(device), swapchain_(swapchain)
{
  InitCapturing();
  g_messageLog.Log(MessageLog::LOG_INFO, "D3D11", "initializing overlay");

  DXGI_SWAP_CHAIN_DESC swapchain_desc;
  swapchain_->GetDesc(&swapchain_desc);
  try {
    textRenderer_.reset(new TextRenderer{static_cast<int>(swapchain_desc.BufferDesc.Width),
                                         static_cast<int>(swapchain_desc.BufferDesc.Height)});
  }
  catch (const std::exception&) {
    return;
  }

  if (!CreateOverlayResources(swapchain_desc.BufferDesc.Width, swapchain_desc.BufferDesc.Height))
    return;

  device_->GetImmediateContext(&context_);

  if (!CreateOverlayRenderTarget()) return;
  if (!CreateOverlayTexture()) return;

  if (!RecordOverlayCommandList()) return;

  initSuccessfull_ = true;
  g_messageLog.Log(MessageLog::LOG_INFO, "D3D11", "overlay successfully initialized");
}

d3d11_renderer::~d3d11_renderer()
{}

void d3d11_renderer::on_present()
{
  if (initSuccessfull_ && RecordingState::GetInstance().DisplayOverlay()) {
    textRenderer_->DrawOverlay();
    UpdateOverlayTexture();

    context_->ExecuteCommandList(overlayCommandList_.Get(), true);
  }
}

bool d3d11_renderer::CreateOverlayRenderTarget()
{
  HRESULT hr;

  ComPtr<ID3D11Texture2D> backBuffer;
  hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
  assert(SUCCEEDED(hr));
  D3D11_TEXTURE2D_DESC backBufferDesc;
  backBuffer->GetDesc(&backBufferDesc);

  D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = backBufferDesc.Format;
  rtvDesc.ViewDimension = backBufferDesc.SampleDesc.Count > 0 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                                                              : D3D11_RTV_DIMENSION_TEXTURE2D;
  rtvDesc.Texture2D.MipSlice = 0;
  hr = device_->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &renderTarget_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11", "Failed creating overlay render target", hr);
    return false;
  }
  return true;
}

bool d3d11_renderer::CreateOverlayTexture()
{
  D3D11_TEXTURE2D_DESC displayDesc{};
  displayDesc.Width = textRenderer_->GetFullWidth();
  displayDesc.Height = textRenderer_->GetFullHeight();
  displayDesc.MipLevels = 1;
  displayDesc.ArraySize = 1;
  displayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  displayDesc.SampleDesc.Count = 1;
  displayDesc.SampleDesc.Quality = 0;
  displayDesc.Usage = D3D11_USAGE_DEFAULT;
  displayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  displayDesc.CPUAccessFlags = 0;
  HRESULT hr = device_->CreateTexture2D(&displayDesc, nullptr, &displayTexture_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Texture - failed creating display texture", hr);
    return false;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = displayDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = 1;

  hr = device_->CreateShaderResourceView(displayTexture_.Get(), &srvDesc, &displaySRV_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Texture - failed creating display shader resource view", hr);
    return false;
  }

  D3D11_TEXTURE2D_DESC stagingDesc = displayDesc;
  stagingDesc.Usage = D3D11_USAGE_STAGING;
  stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  stagingDesc.BindFlags = 0;
  hr = device_->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Texture - failed creating staging texture", hr);
    return false;
  }

  return true;
}

bool d3d11_renderer::CreateOverlayResources(int backBufferWidth, int backBufferHeight)
{
  // Create shaders
  HRESULT hr = device_->CreateVertexShader(g_OverlayVS, sizeof(g_OverlayVS), nullptr, &overlayVS_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Resources - failed creating vertex shader", hr);
    return false;
  }

  hr = device_->CreatePixelShader(g_OverlayPS, sizeof(g_OverlayPS), nullptr, &overlayPS_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Resources - failed creating pixel shader", hr);
    return false;
  }

  // create rasterizer
  D3D11_RASTERIZER_DESC rDesc = {};
  rDesc.CullMode = D3D11_CULL_BACK;
  rDesc.FillMode = D3D11_FILL_SOLID;
  rDesc.FrontCounterClockwise = false;
  rDesc.DepthClipEnable = true;
  hr = device_->CreateRasterizerState(&rDesc, &rasterizerState_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Resources - failed creating rasterizer state", hr);
    return false;
  }

  const auto screenPos = textRenderer_->GetScreenPos();
  std::pair<float, float> viewPortOffset = {static_cast<float>(screenPos.x),
                                            static_cast<float>(screenPos.y)};
  // create viewport offset constant buffer
  D3D11_BUFFER_DESC bDesc = {};
  bDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bDesc.ByteWidth = sizeof(viewPortOffset) * 2;
  bDesc.Usage = D3D11_USAGE_DEFAULT;

  D3D11_SUBRESOURCE_DATA cbData;
  cbData.pSysMem = &viewPortOffset;
  cbData.SysMemPitch = sizeof(viewPortOffset);
  cbData.SysMemSlicePitch = cbData.SysMemPitch;

  hr = device_->CreateBuffer(&bDesc, &cbData, &viewportOffsetCB_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Resources - failed creating viewport offset constant buffer", hr);
    return false;
  }

  D3D11_BLEND_DESC blendDesc{};
  blendDesc.AlphaToCoverageEnable = false;
  blendDesc.IndependentBlendEnable = false;
  blendDesc.RenderTarget[0].BlendEnable = true;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

  blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

  hr = device_->CreateBlendState(&blendDesc, &blendState_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay Resources - failed creating blend state", hr);
    return false;
  }

  const auto width = textRenderer_->GetFullWidth();
  const auto height = textRenderer_->GetFullHeight();

  // create the viewport and scissor rect
  scissorRect_ = {};
  scissorRect_.left = screenPos.x;
  scissorRect_.right = screenPos.x + width;
  scissorRect_.top = screenPos.y;
  scissorRect_.bottom = screenPos.y + height;

  viewPort_ = {};
  viewPort_.TopLeftX = static_cast<float>(screenPos.x);
  viewPort_.TopLeftY = static_cast<float>(screenPos.y);
  viewPort_.Width = static_cast<float>(width);
  viewPort_.Height = static_cast<float>(height);
  viewPort_.MaxDepth = 1.0f;

  return true;
}

void d3d11_renderer::UpdateOverlayTexture()
{
  CopyOverlayTexture();

  const auto copyArea = textRenderer_->GetCopyArea();
  if (textRenderer_->CopyFullArea()) {
    context_->CopyResource(displayTexture_.Get(), stagingTexture_.Get());
  }
  else {
    D3D11_BOX area = {
        0, 0, 0, static_cast<UINT>(copyArea.right), static_cast<UINT>(copyArea.bottom), 1};
    context_->CopySubresourceRegion(displayTexture_.Get(), 0, 0, 0, 0, stagingTexture_.Get(), 0,
                                    &area);
  }
}

void d3d11_renderer::CopyOverlayTexture()
{
  const auto textureData = textRenderer_->GetBitmapDataRead();
  if (textureData.dataPtr && textureData.size) {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context_->Map(stagingTexture_.Get(), 0, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_WARNING, "D3D11",
                       "mapping of display texture failed, HRESULT", hr);
      return;
    }

    memcpy(mappedResource.pData, textureData.dataPtr, textureData.size);
    context_->Unmap(stagingTexture_.Get(), 0);
  }
  textRenderer_->UnlockBitmapData();
}

bool d3d11_renderer::RecordOverlayCommandList()
{
  ComPtr<ID3D11DeviceContext> overlayContext;
  HRESULT hr = device_->CreateDeferredContext(0, &overlayContext);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay CMD List - failed creating defferred context", hr);
    return false;
  }

  overlayContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  overlayContext->IASetInputLayout(nullptr);

  overlayContext->VSSetShader(overlayVS_.Get(), nullptr, 0);
  overlayContext->PSSetShader(overlayPS_.Get(), nullptr, 0);
  ID3D11ShaderResourceView* srvs[] = {displaySRV_.Get()};
  overlayContext->PSSetShaderResources(0, 1, srvs);
  ID3D11Buffer* cbs[] = {viewportOffsetCB_.Get()};
  overlayContext->PSSetConstantBuffers(0, 1, cbs);

  ID3D11RenderTargetView* rtv[] = {renderTarget_.Get()};
  overlayContext->OMSetRenderTargets(1, rtv, nullptr);
  overlayContext->OMSetBlendState(blendState_.Get(), 0, 0xffffffff);

  overlayContext->RSSetState(rasterizerState_.Get());
  overlayContext->RSSetViewports(1, &viewPort_);
  overlayContext->RSSetScissorRects(1, &scissorRect_);

  overlayContext->Draw(3, 0);

  hr = overlayContext->FinishCommandList(false, &overlayCommandList_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D11",
                     " Overlay CMD List - failed finishing command list", hr);
    return false;
  }
  return true;
}
}
