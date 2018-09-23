#pragma once
#include <OVR_CAPI_D3D.h>

#include "d3d/d3d11_renderer.hpp"
#include "d3d/d3d12_renderer.hpp"

// just forward the API call

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainLength(ovrSession session,
	ovrTextureSwapChain chain, int* out_Length);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainCurrentIndex(ovrSession session,
	ovrTextureSwapChain chain, int* out_Index);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CommitTextureSwapChain(ovrSession session,
	ovrTextureSwapChain chain);

OVR_PUBLIC_FUNCTION(void) ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain);

// D3D specific functions

OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainDX(ovrSession session,
	IUnknown* d3dPtr, const ovrTextureSwapChainDesc* desc, ovrTextureSwapChain* out_TextureSwapChain);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferDX(ovrSession session,
	ovrTextureSwapChain chain, int index, IID iid, void** out_Buffer);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_EndFrame(ovrSession session, long long frameIndex,
	const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const* const* layerPtrList,
	unsigned int layerCount);


namespace CompositorOverlay {
class Oculus_D3D {
public:
	// called in dxgi.cpp - D3D11Device could be set during a hook of
	// hook_ovr_CreateTextureSwapChainDX, but for D3D12CommandQueue
	// it apparently seems safest to use the one which is in charge of the dxgi swapchain
	void SetDevice(IUnknown* device);
	
	bool Init(ovrSession session);
	bool Render(ovrSession session);

	ovrTextureSwapChain GetSwapChain() { return swapchain_; }
	ovrRecti GetViewport();

private:
	enum D3DVersion { D3DVersion_11 = 11, D3DVersion_12 = 12, D3DVersion_Undefined };

	const uint32_t screenWidth_ = 256;
	const uint32_t screenHeight_ = 180;

	bool initialized_ = false;

	ovrTextureSwapChain swapchain_;
	D3DVersion d3dVersion_ = D3DVersion_Undefined;

	std::unique_ptr<GameOverlay::d3d11_renderer> d3d11Renderer_;
	std::unique_ptr<GameOverlay::d3d12_renderer> d3d12Renderer_;

	// D3D11
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device_;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> d3d11RenderTargets_;

	//D3D12
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12Commandqueue_;
	Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> d3d12RenderTargetHeap_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> d3d12RenderTargets_;
	UINT d3d12RtvHeapDescriptorSize_;
	int bufferCount_ = 0;

	bool CreateD3D11RenderTargets(ovrSession session);
	bool CreateD3D12RenderTargets(ovrSession session);
};
}

extern std::unique_ptr<CompositorOverlay::Oculus_D3D> g_OculusD3D;
