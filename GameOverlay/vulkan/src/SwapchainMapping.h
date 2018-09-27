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

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

#include "OverlayImageData.h"
#include "SwapchainImageData.h"
#include "SwapchainQueueMapping.h"

struct SwapchainMapping {
  VkDevice device;
  VkFormat format;
  VkFormat overlayFormat;
  VkExtent2D extent;
  VkImageUsageFlags usage;
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
  VkBuffer uniformBuffer;
  VkDeviceMemory uniformMemory;
  std::vector<SwapchainImageData> imageData;
  std::unordered_map<uint32_t, SwapchainQueueMapping> queueMappings;

  void ClearImageData(VkLayerDispatchTable* pTable);
};
