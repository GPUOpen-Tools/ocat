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

#include "../../OverlayPS_Byte.h"
#include "../../OverlayVS_Byte.h"
#include "Recording/Capturing.h"
#include "Logging/MessageLog.h"
#include "Rendering/ConstantBuffer.h"

using Microsoft::WRL::ComPtr;

namespace GameOverlay {
d3d11_renderer::d3d11_renderer(ID3D11Device* device, IDXGISwapChain* swapchain)
  : device_(device), swapchain_(swapchain)
{
  InitCapturing();
  g_messageLog.LogInfo("D3D11", "Initializing overlay.");

  DXGI_SWAP_CHAIN_DESC swapchain_desc;
  swapchain_->GetDesc(&swapchain_desc);
  overlayBitmap_.reset(new OverlayBitmap());
  if (!overlayBitmap_->Init(
    static_cast<int>(swapchain_desc.BufferDesc.Width),
    static_cast<int>(swapchain_desc.BufferDesc.Height),
    OverlayBitmap::API::DX11))
  {
    return;
  }

  if (!CreateOverlayResources(swapchain_desc.BufferDesc.Width, swapchain_desc.BufferDesc.Height))
  {
    return;
  }

  device_->GetImmediateContext(&context_);

  if (!CreateOverlayRenderTarget())
  {
    return;
  }

  if (!CreateOverlayTexture())
  {
    return;
  }

  if (!RecordOverlayCommandList())
  {
    status = InitializationStatus::IMMEDIATE_CONTEXT_INITIALIZED;
    return;
  }

  status = InitializationStatus::DEFERRED_CONTEXT_INITIALIZED;
  g_messageLog.LogInfo("D3D11", "Overlay successfully initialized.");
}

d3d11_renderer::d3d11_renderer(ID3D11Device *device,
  std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> renderTargets,
  int backBufferWidth, int backBufferHeight)
  : device_(device), renderTargets_(renderTargets)
{
  overlayBitmap_.reset(new OverlayBitmap());
  if (!overlayBitmap_->Init(
    static_cast<int>(backBufferWidth),
    static_cast<int>(backBufferHeight),
    OverlayBitmap::API::DX11))
  {
    return;
  }

  if (!CreateOverlayResources(backBufferWidth, backBufferHeight))
  {
    return;
  }

  device_->GetImmediateContext(&context_);

  if (!CreateOverlayTexture())
  {
    return;
  }

  if (!RecordOverlayCommandList())
  {
    status = InitializationStatus::IMMEDIATE_CONTEXT_INITIALIZED;
    return;
  }

  status = InitializationStatus::DEFERRED_CONTEXT_INITIALIZED;
  g_messageLog.LogInfo("D3D11", "Overlay successfully initialized.");
}

d3d11_renderer::~d3d11_renderer()
{
  // Empty
}

bool d3d11_renderer::RecordOverlayCommandList()
{
  ComPtr<ID3D11DeviceContext> overlayContext;
  HRESULT hr = device_->CreateDeferredContext(0, &overlayContext);
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D11", "Overlay CMD List - failed creating deferred context", hr);
    return false;
  }

  overlayCommandList_.resize(renderTargets_.size());

  for (uint32_t i = 0; i < renderTargets_.size(); i++) {

    overlayContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    overlayContext->IASetInputLayout(nullptr);

    overlayContext->VSSetShader(overlayVS_.Get(), nullptr, 0);
    overlayContext->PSSetShader(overlayPS_.Get(), nullptr, 0);
    ID3D11ShaderResourceView* srvs[] = { displaySRV_.Get() };
    overlayContext->PSSetShaderResources(0, 1, srvs);
    ID3D11Buffer* cbs[] = { viewportOffsetCB_.Get() };
    overlayContext->PSSetConstantBuffers(0, 1, cbs);

    ID3D11RenderTargetView* rtv[] = { renderTargets_[i].Get() };
    overlayContext->OMSetRenderTargets(1, rtv, nullptr);
    overlayContext->OMSetBlendState(blendState_.Get(), 0, 0xffffffff);

    overlayContext->RSSetState(rasterizerState_.Get());
    overlayContext->RSSetViewports(1, &viewPort_);
    overlayContext->Draw(3, 0);

    overlayCommandList_[i].Reset();
    hr = overlayContext->FinishCommandList(false, &overlayCommandList_[i]);
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D11", "Overlay CMD List - failed finishing command list", hr);
      return false;
    }
  }
  return true;
}

