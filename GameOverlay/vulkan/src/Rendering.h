//
// Copyright(c) 2016 Advanced Micro Devices, Inc. All rights reserved.
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

#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

#include "HashMap.h"
#include "Rendering/OverlayBitmap.h"

#include "OverlayImageData.h"
#include "SwapchainImageData.h"
#include "SwapchainImageMapping.h"
#include "SwapchainQueueMapping.h"
#include "SwapchainMapping.h"

class Rendering final {
public:
  Rendering(const std::wstring& shaderDirectory);
  Rendering(const Rendering&) = delete;
  Rendering& operator=(const Rendering&) = delete;

  void OnDestroySwapchain(VkDevice device, VkDevDispatchTable* pTable, VkSwapchainKHR swapchain);
  void OnCreateSwapchain(VkDevice device, VkDevDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    VkSwapchainKHR swapchain, VkFormat format, const VkExtent2D& extent,
    const VkImageUsageFlags usage);
  void OnGetSwapchainImages(VkDevDispatchTable* pTable, VkSwapchainKHR swapchain,
    uint32_t imageCount, VkImage* images);

  void OnDestroyCompositor(VkDevDispatchTable* pTable);
  bool OnInitCompositor(VkDevice device, VkDevDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    VkFormat format, const VkExtent2D& extent, VkImageUsageFlags usage,
    uint32_t imageCount, VkImage* images);

  VkSemaphore OnPresent(VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkQueue queue,
    uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
    VkSwapchainKHR swapchain, uint32_t imageIndex, uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores, bool lagIndicatorState = false);

  VkSemaphore OnSubmitFrameCompositor(VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkQueue queue,
    uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
    uint32_t imageIndex);

  VkRect2D GetViewportCompositor() { return compositorSwapchainMapping_.overlayRect; }
  bool Initialized() { return pipelineInitialized_; }

  int GetLagIndicatorHotkey() { return overlayBitmap_->GetLagIndicatorHotkey(); }
  bool HideOverlay() { return overlayBitmap_->HideOverlay(); }

protected:
  void DestroySwapchain(VkDevDispatchTable* pTable, SwapchainMapping* sm);

  bool InitRenderPass(VkDevDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    SwapchainMapping* sm);

  bool InitPipeline(VkDevDispatchTable* pTable, uint32_t imageCount,
    VkImage* images, SwapchainMapping* sm);

  VkSemaphore Present(VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkQueue queue,
    uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
    uint32_t imageIndex, uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores, SwapchainMapping* swapchainMapping,
    bool lagIndicatorState = false);

  VkResult RecordRenderPass(VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    SwapchainMapping* sm, SwapchainQueueMapping* qm,
    uint32_t queueFamilyIndex, SwapchainImageMapping* im);
  void CreateImageMapping(VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    SwapchainMapping* sm, SwapchainQueueMapping* qm,
    uint32_t queueFamilyIndex, SwapchainImageMapping* im);
  VkShaderModule CreateShaderModuleFromFile(VkDevice device, VkDevDispatchTable* pTable,
    const std::wstring& fileName) const;
  VkShaderModule CreateShaderModuleFromBuffer(VkDevice device, VkDevDispatchTable* pTable,
    const char* shaderBuffer, uint32_t bufferSize) const;
  VkResult UpdateUniformBuffer(VkDevDispatchTable* pTable, SwapchainMapping* sm);
  VkResult UpdateOverlayPosition(VkDevDispatchTable* pTable,
    SwapchainMapping* sm, const OverlayBitmap::Position& position);
  VkResult CreateOverlayImageBuffer(
      VkDevice device, VkDevDispatchTable* pTable,
    SwapchainMapping * sm, OverlayImageData & overlayImage, VkBuffer & uniformBuffer, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties);
  VkResult CreateUniformBuffer(VkDevice device, VkDevDispatchTable* pTable,
    SwapchainMapping * sm, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties);
  VkResult CreateFrameBuffer(VkDevDispatchTable* pTable, SwapchainMapping* sm,
                             SwapchainImageData& imageData, VkImage& image);

  HashMap<VkSwapchainKHR, SwapchainMapping*> swapchainMappings_;
  SwapchainMapping compositorSwapchainMapping_;
  std::wstring shaderDirectory_;
  std::unique_ptr<OverlayBitmap> overlayBitmap_;
  int remainingRecordRenderPassUpdates_ = 0;
  bool overlayBitmapInitialized_ = false;
  bool pipelineInitialized_ = false;

  bool lagIndicatorVisibility_ = true;
};
