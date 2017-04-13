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
#include "..\deps\d3dx12\d3dx12.h"

#include "Capturing.h"

#include "..\..\OverlayPS_Byte.h"
#include "..\..\OverlayVS_Byte.h"
#include "MessageLog.h"

using Microsoft::WRL::ComPtr;

namespace gameoverlay {
d3d12_renderer::d3d12_renderer(ID3D12CommandQueue* commandqueue, IDXGISwapChain3* swapchain)
    : queue_(commandqueue), swapchain_(swapchain)
{
  InitCapturing();

  g_messageLog.Log(MessageLog::LOG_INFO, "D3D12", "initializing overlay");
  queue_->GetDevice(IID_PPV_ARGS(&device_));

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
  swapchain_->GetDesc1(&swapchain_desc);
  try {
    textRenderer_.reset(new TextRenderer{static_cast<int>(swapchain_desc.Width),
                                         static_cast<int>(swapchain_desc.Height)});
  }
  catch (const std::exception&) {
    return;
  }
  bufferCount_ = swapchain_desc.BufferCount;

  if (!CreateFrameFences()) return;

  if (!CreateCMDList()) return;

  if (!CreateRenderTargets()) return;
  if (!CreateRootSignature()) return;
  if (!CreatePipelineStateObject()) return;

  if (!CreateOverlayTextures()) return;
  if (!CreateOverlayResources()) return;

  initSuccessfull_ = true;
  g_messageLog.Log(MessageLog::LOG_INFO, "D3D12", "overlay successfully initialized");
}

d3d12_renderer::~d3d12_renderer()
{
  for (auto handle : frameFenceEvents_)
  {
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
}

void d3d12_renderer::on_present()
{
  if (initSuccessfull_ && RecordingState::GetInstance().DisplayOverlay()) {
    const auto currentBackBuffer = swapchain_->GetCurrentBackBufferIndex();
    WaitForFence(frameFences_[currentBackBuffer].Get(),
      frameFenceValues_[currentBackBuffer], frameFenceEvents_[currentBackBuffer]);

    commandPool_->Reset();
    textRenderer_->DrawOverlay();

    UpdateOverlayTexture();
    DrawOverlay();
  }
}

bool d3d12_renderer::CreateCMDList()
{
  HRESULT hr;
  hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandPool_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "CreateCommandAllocator failed", hr);
    return false;
  }

  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandPool_.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "CreateCommandList failed", hr);
    return false;
  }
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
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreateRenderTargets - CreateDescriptorHeap failed", hr);
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
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "CreateRenderTargets - GetBuffer failed",
                       hr);
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
  for (int i = 0; i < bufferCount_; ++i)
  {
    HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences_[i]));
    if (FAILED(hr))
    {
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "Create Fence failed", hr);
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
  range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

  CD3DX12_ROOT_PARAMETER1 rootParam[2];
  rootParam[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);
  rootParam[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
                                        D3D12_SHADER_VISIBILITY_PIXEL);

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescOverlay;
  rootSignatureDescOverlay.Init_1_1(_countof(rootParam), rootParam, 0, nullptr,
                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> signatureOverlay;
  ComPtr<ID3DBlob> errorOverlay;

  HRESULT hr = D3DX12SerializeVersionedRootSignature(
      &rootSignatureDescOverlay, featureData.HighestVersion, &signatureOverlay, &errorOverlay);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreateRootSignature - D3DX12SerializeVersionedRootSignature failed", hr);
    return false;
  }

  hr = device_->CreateRootSignature(0, signatureOverlay->GetBufferPointer(),
                                    signatureOverlay->GetBufferSize(),
                                    IID_PPV_ARGS(&rootSignature_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreateRootSignature - CreateRootSignature failed", hr);
    return false;
  }
  return true;
}

bool d3d12_renderer::CreatePipelineStateObject()
{
  ComPtr<ID3D12Resource> buffer;
  HRESULT hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&buffer));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "CreatePipelineStateObject - GetBuffer failed",
                     hr);
    return false;
  }

  D3D12_BLEND_DESC blendDesc{};
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
  psoDesc.RTVFormats[0] = buffer->GetDesc().Format;
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

  hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreatePipelineStateObject - CreateGraphicsPipelineState failed", hr);
    return false;
  }
  return true;
}

bool d3d12_renderer::CreateOverlayTextures()
{
  commandList_->Reset(commandPool_.Get(), nullptr);
  HRESULT hr;

  const auto displayTextureWidth = textRenderer_->GetFullWidth();
  const auto displayTextureHeight = textRenderer_->GetFullHeight();

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
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
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
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
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
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                       "CreateOverlayTextures - CreateDescriptorHeap displayHeap_", hr);
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
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreateOverlayTextures - commandList Close failed", hr);
    return false;
  }
  ID3D12CommandList* ppCommandLists[] = {commandList_.Get()};
  queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  WaitForCompletion();

  return true;
}