// just use first render target, probably we only have one here anyways
bool d3d11_renderer::on_present()
{
  return on_present(0);
}

bool d3d11_renderer::on_present(int backBufferIndex)
{
  if (status == InitializationStatus::UNINITIALIZED)
  {
    return false;
  }

  overlayBitmap_->DrawOverlay();

  if (!UpdateOverlayPosition())
  {
    return false;
  }

  UpdateOverlayTexture();

  switch (status)
  {
  case InitializationStatus::DEFERRED_CONTEXT_INITIALIZED:
  {
    context_->ExecuteCommandList(overlayCommandList_[backBufferIndex].Get(), true);
    return true;
  }
  case InitializationStatus::IMMEDIATE_CONTEXT_INITIALIZED:
  {
    // save current context_ state
    D3D_PRIMITIVE_TOPOLOGY topology;
    context_->IAGetPrimitiveTopology(&topology);
    Microsoft::WRL::ComPtr<ID3D11InputLayout> pInputLayout;
    context_->IAGetInputLayout(&pInputLayout);
    Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11ClassInstance> pVSClassInstances;
    UINT vsNumClassInstances;
    context_->VSGetShader(&pVertexShader, &pVSClassInstances, &vsNumClassInstances);
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11ClassInstance> pPSClassInstances;
    UINT psNumClassInstances;
    context_->PSGetShader(&pPixelShader, &pPSClassInstances, &psNumClassInstances);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pShaderResourceView;
    context_->PSGetShaderResources(0, 1, &pShaderResourceView);
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffers;
    context_->PSGetConstantBuffers(0, 1, &constantBuffers);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDepthStencilView;
    context_->OMGetRenderTargets(1, &pRenderTargetView, &pDepthStencilView);
    Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState;
    FLOAT blendFactor[4];
    UINT sampleMask;
    context_->OMGetBlendState(&pBlendState, &blendFactor[0], &sampleMask);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
    context_->RSGetState(&rasterizerState);
    UINT numViewports;
    context_->RSGetViewports(&numViewports, NULL);
    std::vector<D3D11_VIEWPORT> viewports(numViewports);
    context_->RSGetViewports(&numViewports, &viewports[0]);

    // record overlay commands
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(nullptr);
    context_->VSSetShader(overlayVS_.Get(), nullptr, 0);
    context_->PSSetShader(overlayPS_.Get(), nullptr, 0);
    ID3D11ShaderResourceView* srvs[] = { displaySRV_.Get() };
    context_->PSSetShaderResources(0, 1, srvs);
    ID3D11Buffer* cbs[] = { viewportOffsetCB_.Get() };
    context_->PSSetConstantBuffers(0, 1, cbs);
    ID3D11RenderTargetView* rtv[] = { renderTargets_[backBufferIndex].Get() };
    context_->OMSetRenderTargets(1, rtv, nullptr);
    context_->OMSetBlendState(blendState_.Get(), 0, 0xffffffff);
    context_->RSSetState(rasterizerState_.Get());
    context_->RSSetViewports(1, &viewPort_);
    context_->Draw(3, 0);

    // restore context to previous state
    context_->IASetPrimitiveTopology(topology);
    context_->IASetInputLayout(pInputLayout.Get());
    ID3D11ClassInstance* vsCIs[] = {pVSClassInstances.Get()};
    context_->VSSetShader(pVertexShader.Get(), vsCIs, vsNumClassInstances);
    ID3D11ClassInstance* psCIs[] = {pPSClassInstances.Get()};
    context_->PSSetShader(pPixelShader.Get(), psCIs, psNumClassInstances);
    ID3D11ShaderResourceView* srvs_prev[] = {pShaderResourceView.Get()};
    context_->PSSetShaderResources(0, 1, srvs_prev);
    ID3D11Buffer* cbs_prev[] = {constantBuffers.Get()};
    context_->PSSetConstantBuffers(0, 1, cbs_prev);
    ID3D11RenderTargetView* rtv_prev[] = {pRenderTargetView.Get()};
    context_->OMSetRenderTargets(1, rtv_prev, pDepthStencilView.Get());
    context_->OMSetBlendState(pBlendState.Get(), blendFactor, sampleMask);
    context_->RSSetState(rasterizerState.Get());
    context_->RSSetViewports(numViewports, viewports.data());
    return true;
  }
  }

  return false;
}

