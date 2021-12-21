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
#include <assert.h>
#include "vk_dispatch_defs.h"
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

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
  VkDescriptorSet lagIndicatorDescriptorSet;
  bool valid;

  bool CopyBuffer(VkDevice device, VkDeviceSize size, VkDevDispatchTable* pTable,
    PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
    VkCommandPool commandPool, VkQueue queue);
};
