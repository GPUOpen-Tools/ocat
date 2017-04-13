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
#include <vulkan\vk_layer.h>
#include <vulkan\vulkan.h>

#include "HashMap.h"
#include "TextRenderer.h"

class Rendering final {
 public:
  Rendering(const std::string& dir);

  Rendering(const Rendering&) = delete;
  Rendering& operator=(const Rendering&) = delete;

  void OnDestroySwapchain(VkDevice device, VkLayerDispatchTable* pTable, VkSwapchainKHR swapchain);

  void OnCreateSwapchain(VkDevice device, VkLayerDispatchTable* pTable,
                         const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
                         VkSwapchainKHR swapchain, VkFormat format, const VkExtent2D& extent);

  void OnGetSwapchainImages(VkLayerDispatchTable* pTable, VkSwapchainKHR swapchain,
                            uint32_t imageCount, VkImage* images);

  VkSemaphore OnPresent(VkLayerDispatchTable* pTable,
                        PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr, VkQueue queue,
                        uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
                        VkSwapchainKHR swapchain, uint32_t imageIndex, uint32_t waitSemaphoreCount,
                        const VkSemaphore* pWaitSemaphores);

 protected:
  struct SwapchainImageMapping {
    uint32_t imageIndex;
    VkCommandBuffer commandBuffer[2];
    VkSemaphore semaphore;
  };

  struct SwapchainQueueMapping {
    VkQueue queue;
    int32_t isGraphicsQueue;
    VkCommandPool commandPool;
    std::vector<SwapchainImageMapping> imageMappings;
  };

  struct SwapchainImageData {
    VkImage image;
    VkImageView view;
    VkFramebuffer framebuffer;
    VkDescriptorSet computeDescriptorSet[2];
  };

  struct OverlayImageData {
    VkBuffer overlayHostBuffer;
    VkDeviceMemory overlayHostMemory;
    VkBuffer overlayBuffer;
    VkBufferView bufferView;
    VkDeviceMemory overlayMemory;
    uint32_t commandBufferIndex;
    VkCommandBuffer commandBuffer[2];
    VkFence commandBufferFence[2];
    VkSemaphore overlayCopySemaphore;
    VkDescriptorSet descriptorSet;
    bool valid;

    bool CopyBuffer(VkDevice device, VkDeviceSize size, VkLayerDispatchTable* pTable,
                    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr, 
                    VkCommandPool commandPool, VkQueue queue);
  };

  struct SwapchainMapping {
    VkDevice device;
    VkFormat format;
    VkFormat overlayFormat;
    VkExtent2D extent;
    VkRect2D overlayRect;
    OverlayImageData overlayImages[2];
    uint32_t lastOverlayBufferSize;
    uint32_t nextOverlayImage;
    VkRenderPass renderPass;
    VkPipeline gfxPipeline;
    VkPipelineLayout gfxPipelineLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    VkDescriptorPool descriptorPool;
    std::vector<SwapchainImageData> imageData;
    std::vector<SwapchainQueueMapping> queueMappings;

    void ClearImageData(VkLayerDispatchTable* pTable);
  };

  void CreateImageMapping(VkLayerDispatchTable* pTable,
                          PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
                          VkSwapchainKHR swapchain, SwapchainMapping* sm, SwapchainQueueMapping* qm,
                          uint32_t queueFamilyIndex, SwapchainImageMapping* im);
  VkShaderModule CreateShaderModuleFromFile(VkDevice device, VkLayerDispatchTable* pTable,
                                            const std::string& fileName) const;
  VkShaderModule CreateShaderModuleFromBuffer(VkDevice device, VkLayerDispatchTable* pTable,
                                              const char* shaderBuffer, uint32_t bufferSize) const;

  HashMap<VkSwapchainKHR, SwapchainMapping*> swapchainMappings_;
  std::string shaderDirectory_;

  std::unique_ptr<TextRenderer> textRenderer_;
};