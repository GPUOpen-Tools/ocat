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

#include "../Rendering.h"

namespace CompositorOverlay
{
class SteamVR_Vk
{
public:
  void Init(VkDevDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    VkFormat format, VkImageUsageFlags usage);
 void Render(const vr::Texture_t* pTexture, VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkQueue queue, uint32_t queueFamilyIndex, VkQueueFlags queueFlags);

  void DestroyRenderer(VkDevice device, VkDevDispatchTable* pTable);

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
