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

#include "d3d12_renderer.hpp"
#include <assert.h>
#include "../deps/d3dx12/d3dx12.h"

#include "Recording/Capturing.h"

#include "../../OverlayPS_Byte.h"
#include "../../OverlayVS_Byte.h"
#include "Logging/MessageLog.h"

using Microsoft::WRL::ComPtr;

namespace GameOverlay {
d3d12_renderer::d3d12_renderer(ID3D12CommandQueue* commandqueue, IDXGISwapChain3* swapchain)
    : queue_(commandqueue), swapchain_(swapchain)
{
  InitCapturing();

  g_messageLog.LogInfo("D3D12", "Initializing overlay.");
  queue_->GetDevice(IID_PPV_ARGS(&device_));

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
  swapchain_->GetDesc1(&swapchain_desc);

  overlayBitmap_.reset(new OverlayBitmap());
  if (!overlayBitmap_->Init(static_cast<int>(swapchain_desc.Width),
                            static_cast<int>(swapchain_desc.Height), OverlayBitmap::API::DX12)) {
    return;
  }

  bufferCount_ = swapchain_desc.BufferCount;

  if (!CreateFrameFences()) return;
  if (!CreateCMDList()) return;
  if (!CreateRenderTargets()) return;
  if (!CreateRootSignature()) return;
  if (!CreatePipelineStateObject()) return;
  if (!CreateOverlayTextures()) return;
  if (!CreateConstantBuffer()) return;

  initSuccessfull_ = true;
  g_messageLog.LogInfo("D3D12", "Overlay successfully initialized.");
}

d3d12_renderer::d3d12_renderer(ID3D12CommandQueue* commandqueue,
                               Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetHeap,
                               std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets,
                               UINT rtvHeapDescriptorSize, int bufferCount, int backBufferWidth,
                               int backBufferHeight)
    : queue_(commandqueue),
      renderTargetHeap_(renderTargetHeap),
      renderTargets_(renderTargets),
      rtvHeapDescriptorSize_(rtvHeapDescriptorSize),
      bufferCount_(bufferCount)
{
  overlayBitmap_.reset(new OverlayBitmap());
  if (!overlayBitmap_->Init(static_cast<int>(backBufferWidth), static_cast<int>(backBufferHeight),
                            OverlayBitmap::API::DX12)) {
    return;
  }

  queue_->GetDevice(IID_PPV_ARGS(&device_));
  if (!CreateFrameFences()) return;
  if (!CreateCMDList()) return;
  if (!CreateRootSignature()) return;
  if (!CreatePipelineStateObject()) return;
  if (!CreateOverlayTextures()) return;
  if (!CreateConstantBuffer()) return;

  initSuccessfull_ = true;
  g_messageLog.LogInfo("D3D12", "Overlay successfully initialized.");
}

d3d12_renderer::~d3d12_renderer()
{
  for (auto handle : frameFenceEvents_) {
    CloseHandle(handle);
  }
}

namespace {
void WaitForFence(ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent)
{
  if (fence->GetCompletedValue() < completionValue) {
    fence->SetEventOnCompletion(completionValue, waitEvent);
    WaitForSingleObject(waitEvent, INFINITE);
  }
}
}  // namespace

bool d3d12_renderer::on_present()
{
  const auto currentBufferIndex = swapchain_->GetCurrentBackBufferIndex();
  return on_present(currentBufferIndex);
}

bool d3d12_renderer::on_present(int backBufferIndex)
{
  if (!initSuccessfull_) {
    return false;
  }

  WaitForFence(frameFences_[backBufferIndex].Get(), frameFenceValues_[backBufferIndex],
               frameFenceEvents_[backBufferIndex]);

  commandPool_->Reset();

  overlayBitmap_->DrawOverlay();
  UpdateOverlayTexture();

  DrawOverlay(backBufferIndex);

  return true;
}

void d3d12_renderer::UpdateOverlayPosition()
{
  const auto width = overlayBitmap_->GetFullWidth();
  const auto height = overlayBitmap_->GetFullHeight();
  const auto screenPos = overlayBitmap_->GetScreenPos();
  ConstantBuffer constantBuffer;
  constantBuffer.screenPosX = static_cast<float>(screenPos.x);
  constantBuffer.screenPosY = static_cast<float>(screenPos.y);

  // update the viewport
  viewPort_.TopLeftX = constantBuffer.screenPosX;
  viewPort_.TopLeftY = constantBuffer.screenPosY;
  viewPort_.Width = static_cast<float>(width);
  viewPort_.Height = static_cast<float>(height);
  viewPort_.MaxDepth = 1.0f;
  viewPort_.MinDepth = 0.0f;

  rectScissor_.left = screenPos.x;
  rectScissor_.top = screenPos.y;
  rectScissor_.right = screenPos.x + width;
  rectScissor_.bottom = screenPos.y + height;

  UpdateConstantBuffer(constantBuffer);
}

bool d3d12_renderer::CreateCMDList()
{
  commandPool_.Reset();
  commandList_.Reset();

  HRESULT hr;
  hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandPool_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateCommandAllocator failed", hr);
    return false;
  }

  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandPool_.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateCommandList failed", hr);
    return false;
  }

  commandList_->Close();

  return true;
}

