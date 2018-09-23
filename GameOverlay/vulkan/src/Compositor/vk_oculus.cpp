#include "vk_oculus.h"

#include "Utility/FileDirectory.h"
#include "hook_manager.hpp"

std::unique_ptr<CompositorOverlay::Oculus_Vk> g_OculusVk;

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
  if (g_OculusVk) {
    GameOverlay::find_hook_trampoline(&ovr_DestroyTextureSwapChain)(
      session, g_OculusVk->GetSwapChain());
  }

  GameOverlay::find_hook_trampoline(&ovr_DestroyTextureSwapChain)(session, chain);
}

// Vulkan specific

OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session)
{
  GameOverlay::find_hook_trampoline(&ovr_Destroy)(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainVk(ovrSession session, VkDevice device,
  const ovrTextureSwapChainDesc* desc, ovrTextureSwapChain* out_TextureSwapChain)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_CreateTextureSwapChainVk)(
    session, device, desc, out_TextureSwapChain);

  return result;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferVk(ovrSession session,
  ovrTextureSwapChain chain, int index, VkImage* out_Image)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainBufferVk)(
    session, chain, index, out_Image);

  return result;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetSynchonizationQueueVk(ovrSession session, VkQueue queue)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_SetSynchonizationQueueVk)(
    session, queue);

  if (OVR_SUCCESS(result)) {
    g_messageLog.LogError("Vulkan Oculus", "synchronization queue error");
  }

  return result;
}

namespace CompositorOverlay
{
void Oculus_Vk::SetDevice(VkDevice device)
{
  device_ = device;
}

void Oculus_Vk::SetQueue(VkQueue queue)
{
  queue_ = queue;
}

ovrRecti Oculus_Vk::GetViewport()
{
  VkRect2D rect2D = renderer_->GetViewportCompositor();
  ovrRecti viewport;

  viewport.Pos.x = rect2D.offset.x;
  viewport.Pos.y = rect2D.offset.y;
  viewport.Size.w = rect2D.extent.width;
  viewport.Size.h = rect2D.extent.height;

  return viewport;
}

bool Oculus_Vk::Init(VkLayerDispatchTable* pTable,
  const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties, ovrSession& session)
{
  if (initialized_)
    return true;

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

  GameOverlay::find_hook_trampoline(&ovr_CreateTextureSwapChainVk)(session, device_,
    &overlayDesc, &swapchain_);

  VkExtent2D extent = { screenWidth_, screenHeight_ };

  int textureCount = 0;
  GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainLength)(session, swapchain_,
    &textureCount);
  images_.resize(textureCount);
  for (int i = 0; i < textureCount; i++)
  {
    ovrResult result = GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainBufferVk)
      (session, swapchain_, i, &images_[i]);
  }

  renderer_.reset(new Rendering(g_fileDirectory.GetDirectory(DirectoryType::Bin)));
  initialized_ = renderer_->OnInitCompositor(device_, pTable, physicalDeviceMemoryProperties,
    VK_FORMAT_R8G8B8A8_UNORM, extent, 0, textureCount, images_.data());

  return initialized_;
}

bool Oculus_Vk::Render(VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
  uint32_t queueFamilyIndex, VkQueueFlags queueFlags, ovrSession& session)
{
  ovrResult result = GameOverlay::find_hook_trampoline(&ovr_SetSynchonizationQueueVk)(
    session, queue_);

  if (!OVR_SUCCESS(result)) {
    g_messageLog.LogInfo("Compositor Oculus Vk", "Failed to set synchronization queue");
    return false;
  }

  int imageIndex = 0;
  GameOverlay::find_hook_trampoline(&ovr_GetTextureSwapChainCurrentIndex)(
    session, swapchain_, &imageIndex);

  VkSemaphore semaphore = renderer_->OnSubmitFrameCompositor(pTable, setDeviceLoaderDataFuncPtr, queue_, 
    queueFamilyIndex, queueFlags, imageIndex);

  GameOverlay::find_hook_trampoline(&ovr_CommitTextureSwapChain)(session,
    swapchain_);

  return (semaphore != VK_NULL_HANDLE);
}

void Oculus_Vk::DestroyRenderer(VkDevice device, VkLayerDispatchTable* pTable)
{
  if (device == device_ && renderer_)
    renderer_->OnDestroyCompositor(pTable);
}
}
