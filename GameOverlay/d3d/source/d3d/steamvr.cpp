#include <wrl.h>
#include "../deps/d3dx12/d3dx12.h"

#include "steamvr.h"
#include "hook_manager.hpp"
#include "Logging/MessageLog.h"

vr::IVRCompositor* g_Compositor;
std::unique_ptr<CompositorOverlay::SteamVR_D3D> g_SteamVRD3D;

void hook_openvr_compositor_object(vr::IVRCompositor* pCompositor)
{
	g_Compositor = pCompositor;

	if (GameOverlay::install_hook(VTABLE(g_Compositor), 5,
		reinterpret_cast<GameOverlay::hook::address>(&IVRCompositor_Submit))) {
		g_messageLog.LogInfo("SteamVR", "Successfully installed hook for compositor submit.");
	}
}

void* VR_GetGenericInterface(const char* pchInterfaceVersion, vr::EVRInitError *peError) {
	void* voidPtr = GameOverlay::find_hook_trampoline(&VR_GetGenericInterface)(pchInterfaceVersion, peError);

	const char* comp = "IVRCompositor";
	if (std::strncmp(comp, pchInterfaceVersion, 13) == 0) {
		g_messageLog.LogInfo("SteamVR", "generic interface - Compositor");
		hook_openvr_compositor_object(static_cast<vr::IVRCompositor *> (voidPtr));
	}

	return voidPtr;
}

vr::EVRCompositorError VR_CALLTYPE IVRCompositor_Submit(vr::IVRCompositor* pCompositor,
	vr::EVREye eEye, const vr::Texture_t *pTexture,
	const vr::VRTextureBounds_t* pBounds, vr::EVRSubmitFlags nSubmitFlags)
{
	// only update overlay once a frame and not twice, so just consider one left eye submit call
	if (eEye == vr::Eye_Left && g_SteamVRD3D->Init(pTexture->eType))
	{
		g_SteamVRD3D->Render(pTexture);
	}

	return GameOverlay::find_hook_trampoline(&IVRCompositor_Submit)(
		pCompositor, eEye, pTexture, pBounds, nSubmitFlags);
}

__declspec(dllexport) vr::IVRCompositor* GetVRCompositor() {
	return g_Compositor;
}

__declspec(dllexport) vr::IVROverlay* CreateVROverlay() {
	const char *pchInterfaceVersion = "IVROverlay_017";
	vr::EVRInitError peError;
	void* voidPtr = GameOverlay::find_hook_trampoline(&VR_GetGenericInterface)(pchInterfaceVersion, &peError);

	return static_cast<vr::IVROverlay *> (voidPtr);
}