bool d3d12_renderer::CreateRenderTargets()
{
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = bufferCount_;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&renderTargetHeap_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateRenderTargets - CreateDescriptorHeap failed", hr);
    return false;
  }

  rtvHeapDescriptorSize_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetHeap_->GetCPUDescriptorHandleForHeapStart());
  renderTargets_.resize(bufferCount_);

  // Create a RTV for each frame.
  for (UINT buffer_index = 0; buffer_index < renderTargets_.size(); buffer_index++) {
    hr = swapchain_->GetBuffer(buffer_index, IID_PPV_ARGS(&renderTargets_[buffer_index]));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "CreateRenderTargets - GetBuffer failed", hr);
      return false;
    }
    device_->CreateRenderTargetView(renderTargets_[buffer_index].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, rtvHeapDescriptorSize_);
  }
  return true;
}

bool d3d12_renderer::CreateFrameFences()
{
  currFenceValue_ = 0;

  frameFences_.resize(bufferCount_);
  for (int i = 0; i < bufferCount_; ++i) {
    HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences_[i]));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "Create Fence failed", hr);
      return false;
    }
    frameFenceEvents_.push_back(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    frameFenceValues_.push_back(0);
  }
  return true;
}

bool d3d12_renderer::CreateRootSignature()
{
  D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
  featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

  if (FAILED(device_->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,
                                          sizeof(featureData)))) {
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  CD3DX12_DESCRIPTOR_RANGE1 range[1];
  range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0,
                D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

  CD3DX12_ROOT_PARAMETER1 rootParam[2];
  rootParam[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);
  rootParam[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
                                        D3D12_SHADER_VISIBILITY_PIXEL);

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescOverlay;
  rootSignatureDescOverlay.Init_1_1(_countof(rootParam), rootParam, 0, nullptr,
                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> signatureOverlay;
  ComPtr<ID3DBlob> errorOverlay;

  HRESULT hr = D3DX12SerializeVersionedRootSignature(
      &rootSignatureDescOverlay, featureData.HighestVersion, &signatureOverlay, &errorOverlay);
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12",
                          "CreateRootSignature - D3DX12SerializeVersionedRootSignature failed", hr);
    return false;
  }

  hr = device_->CreateRootSignature(0, signatureOverlay->GetBufferPointer(),
                                    signatureOverlay->GetBufferSize(),
                                    IID_PPV_ARGS(&rootSignature_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateRootSignature - CreateRootSignature failed", hr);
    return false;
  }
  return true;
}