bool d3d11_renderer::CreateOverlayRenderTarget()
{
  HRESULT hr;

  ComPtr<ID3D11Texture2D> backBuffer;
  hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11", "Failed retrieving back buffer.", hr);
    return false;
  }
  // we just create one render target here
  renderTargets_.resize(1);

  D3D11_TEXTURE2D_DESC backBufferDesc;
  backBuffer->GetDesc(&backBufferDesc);

  D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = backBufferDesc.Format;
  rtvDesc.ViewDimension = backBufferDesc.SampleDesc.Count > 0 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
    : D3D11_RTV_DIMENSION_TEXTURE2D;
  rtvDesc.Texture2D.MipSlice = 0;
  hr = device_->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &renderTargets_[0]);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11", "Failed creating overlay render target.", hr);
    return false;
  }
  return true;
}

bool d3d11_renderer::CreateOverlayTexture()
{
  D3D11_TEXTURE2D_DESC displayDesc{};
  displayDesc.Width = overlayBitmap_->GetFullWidth();
  displayDesc.Height = overlayBitmap_->GetFullHeight();
  displayDesc.MipLevels = 1;
  displayDesc.ArraySize = 1;
  displayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  displayDesc.SampleDesc.Count = 1;
  displayDesc.SampleDesc.Quality = 0;
  displayDesc.Usage = D3D11_USAGE_DEFAULT;
  displayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  displayDesc.CPUAccessFlags = 0;
  HRESULT hr = device_->CreateTexture2D(&displayDesc, nullptr, &displayTexture_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Texture - failed creating display texture.", hr);
    return false;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = displayDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = 1;

  hr = device_->CreateShaderResourceView(displayTexture_.Get(), &srvDesc, &displaySRV_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Texture - failed creating display shader resource view.", hr);
    return false;
  }

  D3D11_TEXTURE2D_DESC stagingDesc = displayDesc;
  stagingDesc.Usage = D3D11_USAGE_STAGING;
  stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  stagingDesc.BindFlags = 0;
  hr = device_->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Texture - failed creating staging texture.", hr);
    return false;
  }

  return true;
}

