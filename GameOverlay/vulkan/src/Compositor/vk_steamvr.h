#pragma once
#include <openvr.h>
#include <memory>

#include "../Rendering.h"

namespace CompositorOverlay
{
class SteamVR_Vk
{
public:
  void Init(VkLayerDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    VkFormat format, VkImageUsageFlags usage);
  void Render(const vr::Texture_t *pTexture,
    VkLayerDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkQueue queue, uint32_t queueFamilyIndex, VkQueueFlags queueFlags);

  void DestroyRenderer(VkDevice device, VkLayerDispatchTable* pTable);

  void SetDevice(VkDevice device);
  VkDevice GetDevice() { return device_; }
  VkSwapchainKHR* GetSwapchain() { return &swapchain_; }

  bool IsInitialized() { return initialized_; }

private:
  const uint32_t screenWidth_ = 256;
  const uint32_t screenHeight_ = 180;

  vr::IVROverlay* overlay_;
  vr::VROverlayHandle_t overlayHandle_;

  std::unique_ptr<Rendering> renderer_;

  VkDevice device_;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> images_;
  uint32_t imageCount_ = 0;
  uint32_t currentIndex_ = 0;

  bool initialized_;
};
}
extern std::unique_ptr<CompositorOverlay::SteamVR_Vk> g_SteamVRVk;
