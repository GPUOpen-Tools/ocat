#pragma once
#include <OVR_CAPI_Vk.h>

#include "../Rendering.h"

// just forward the API call

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainLength(ovrSession session,
  ovrTextureSwapChain chain, int* out_Length);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainCurrentIndex(ovrSession session,
  ovrTextureSwapChain chain, int* out_Index);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CommitTextureSwapChain(ovrSession session,
  ovrTextureSwapChain chain);

OVR_PUBLIC_FUNCTION(void) ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain);

// Vulkan specific

OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainVk(ovrSession session, VkDevice device,
  const ovrTextureSwapChainDesc* desc, ovrTextureSwapChain* out_TextureSwapChain);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferVk(ovrSession session,
  ovrTextureSwapChain chain, int index, VkImage* out_Image);

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetSynchonizationQueueVk(ovrSession session, VkQueue queue);

// needs to be called in VK_LAYER_OCAT_overlay.cpp to have access to pTable etc.
OVR_PUBLIC_FUNCTION(ovrResult) ovr_EndFrame(ovrSession session, long long frameIndex,
  const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const* const* layerPtrList,
  unsigned int layerCount);

namespace CompositorOverlay {
class Oculus_Vk {
public:
  void SetDevice(VkDevice device);
  void SetQueue(VkQueue queue);
  VkQueue GetQueue() { return queue_; }
  VkDevice GetDevice() { return device_; }
  ovrTextureSwapChain GetSwapChain() { return swapchain_; }

  bool Init(VkLayerDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    ovrSession& session);
  bool Render(VkLayerDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    uint32_t queueFamilyIndex, VkQueueFlags queueFlags, ovrSession& session);

  void DestroyRenderer(VkDevice device, VkLayerDispatchTable* pTable);

  ovrRecti GetViewport();

private:
  const uint32_t screenWidth_ = 256;
  const uint32_t screenHeight_ = 180;

  bool initialized_ = false;

  ovrTextureSwapChain swapchain_;

  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::vector<VkImage> images_;

  std::unique_ptr<Rendering> renderer_;
};
}

extern std::unique_ptr<CompositorOverlay::Oculus_Vk> g_OculusVk;
