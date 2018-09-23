#include <wrl.h>

#include "vk_steamvr.h"
#include "d3d/steamvr.h"
#include "hook_manager.hpp"

#include "Logging/MessageLog.h"
#include "Utility/FileDirectory.h"


std::unique_ptr<CompositorOverlay::SteamVR_Vk> g_SteamVRVk;

namespace CompositorOverlay
{
void SteamVR_Vk::SetDevice(VkDevice device)
{
  device_ = device;
}

void SteamVR_Vk::Init(VkLayerDispatchTable* pTable,
  const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
  VkFormat format, VkImageUsageFlags usage)
{
  if (initialized_)
    return;

  renderer_.reset(new Rendering(g_fileDirectory.GetDirectory(DirectoryType::Bin)));
  pTable->GetSwapchainImagesKHR(device_, swapchain_, &imageCount_, nullptr);
  images_.resize(imageCount_);
  pTable->GetSwapchainImagesKHR(device_, swapchain_, &imageCount_, images_.data());

  VkExtent2D extent = { screenWidth_, screenHeight_ };

  initialized_ = renderer_->OnInitCompositor(device_, pTable, physicalDeviceMemoryProperties,
    format, extent, usage, imageCount_, images_.data());

  // init ivroverlay
  overlay_ = CreateVROverlay();
  vr::EVROverlayError error = overlay_->CreateOverlay("OCAT Overlay", "SteamVR", &overlayHandle_);

  if (error != vr::VROverlayError_None)
  {
    initialized_ = false;
    return;
  }

  vr::HmdMatrix34_t hmd;
  vr::TrackingUniverseOrigin trackingOrigin;
  overlay_->GetOverlayTransformAbsolute(overlayHandle_, &trackingOrigin, &hmd);

  hmd.m[0][3] = 0;
  hmd.m[1][3] = 0.1f;
  hmd.m[2][3] = -1;

  overlay_->SetOverlayTransformAbsolute(overlayHandle_, trackingOrigin, &hmd);
}

void SteamVR_Vk::Render(const vr::Texture_t *pTexture,
  VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
  VkQueue queue, uint32_t queueFamilyIndex, VkQueueFlags queueFlags) 
{
  renderer_->OnSubmitFrameCompositor(pTable, setDeviceLoaderDataFuncPtr, queue, queueFamilyIndex,
    queueFlags, currentIndex_);

  vr::Texture_t overlayTexture = *pTexture;
  vr::VRVulkanTextureData_t overlayTextureData = *static_cast<vr::VRVulkanTextureData_t*> (pTexture->handle);
  overlayTextureData.m_nWidth = screenWidth_;
  overlayTextureData.m_nHeight = screenHeight_;
  overlayTextureData.m_nImage = (uint64_t) images_[currentIndex_];
  overlayTextureData.m_nFormat = VK_FORMAT_B8G8R8A8_UNORM;
  overlayTexture.handle = reinterpret_cast<void*> (&overlayTextureData);

  overlay_->ClearOverlayTexture(overlayHandle_);
  overlay_->SetOverlayTexture(overlayHandle_, &overlayTexture);
  overlay_->ShowOverlay(overlayHandle_);
  currentIndex_ ^= 1;
}

void SteamVR_Vk::DestroyRenderer(VkDevice device, VkLayerDispatchTable* pTable)
{
  if (device == device_)
    renderer_->OnDestroyCompositor(pTable);
}
}