bool d3d12_renderer::CreatePipelineStateObject()
{
  D3D12_BLEND_DESC blendDesc = {};
  blendDesc.AlphaToCoverageEnable = false;
  blendDesc.IndependentBlendEnable = false;
  blendDesc.RenderTarget[0].BlendEnable = true;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.VS.BytecodeLength = sizeof(g_OverlayVS);
  psoDesc.VS.pShaderBytecode = g_OverlayVS;
  psoDesc.PS.BytecodeLength = sizeof(g_OverlayPS);
  psoDesc.PS.pShaderBytecode = g_OverlayPS;
  psoDesc.InputLayout = {nullptr, 0};
  psoDesc.pRootSignature = rootSignature_.Get();
  psoDesc.NumRenderTargets = 1;

  if (swapchain_) {
    ComPtr<ID3D12Resource> buffer;
    HRESULT hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&buffer));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "CreatePipelineStateObject - GetBuffer failed", hr);
      return false;
    }

    psoDesc.RTVFormats[0] = buffer->GetDesc().Format;
  }
  else {
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  }

  psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
  psoDesc.InputLayout.NumElements = 0;
  psoDesc.InputLayout.pInputElementDescs = nullptr;
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState = blendDesc;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.DepthStencilState.DepthEnable = false;
  psoDesc.DepthStencilState.StencilEnable = false;
  psoDesc.SampleMask = 0xFFFFFFFF;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreatePipelineStateObject - CreateGraphicsPipelineState failed",
                          hr);
    return false;
  }
  return true;
}

bool d3d12_renderer::CreateOverlayTextures()
{
  commandList_->Reset(commandPool_.Get(), nullptr);

  HRESULT hr;
  const auto displayTextureWidth = overlayBitmap_->GetFullWidth();
  const auto displayTextureHeight = overlayBitmap_->GetFullHeight();
  // Create display texture
  {
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.Width = displayTextureWidth;
    textureDesc.Height = displayTextureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    const auto textureHeapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device_->CreateCommittedResource(
        &textureHeapType, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&displayTexture_));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12",
                            "CreateOverlayTextures - CreateCommittedResource displayTexture_", hr);
      return false;
    }

    // Create upload buffer
    const auto uploadBufferSize = GetRequiredIntermediateSize(displayTexture_.Get(), 0, 1);
    const auto uploadBufferHeapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto uploadBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    hr = device_->CreateCommittedResource(
        &uploadBufferHeapType, D3D12_HEAP_FLAG_NONE, &uploadBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer_));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12",
                            "CreateOverlayTextures - CreateCommittedResource uploadTexture", hr);
      return false;
    }

    device_->GetCopyableFootprints(&textureDesc, 0, 1, 0, &uploadFootprint_, nullptr, nullptr,
                                   nullptr);

    // create srv
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 2;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&displayHeap_));
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "CreateOverlayTextures - CreateDescriptorHeap displayHeap_",
                            hr);
      return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device_->CreateShaderResourceView(displayTexture_.Get(), &srvDesc,
                                      displayHeap_->GetCPUDescriptorHandleForHeapStart());
  }

  // execute command list
  hr = commandList_->Close();
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateOverlayTextures - Failed closing CommandList", hr);
    return false;
  }
  ID3D12CommandList* ppCommandLists[] = {commandList_.Get()};
  queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  WaitForCompletion();
  return true;
}

bool d3d12_renderer::CreateConstantBuffer()
{
  viewportOffsetCB_.Reset();
  HRESULT hr = device_->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer)), D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr, IID_PPV_ARGS(&viewportOffsetCB_));

  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "CreateConstantBuffer - failed to create constant buffer.", hr);
    return false;
  }
  return true;
}

void d3d12_renderer::UpdateConstantBuffer(const ConstantBuffer& constantBuffer)
{
  void* mappedMemory;
  HRESULT hr = viewportOffsetCB_->Map(0, nullptr, &mappedMemory);
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "UpdateConstantBuffer - failed to update constant buffer.", hr);
    return;
  }
  ConstantBuffer* mappedConstantBuffer = static_cast<ConstantBuffer*>(mappedMemory);
  mappedConstantBuffer->screenPosX = constantBuffer.screenPosX;
  mappedConstantBuffer->screenPosY = constantBuffer.screenPosY;
  viewportOffsetCB_->Unmap(0, nullptr);
}

