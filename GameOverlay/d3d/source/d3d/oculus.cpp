#include "oculus.h"
#include "hook_manager.hpp"
#include "../OverlayPS_Byte.h"
#include "../OverlayVS_Byte.h"
#include "../deps/d3dx12/d3dx12.h"

using Microsoft::WRL::ComPtr;

std::unique_ptr<CompositorOverlay::Oculus_D3D> g_OculusD3D;

// just forward call to the hooked function

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainLength(ovrSession session,
  ovrTextureSwapChain chain, int* out_Length)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainLength)(
    session, chain, out_Length);

  return result;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainCurrentIndex(ovrSession session,
  ovrTextureSwapChain chain, int* out_Index)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainCurrentIndex)(
    session, chain, out_Index);

  if (OVR_FAILURE(result)) {
    g_messageLog.LogError("Oculus", "Failed to get swap chain current index");
  }

  return result;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CommitTextureSwapChain(ovrSession session,
  ovrTextureSwapChain chain)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_CommitTextureSwapChain)(
    session, chain);

  if (!OVR_SUCCESS(result)) {
    g_messageLog.LogError("OVRError", "Failed to commit swap chain");
  }

  return result;
}

OVR_PUBLIC_FUNCTION(void) ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain)
{
  GameOverlay::find_hook_trampoline(&ovr_DestroyTextureSwapChain)(session, chain);
}

// D3D specific functions

OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session)
{
  if (g_OculusD3D) {
    GameOverlay::find_hook_trampoline(&ovr_DestroyTextureSwapChain)(
      session, g_OculusD3D->GetSwapChain());
  }

  GameOverlay::find_hook_trampoline(&ovr_Destroy)(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainDX(ovrSession session,
  IUnknown* d3dPtr, const ovrTextureSwapChainDesc* desc, ovrTextureSwapChain* out_TextureSwapChain) {

  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_CreateTextureSwapChainDX)(
    session, d3dPtr, desc, out_TextureSwapChain);

  return result;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferDX(ovrSession session,
  ovrTextureSwapChain chain, int index, IID iid, void** out_Buffer)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainBufferDX)(
    session, chain, index, iid, out_Buffer);

  return result;
}

// use an additional quad layer on top of the layer list which was submitted to the
// compositor to render the overlay into the Oculus

OVR_PUBLIC_FUNCTION(ovrResult) ovr_EndFrame(ovrSession session, long long frameIndex,
  const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const* const* layerPtrList,
  unsigned int layerCount) 
{
  if (g_OculusD3D->Init(session)) {

    // if something failed during rendering, just submit frame without overlay
    if (!g_OculusD3D->Render(session)) {
      return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
        session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
    }

    ovrLayerQuad overlayLayer;
    overlayLayer.Header.Type = ovrLayerType_Quad;
    overlayLayer.Header.Flags = ovrLayerFlag_HighQuality | ovrLayerFlag_HeadLocked;
    overlayLayer.ColorTexture = g_OculusD3D->GetSwapChain();

    // we use a fixed overlay position within the HMD;
    overlayLayer.QuadPoseCenter.Position.y =  0.00f;
    overlayLayer.QuadPoseCenter.Position.x =  0.00f;
    overlayLayer.QuadPoseCenter.Position.z = -1.00f;
    overlayLayer.QuadPoseCenter.Orientation.x = 0;
    overlayLayer.QuadPoseCenter.Orientation.y = 0;
    overlayLayer.QuadPoseCenter.Orientation.z = 0;
    overlayLayer.QuadPoseCenter.Orientation.w = 1;
    // HUD is 50cm wide, 30cm tall.
    overlayLayer.QuadSize.x = 0.50f;
    overlayLayer.QuadSize.y = 0.30f;
    // Display all of the HUD texture.
    overlayLayer.Viewport = g_OculusD3D->GetViewport();

    // append overlayLayer at the end of the layer list
    std::vector<const ovrLayerHeader*> list;
    list.resize(layerCount + 1);
    for (uint32_t i = 0; i < layerCount; i++) {

      list[i] = *(layerPtrList + i);
    }
    list[layerCount] = &overlayLayer.Header;

    return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
      session, frameIndex, viewScaleDesc, list.data(), layerCount + 1);
  }

  // no overlay, just pass the frame to the compositor without modification
  return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
    session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
}