bool d3d11_renderer::CreateOverlayResources(int backBufferWidth, int backBufferHeight)
{
  // Create shaders
  HRESULT hr = device_->CreateVertexShader(g_OverlayVS, sizeof(g_OverlayVS), nullptr, &overlayVS_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Resources - failed creating vertex shader.", hr);
    return false;
  }

  hr = device_->CreatePixelShader(g_OverlayPS, sizeof(g_OverlayPS), nullptr, &overlayPS_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Resources - failed creating pixel shader.", hr);
    return false;
  }

  // create rasterizer
  D3D11_RASTERIZER_DESC rDesc = {};
  rDesc.CullMode = D3D11_CULL_BACK;
  rDesc.FillMode = D3D11_FILL_SOLID;
  rDesc.FrontCounterClockwise = false;
  rDesc.DepthClipEnable = true;
  hr = device_->CreateRasterizerState(&rDesc, &rasterizerState_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Resources - failed creating rasterizer state.", hr);
    return false;
  }

  // create viewport offset constant buffer
  D3D11_BUFFER_DESC constantBufferDesc = {};
  constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  constantBufferDesc.ByteWidth = static_cast<UINT>(sizeof(ConstantBuffer));
  constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  constantBufferDesc.MiscFlags = 0;
  constantBufferDesc.StructureByteStride = 0;

  hr = device_->CreateBuffer(&constantBufferDesc, NULL, &viewportOffsetCB_);
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Resources - failed creating viewport offset constant buffer.", hr);
    return false;
  }

  D3D11_BLEND_DESC blendDesc = {};
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
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D11",
      "Overlay Resources - failed creating blend state.", hr);
    return false;
  }
  return true;
}

bool d3d11_renderer::UpdateOverlayPosition()
{
  const auto width = static_cast<float>(overlayBitmap_->GetFullWidth());
  const auto height = static_cast<float>(overlayBitmap_->GetFullHeight());
  const auto screenPos = overlayBitmap_->GetScreenPos();
  const auto screenPosX = static_cast<float>(screenPos.x);
  const auto screenPosY = static_cast<float>(screenPos.y);

  if (viewPort_.Width == width 
    && viewPort_.Height == height
    && viewPort_.TopLeftX == screenPosX
    && viewPort_.TopLeftY == screenPosY)
  {
    // Overlay position is up to date.
    return true;
  }

  // update the constant buffer
  ConstantBuffer constantBuffer;
  constantBuffer.screenPosX = screenPosX;
  constantBuffer.screenPosY = screenPosY;

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  HRESULT hr = context_->Map(viewportOffsetCB_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  if (FAILED(hr))
  {
    g_messageLog.LogWarning("D3D11",
      "Mapping of constant buffer failed, HRESULT", hr);
    return false;
  }
  auto pConstantBuffer = reinterpret_cast<ConstantBuffer*>(mappedResource.pData);
  std::memcpy(pConstantBuffer, &constantBuffer, sizeof(ConstantBuffer));
  context_->Unmap(viewportOffsetCB_.Get(), 0);

  // update the viewport
  viewPort_.TopLeftX = constantBuffer.screenPosX;
  viewPort_.TopLeftY = constantBuffer.screenPosY;
  viewPort_.Width = width;
  viewPort_.Height = height;
  viewPort_.MaxDepth = 1.0f;
  viewPort_.MinDepth = 0.0f;

  // we are forced to record the command list again 
  // to apply the changes to the viewport.
  if (!RecordOverlayCommandList())
  {
    status = InitializationStatus::IMMEDIATE_CONTEXT_INITIALIZED;
  }
  else {
    status = InitializationStatus::DEFERRED_CONTEXT_INITIALIZED;
  }

  return true;
}

void d3d11_renderer::UpdateOverlayTexture()
{
  CopyOverlayTexture();
  const auto copyArea = overlayBitmap_->GetCopyArea();
  context_->CopyResource(displayTexture_.Get(), stagingTexture_.Get());
}

void d3d11_renderer::CopyOverlayTexture()
{
  const auto textureData = overlayBitmap_->GetBitmapDataRead();
  if (textureData.dataPtr != nullptr && textureData.size > 0) 
  {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context_->Map(stagingTexture_.Get(), 0, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr))
    {
      g_messageLog.LogWarning("D3D11",
        "Mapping of display texture failed, HRESULT", hr);
      return;
    }

    memcpy(mappedResource.pData, textureData.dataPtr, textureData.size);
    context_->Unmap(stagingTexture_.Get(), 0);
  }
  overlayBitmap_->UnlockBitmapData();
}
}