void d3d12_renderer::UpdateOverlayTexture()
{
  const auto textureData = overlayBitmap_->GetBitmapDataRead();
  if (textureData.dataPtr && textureData.size) {
    CD3DX12_RANGE readRange(0, 0);
    HRESULT hr = uploadBuffer_->Map(0, &readRange, &uploadDataPtr_);
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "UpdateOverlayTexture - Mapping failed.", hr);
      return;
    }

    memcpy(uploadDataPtr_, textureData.dataPtr, textureData.size);
    uploadBuffer_->Unmap(0, &readRange);

    commandList_->Reset(commandPool_.Get(), nullptr);
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "UpdateOverlayTexture - Failed to reset command list.", hr);
      return;
    }

    const auto transitionWrite = CD3DX12_RESOURCE_BARRIER::Transition(
        displayTexture_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST, 0);
    commandList_->ResourceBarrier(1, &transitionWrite);

    CD3DX12_TEXTURE_COPY_LOCATION src_resource =
        CD3DX12_TEXTURE_COPY_LOCATION(uploadBuffer_.Get(), uploadFootprint_);
    CD3DX12_TEXTURE_COPY_LOCATION dest_resource;
    dest_resource.pResource = displayTexture_.Get();
    dest_resource.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest_resource.SubresourceIndex = 0;

    const auto& copyArea = overlayBitmap_->GetCopyArea();
    const D3D12_BOX area = {
        static_cast<UINT>(copyArea.left),  static_cast<UINT>(copyArea.top),    0,
        static_cast<UINT>(copyArea.right), static_cast<UINT>(copyArea.bottom), 1};
    commandList_->CopyTextureRegion(&dest_resource, 0, 0, 0, &src_resource, &area);

    const auto transitionRead =
        CD3DX12_RESOURCE_BARRIER::Transition(displayTexture_.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
    commandList_->ResourceBarrier(1, &transitionRead);

    hr = commandList_->Close();
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "UpdateOverlayTexture - Closing command list failed.", hr);
      return;
    }

    // Execute draw commands
    ID3D12CommandList* const commandlists[] = {commandList_.Get()};
    queue_->ExecuteCommandLists(ARRAYSIZE(commandlists), commandlists);
  }
  overlayBitmap_->UnlockBitmapData();
}

void d3d12_renderer::DrawOverlay(int currentIndex)
{
  HRESULT hr = commandList_->Reset(commandPool_.Get(), nullptr);
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "DrawOverlay - Failed to reset command list.", hr);
    return;
  }

  UpdateOverlayPosition();

  commandList_->SetPipelineState(pso_.Get());
  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  ID3D12DescriptorHeap* ppHeaps[] = {displayHeap_.Get()};
  commandList_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  commandList_->SetGraphicsRootDescriptorTable(
      0, displayHeap_->GetGPUDescriptorHandleForHeapStart());
  commandList_->SetGraphicsRootConstantBufferView(1, viewportOffsetCB_->GetGPUVirtualAddress());
  commandList_->RSSetViewports(1, &viewPort_);
  commandList_->RSSetScissorRects(1, &rectScissor_);

  const auto transitionRender = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[currentIndex].Get(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
  commandList_->ResourceBarrier(1, &transitionRender);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetHeap_->GetCPUDescriptorHandleForHeapStart(),
                                          currentIndex, rtvHeapDescriptorSize_);
  commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->DrawInstanced(3, 1, 0, 0);

  const auto transitionPresent = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[currentIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT, 0);
  commandList_->ResourceBarrier(1, &transitionPresent);

  hr = commandList_->Close();
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "DrawOverlay - Failed to close command list.", hr);
    return;
  }

  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  queue_->ExecuteCommandLists(1, commandLists);

  queue_->Signal(frameFences_[currentIndex].Get(), currFenceValue_);
  frameFenceValues_[currentIndex] = currFenceValue_;
  ++currFenceValue_;
}

void d3d12_renderer::WaitForCompletion()
{
  HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "WaitForCompletion - CreateFence failed", hr);
    return;
  }
  fenceValue_ = 1;

  fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fenceEvent_ == nullptr) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    if (FAILED(hr)) {
      g_messageLog.LogError("D3D12", "WaitForCompletion - CreateEvent failed", hr);
      return;
    }
  }

  const auto currFenceValue = fenceValue_;
  hr = queue_->Signal(fence_.Get(), currFenceValue);
  if (FAILED(hr)) {
    g_messageLog.LogError("D3D12", "WaitForCompletion - Signal queue failed", hr);
    return;
  }

  if (fence_->GetCompletedValue() < currFenceValue) {
    hr = fence_->SetEventOnCompletion(currFenceValue, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }
}
}  // namespace GameOverlay