namespace CompositorOverlay
{
void SteamVR_D3D::SetDevice(IUnknown* device)
{
	ID3D11Device *d3d11Device = nullptr;
	HRESULT hr = device->QueryInterface(&d3d11Device);
	if (SUCCEEDED(hr)) {
		d3d11Device_ = d3d11Device;
	}

	ID3D12CommandQueue *d3d12CommandQueue = nullptr;
	hr = device->QueryInterface(&d3d12CommandQueue);
	if (SUCCEEDED(hr)) {
		d3d12Commandqueue_ = d3d12CommandQueue;
		d3d12Commandqueue_->GetDevice(IID_PPV_ARGS(&d3d12Device_));
	}
}

bool SteamVR_D3D::Init(const vr::ETextureType eType) {
	if (initialized_)
		return true;

	overlay_ = CreateVROverlay();
	overlay_->CreateOverlay("test", "overlay", &overlayHandle_);

	vr::HmdMatrix34_t hmd;
	vr::TrackingUniverseOrigin trackingOrigin;
	overlay_->GetOverlayTransformAbsolute(overlayHandle_, &trackingOrigin, &hmd);

	hmd.m[0][3] = 0;
	hmd.m[1][3] = 1.0f;
	hmd.m[2][3] = -1;

	overlay_->SetOverlayTransformAbsolute(overlayHandle_, trackingOrigin, &hmd);

	g_messageLog.LogInfo("SteamVR", "overlay ivr.");

	switch (eType)
	{
	case vr::TextureType_DirectX:
	{
		D3D11_TEXTURE2D_DESC displayDesc{};
		displayDesc.Width = screenWidth_;
		displayDesc.Height = screenHeight_;
		displayDesc.MipLevels = 1;
		displayDesc.ArraySize = 1;
		displayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		displayDesc.SampleDesc.Count = 1;
		displayDesc.SampleDesc.Quality = 0;
		displayDesc.Usage = D3D11_USAGE_DEFAULT;
		displayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		displayDesc.CPUAccessFlags = 0;
		HRESULT hr = d3d11Device_->CreateTexture2D(&displayDesc, nullptr, &d3d11OverlayTexture_);

		d3d11RenderTargets_.resize(1);
		D3D11_TEXTURE2D_DESC backBufferDesc;
		d3d11OverlayTexture_->GetDesc(&backBufferDesc);

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = backBufferDesc.Format;
		rtvDesc.ViewDimension = backBufferDesc.SampleDesc.Count > 0 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
			: D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		hr = d3d11Device_->CreateRenderTargetView(d3d11OverlayTexture_.Get(), &rtvDesc,
			&d3d11RenderTargets_[0]);

		if (FAILED(hr))
		{
			return false;
		}

		d3d11Renderer_.reset(new GameOverlay::d3d11_renderer(d3d11Device_.Get(),
			d3d11RenderTargets_, screenWidth_, screenHeight_));

		initialized_ = true;
		return true;
	}
	case vr::TextureType_DirectX12:
	{
		d3d12RenderTargets_.resize(1);

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.Width = screenWidth_;
		textureDesc.Height = screenHeight_;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		const auto textureHeapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT hr = d3d12Device_->CreateCommittedResource(
			&textureHeapType, D3D12_HEAP_FLAG_NONE, &textureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr,
			IID_PPV_ARGS(&d3d12RenderTargets_[0]));
		if (FAILED(hr))
		{
			return false;
		}

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hr = d3d12Device_->CreateDescriptorHeap(&rtvHeapDesc,
			IID_PPV_ARGS(&d3d12RenderTargetHeap_));
		if (FAILED(hr))
		{
			return false;
		}

		d3d12RtvHeapDescriptorSize_ =
			d3d12Device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(d3d12RenderTargetHeap_->GetCPUDescriptorHandleForHeapStart());

		d3d12Device_->CreateRenderTargetView(d3d12RenderTargets_[0].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, d3d12RtvHeapDescriptorSize_);

		d3d12Renderer_.reset(
			new GameOverlay::d3d12_renderer(d3d12Commandqueue_.Get(),
				d3d12RenderTargetHeap_,
				d3d12RenderTargets_,
				d3d12RtvHeapDescriptorSize_,
				1, screenWidth_, screenHeight_));

		initialized_ = true;
		return true;
	}
	}

	return false;
}

void SteamVR_D3D::Render(const vr::Texture_t *pTexture)
{
	switch (pTexture->eType)
	{
	case vr::TextureType_DirectX:
	{
		d3d11Renderer_->on_present();
		vr::Texture_t texture = *pTexture;
		ID3D11Texture2D* tex = static_cast<ID3D11Texture2D*> (d3d11OverlayTexture_.Get());
		texture.handle = static_cast<void*> (tex);

		overlay_->ClearOverlayTexture(overlayHandle_);
		overlay_->SetOverlayTexture(overlayHandle_,&texture);
		overlay_->ShowOverlay(overlayHandle_);
		return;
	}
	case vr::TextureType_DirectX12:
	{
		d3d12Renderer_->on_present(0);
		
		void* handle = pTexture->handle;
		vr::D3D12TextureData_t* d3d12TextureData = static_cast<vr::D3D12TextureData_t*>(handle);
		
		vr::Texture_t texture = *pTexture;
		
		vr::D3D12TextureData_t texData;
		texData.m_nNodeMask = d3d12TextureData->m_nNodeMask;
		texData.m_pCommandQueue = d3d12TextureData->m_pCommandQueue;
		texData.m_pResource = d3d12RenderTargets_[0].Get();
		
		texture.handle = static_cast<void*> (&texData);

		overlay_->ClearOverlayTexture(overlayHandle_);
		overlay_->SetOverlayTexture(overlayHandle_, &texture);
		overlay_->ShowOverlay(overlayHandle_);
		return;
	}
	}

	return;
}
}