namespace CompositorOverlay
{
void Oculus_D3D::SetDevice(IUnknown* device)
{
  ID3D11Device *d3d11Device = nullptr;
  HRESULT hr = device->QueryInterface(&d3d11Device);
  if (SUCCEEDED(hr)) {
    d3d11Device_ = d3d11Device;
    d3dVersion_ = D3DVersion_11;
  }

  ID3D12CommandQueue *d3d12CommandQueue = nullptr;
  hr = device->QueryInterface(&d3d12CommandQueue);
  if (SUCCEEDED(hr)) {
    d3d12Commandqueue_ = d3d12CommandQueue;
    d3d12Commandqueue_->GetDevice(IID_PPV_ARGS(&d3d12Device_));
    d3dVersion_ = D3DVersion_12;
  }
}

bool Oculus_D3D::Init(ovrSession session)
{
  if (initialized_) {
    return true;
  }

  g_messageLog.LogInfo("Compositor Oculus", "Initialize Oculus overlay");

  // create ovr swapchain for the overlay
  ovrTextureSwapChainDesc overlayDesc;
  overlayDesc.Type = ovrTexture_2D;
  overlayDesc.ArraySize = 1;
  overlayDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
  overlayDesc.Width = screenWidth_;
  overlayDesc.Height = screenHeight_;
  overlayDesc.MipLevels = 1;
  overlayDesc.SampleCount = 1;
  overlayDesc.MiscFlags = ovrTextureMisc_DX_Typeless | ovrTextureMisc_AutoGenerateMips;
  overlayDesc.BindFlags = ovrTextureBind_DX_RenderTarget;
  overlayDesc.StaticImage = ovrFalse;

  switch (d3dVersion_)
  {
  case D3DVersion_11:
  {
    GameOverlay::find_hook_trampoline(&ovr_CreateTextureSwapChainDX)(
      session, d3d11Device_.Get(), &overlayDesc, &swapchain_);

    if (!CreateD3D11RenderTargets(session)) {
      return false;
    }

    d3d11Renderer_ = std::make_unique<GameOverlay::d3d11_renderer>(d3d11Device_.Get(),
      d3d11RenderTargets_, screenWidth_, screenHeight_);

    initialized_ = true;
    return true;
  }
  case D3DVersion_12:
  {
    HRESULT hr = GameOverlay::find_hook_trampoline(&ovr_CreateTextureSwapChainDX)(
      session, d3d12Commandqueue_.Get(), &overlayDesc, &swapchain_);

    hr = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainLength)(
      session, swapchain_, &bufferCount_);

    // render targets
    if (!CreateD3D12RenderTargets(session)) {
      return false;
    }

    d3d12Renderer_ = std::make_unique<GameOverlay::d3d12_renderer>(d3d12Commandqueue_.Get(),
      d3d12RenderTargetHeap_, d3d12RenderTargets_, d3d12RtvHeapDescriptorSize_, bufferCount_,
      screenWidth_, screenHeight_);

    initialized_ = true;
    return true;
  }
  }

  return false;
}

bool Oculus_D3D::Render(ovrSession session)
{
  bool result = false;
  int currentIndex = 0;
  GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainCurrentIndex)(session, swapchain_, &currentIndex);

  switch (d3dVersion_)
  {
  case D3DVersion_11:
  {
    result = d3d11Renderer_->on_present(currentIndex);
    break;
  }
  case D3DVersion_12:
  {
    result = d3d12Renderer_->on_present(currentIndex);
    break;
  }
  }

  GameOverlay::find_hook_trampoline(&ovr_CommitTextureSwapChain)(session, swapchain_);

  return result;
}

ovrRecti Oculus_D3D::GetViewport()
{
  ovrRecti ovrViewport;
  switch (d3dVersion_)
  {
  case D3DVersion_11:
  {
    D3D11_VIEWPORT viewPort = d3d11Renderer_->GetViewport();
    ovrViewport.Pos.x = static_cast<UINT> (viewPort.TopLeftX);
    ovrViewport.Pos.y = static_cast<UINT> (viewPort.TopLeftY);
    ovrViewport.Size.w = static_cast<UINT> (viewPort.Width);
    ovrViewport.Size.h = static_cast<UINT> (viewPort.Height);
    break;
  }
  case D3DVersion_12:
  {
    D3D12_VIEWPORT viewPort = d3d12Renderer_->GetViewport();
    ovrViewport.Pos.x = static_cast<UINT> (viewPort.TopLeftX);
    ovrViewport.Pos.y = static_cast<UINT> (viewPort.TopLeftY);
    ovrViewport.Size.w = static_cast<UINT> (viewPort.Width);
    ovrViewport.Size.h = static_cast<UINT> (viewPort.Height);
    break;
  }
  }
  return ovrViewport;
}

bool Oculus_D3D::CreateD3D11RenderTargets(ovrSession session)
{
  int count = 0;

  GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainLength)(session,
    swapchain_, &count);

  // create render targets
  d3d11RenderTargets_.resize(count);
  for (int i = 0; i < count; ++i) {
    ComPtr<ID3D11Texture2D> texture = nullptr;
    HRESULT hr = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainBufferDX)(
      session, swapchain_, i, IID_PPV_ARGS(&texture));

    D3D11_TEXTURE2D_DESC textureDesc;
    texture->GetDesc(&textureDesc);

    D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
    rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvd.ViewDimension = textureDesc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
      : D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvd.Texture2D.MipSlice = 0;

    hr = d3d11Device_->CreateRenderTargetView(texture.Get(), &rtvd, &d3d11RenderTargets_[i]);

    if (FAILED(hr))
    {
      g_messageLog.LogInfo("OCULUS", "Fail create render target view.");
      return false;
    }

    g_messageLog.LogInfo("Compositor Oculus", "Create render target");
  }

  return true;
}

bool Oculus_D3D::CreateD3D12RenderTargets(ovrSession session)
{
  int bufferCount;
  HRESULT hr = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainLength)(
    session, swapchain_, &bufferCount);

  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = bufferCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = d3d12Device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&d3d12RenderTargetHeap_));
  if (FAILED(hr))
  {
    g_messageLog.LogError("D3D12",
      "CreateRenderTargets - CreateDescriptorHeap failed", hr);
    return false;
  }

  d3d12RtvHeapDescriptorSize_ =
    d3d12Device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(d3d12RenderTargetHeap_->GetCPUDescriptorHandleForHeapStart());
  d3d12RenderTargets_.resize(bufferCount);

  // Create a RTV for each frame.
  for (UINT buffer_index = 0; buffer_index < d3d12RenderTargets_.size(); buffer_index++)
  {
    hr = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainBufferDX)(
      session, swapchain_, buffer_index, IID_PPV_ARGS(&d3d12RenderTargets_[buffer_index]));

    D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
    rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    d3d12Device_->CreateRenderTargetView(d3d12RenderTargets_[buffer_index].Get(), &rtvd, rtvHandle);
    rtvHandle.Offset(1, d3d12RtvHeapDescriptorSize_);
  }
  return true;
}
}