bool d3d12_renderer::CreateOverlayResources()
{
  commandList_->Reset(commandPool_.Get(), nullptr);
  HRESULT hr;

  const auto screenPos = textRenderer_->GetScreenPos();
  std::pair<float, float> viewPortOffset = {static_cast<float>(screenPos.x),
                                            static_cast<float>(screenPos.y)};
  ComPtr<ID3D12Resource> cbUpload;
  // create constant buffer
  {
    const UINT bufferSize = sizeof(viewPortOffset);
    hr = device_->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&viewportOffsetCB_));
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                       "CreateOverlayResources - CreateCommittedResource viewportOffsetCB_ failed",
                       hr);
      return false;
    }

    hr = device_->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&cbUpload));
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                       "CreateOverlayResources - CreateCommittedResource cbUpload failed", hr);
      return false;
    }

    D3D12_SUBRESOURCE_DATA cbData = {};
    cbData.pData = reinterpret_cast<UINT8*>(&viewPortOffset);
    cbData.RowPitch = bufferSize;
    cbData.SlicePitch = cbData.RowPitch;

    UpdateSubresources(commandList_.Get(), viewportOffsetCB_.Get(), cbUpload.Get(), 0, 0, 1,
                       &cbData);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
        viewportOffsetCB_.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList_->ResourceBarrier(1, &transition);
  }

  // execute command list
  hr = commandList_->Close();
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                     "CreateOverlayResources - commandList Close failed", hr);
    return false;
  }
  ID3D12CommandList* ppCommandLists[] = {commandList_.Get()};
  queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  WaitForCompletion();
  // set the viewport and scissor rect

  const auto width = textRenderer_->GetFullWidth();
  const auto height = textRenderer_->GetFullHeight();

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

void d3d12_renderer::UpdateOverlayTexture()
{
  const auto textureData = textRenderer_->GetBitmapDataRead();

  if (textureData.dataPtr && textureData.size) {

    CD3DX12_RANGE readRange(0, 0);
    HRESULT hr = uploadBuffer_->Map(0, &readRange, &uploadDataPtr_);
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "CreateOverlayTextures - Map uploadTexture_",
        hr);
    }

    memcpy(uploadDataPtr_, textureData.dataPtr, textureData.size);

    uploadBuffer_->Unmap(0, &readRange);

    hr = commandList_->Reset(commandPool_.Get(), nullptr);
    assert(SUCCEEDED(hr));

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

    const auto& copyArea = textRenderer_->GetCopyArea();
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
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12",
                       "UpdateOverlayTexture - commandList Close failed", hr);
      assert(SUCCEEDED(hr));
      return;
    }

    // Execute draw commands
    ID3D12CommandList* const commandlists[] = {commandList_.Get()};
    queue_->ExecuteCommandLists(ARRAYSIZE(commandlists), commandlists);
  }
  textRenderer_->UnlockBitmapData();
}

void d3d12_renderer::DrawOverlay()
{
  HRESULT hr = commandList_->Reset(commandPool_.Get(), nullptr);
  assert(SUCCEEDED(hr));

  commandList_->SetPipelineState(pso_.Get());
  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  ID3D12DescriptorHeap* ppHeaps[] = {displayHeap_.Get()};
  commandList_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  commandList_->SetGraphicsRootDescriptorTable(0,
                                               displayHeap_->GetGPUDescriptorHandleForHeapStart());
  commandList_->SetGraphicsRootConstantBufferView(1, viewportOffsetCB_->GetGPUVirtualAddress());

  commandList_->RSSetScissorRects(1, &scissorRect_);
  commandList_->RSSetViewports(1, &viewPort_);

  const UINT buffer_index = swapchain_->GetCurrentBackBufferIndex();
  const auto transitionRender = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[buffer_index].Get(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
  commandList_->ResourceBarrier(1, &transitionRender);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetHeap_->GetCPUDescriptorHandleForHeapStart(),
                                          buffer_index, rtvHeapDescriptorSize_);
  commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList_->DrawInstanced(3, 1, 0, 0);

  const auto transitionPresent = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets_[buffer_index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT, 0);
  commandList_->ResourceBarrier(1, &transitionPresent);

  hr = commandList_->Close();
  assert(SUCCEEDED(hr));

  ID3D12CommandList* commandLists[] = {commandList_.Get()};
  queue_->ExecuteCommandLists(1, commandLists);

  queue_->Signal(frameFences_[buffer_index].Get(), currFenceValue_);
  frameFenceValues_[buffer_index] = currFenceValue_;
  ++currFenceValue_;
}

void d3d12_renderer::WaitForCompletion()
{
  HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "WaitForCompletion - CreateFence failed", hr);
    return;
  }
  fenceValue_ = 1;

  fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fenceEvent_ == nullptr) {
    hr = HRESULT_FROM_WIN32(GetLastError());
    if (FAILED(hr)) {
      g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "WaitForCompletion - CreateEvent failed",
                       hr);
      return;
    }
  }

  const auto currFenceValue = fenceValue_;
  hr = queue_->Signal(fence_.Get(), currFenceValue);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "D3D12", "WaitForCompletion - Signal queue failed", hr);
    return;
  }

  if (fence_->GetCompletedValue() < currFenceValue) {
    hr = fence_->SetEventOnCompletion(currFenceValue, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
  }
}
}
