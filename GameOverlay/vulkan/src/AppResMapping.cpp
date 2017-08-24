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

#include "AppResMapping.h"

void AppResMapping::CreateInstance(VkInstance instance, const VkInstanceCreateInfo* pCreateInfo) 
{
  // Empty
}

void AppResMapping::DestroyInstance(VkInstance instance)
{
  physicalDeviceMapping_.RemoveKeys_if(
      [&instance](const std::pair<VkPhysicalDevice, PhysicalDeviceMapping*>& v) {
        if (v.second->instance == instance) {
          delete v.second;
          return true;
        }
        return false;
      });
}

void AppResMapping::EnumeratePhysicalDevices(VkInstance instance,
                                             VkLayerInstanceDispatchTable* pTable,
                                             uint32_t* pPhysicalDeviceCount,
                                             VkPhysicalDevice* pPhysicalDevices)
{
  if (pPhysicalDevices && *pPhysicalDeviceCount > 0) {
    for (uint32_t i = 0; i < *pPhysicalDeviceCount; ++i) {
      VkPhysicalDeviceMemoryProperties memoryProperties;
      pTable->GetPhysicalDeviceMemoryProperties(pPhysicalDevices[i], &memoryProperties);
      physicalDeviceMapping_.Add(pPhysicalDevices[i],
                                 new PhysicalDeviceMapping{instance, memoryProperties});
    }
  }
}

void AppResMapping::CreateDevice(VkDevice device, VkPhysicalDevice physicalDevice,
                                 const VkDeviceCreateInfo* pCreateInfo)
{
  deviceMapping_.Add(device, new DeviceMapping{physicalDevice});
}

void AppResMapping::DestroyDevice(VkDevice device)
{
  delete deviceMapping_.Get(device);
  deviceMapping_.Remove(device);

  if (queueMapping_.GetSize() == 0) {
    return;
  }

  queueMapping_.RemoveKeys_if([&device](const std::pair<VkQueue, QueueMapping*>& v) {
    if (v.second->device == device) {
      delete v.second;
      return true;
    }
    return false;
  });
}

void AppResMapping::GetDeviceQueue(VkQueue queue, VkDevice device, uint32_t queueFamilyIndex,
                                   uint32_t queueIndex)
{
  queueMapping_.Add(queue, new QueueMapping{device, queueFamilyIndex});
}

AppResMapping::PhysicalDeviceMapping* AppResMapping::GetPhysicalDeviceMapping(
    VkPhysicalDevice physicalDevice) const
{
  return physicalDeviceMapping_.Get(physicalDevice);
}

AppResMapping::DeviceMapping* AppResMapping::GetDeviceMapping(VkDevice device) const
{
  return deviceMapping_.Get(device);
}

AppResMapping::QueueMapping* AppResMapping::GetQueueMapping(VkQueue queue) const
{
  return queueMapping_.Get(queue);
}
