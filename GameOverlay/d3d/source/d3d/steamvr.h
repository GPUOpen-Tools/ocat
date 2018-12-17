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
