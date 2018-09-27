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
#include <vk_dispatch_table_helper.h>
#include <vulkan/vulkan.h>

#include "HashMap.h"

class AppResMapping {
 public:
  AppResMapping() {}
  void CreateInstance(VkInstance instance, const VkInstanceCreateInfo* pCreateInfo);
  void DestroyInstance(VkInstance instance);

  void EnumeratePhysicalDevices(VkInstance instance, VkLayerInstanceDispatchTable* pTable,
                                uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);
  void CreateDevice(VkDevice device, VkPhysicalDevice physicalDevice,
                    const VkDeviceCreateInfo* pCreateInfo);
  void DestroyDevice(VkDevice device);

  void GetDeviceQueue(VkQueue queue, VkDevice device, uint32_t queueFamilyIndex,
                      uint32_t queueIndex);

  struct PhysicalDeviceMapping {
    VkInstance instance;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<VkQueueFamilyProperties> queueProperties;
  };
  PhysicalDeviceMapping* GetPhysicalDeviceMapping(VkPhysicalDevice physicalDevice) const;

  struct DeviceMapping {
    VkPhysicalDevice physicalDevice;
  };
  DeviceMapping* GetDeviceMapping(VkDevice device) const;

  struct QueueMapping {
    VkDevice device;
    uint32_t queueFamilyIndex;
  };
  QueueMapping* GetQueueMapping(VkQueue queue) const;

 protected:
  HashMap<VkPhysicalDevice, PhysicalDeviceMapping*> physicalDeviceMapping_;
  HashMap<VkDevice, DeviceMapping*> deviceMapping_;
  HashMap<VkQueue, QueueMapping*> queueMapping_;
};
