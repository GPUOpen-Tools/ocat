//
// Copyright(c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
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

#pragma once
#include <openvr.h>

#include <memory>

#include "d3d/d3d11_renderer.hpp"
#include "d3d/d3d12_renderer.hpp"

void* VR_GetGenericInterface(const char* pchInterfaceVersion, vr::EVRInitError *peError);

vr::EVRCompositorError IVRCompositor_Submit(vr::IVRCompositor* pCompositor, 
  vr::EVREye eEye, const vr::Texture_t *pTexture,
  const vr::VRTextureBounds_t* pBounds = 0, 
  vr::EVRSubmitFlags nSubmitFlags = vr::Submit_Default);

__declspec(dllexport) vr::IVRCompositor* GetVRCompositor();
__declspec(dllexport) vr::IVROverlay* CreateVROverlay();

namespace CompositorOverlay {
  class SteamVR_D3D
  {
  public:
    ~SteamVR_D3D();

    bool Init(const vr::ETextureType eType);
    void Render(const vr::Texture_t *pTexture);
    
    void SetDevice(IUnknown* device);
  private:
    const uint32_t screenWidth_ = 256;
    const uint32_t screenHeight_ = 180;

    vr::IVROverlay* overlay_;
    vr::VROverlayHandle_t overlayHandle_;

    std::unique_ptr<GameOverlay::d3d11_renderer> d3d11Renderer_;
    std::unique_ptr<GameOverlay::d3d12_renderer> d3d12Renderer_;

    // D3D11
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11OverlayTexture_;
    std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> d3d11RenderTargets_;

    // D3D12
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12Commandqueue_;
    Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> d3d12RenderTargetHeap_;
    UINT d3d12RtvHeapDescriptorSize_;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> d3d12RenderTargets_;

    bool initialized_ = false;
  };
}

extern vr::IVRCompositor* g_Compositor;
extern std::unique_ptr<CompositorOverlay::SteamVR_D3D> g_SteamVRD3D;
