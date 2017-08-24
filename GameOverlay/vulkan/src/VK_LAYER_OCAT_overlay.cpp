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

#define VK_USE_PLATFORM_WIN32_KHR
#include <vk_layer_table.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
#include "vk_layer_extension_utils.h"

#include "AppResMapping.h"
#include "Recording\Capturing.h"
#include "Utility\FileDirectory.h"
#include "HashMap.h"
#include "Rendering.h"
#include "Utility\ProcessHelper.h"

#include <vk_layer_extension_utils.cpp>
#include <vk_layer_table.cpp>

static const VkLayerProperties global_layer = {
    "VK_LAYER_OCAT_overlay", VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION), 1, "Layer: overlay"};

AppResMapping g_AppResources;
std::unique_ptr<Rendering> g_Rendering;

struct VulkanFunction {
  const char* name;
  PFN_vkVoidFunction ptr;
};

VulkanFunction hookedInstanceFunctions[] = {
    {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr)},
    {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(vkCreateInstance)},
    {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyInstance)},
    {"vkEnumeratePhysicalDevices",
     reinterpret_cast<PFN_vkVoidFunction>(vkEnumeratePhysicalDevices)},
    {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(vkCreateDevice)},
    {"vkEnumerateInstanceLayerProperties",
     reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceLayerProperties)},
    {"vkEnumerateDeviceLayerProperties",
     reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateDeviceLayerProperties)},
    {"vkEnumerateInstanceExtensionProperties",
     reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceExtensionProperties)},
    {"vkEnumerateDeviceExtensionProperties",
     reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateDeviceExtensionProperties)},
    {"vkGetPhysicalDeviceQueueFamilyProperties",
     reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceQueueFamilyProperties)}};

VulkanFunction hookedDeviceFunctions[] = {
    {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDevice)},
    {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceQueue)},
};

VulkanFunction hookedKHRExtFunctions[] = {
    {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(vkCreateSwapchainKHR)},
    {"vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(vkDestroySwapchainKHR)},
    {"vkGetSwapchainImagesKHR", reinterpret_cast<PFN_vkVoidFunction>(vkGetSwapchainImagesKHR)},
    {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(vkQueuePresentKHR)}};

void registerDeviceKHRExtFunctions(const VkDeviceCreateInfo* pCreateInfo,
                                   VkLayerDispatchTable* pTable, VkDevice device)
{
  PFN_vkGetDeviceProcAddr gpa = pTable->GetDeviceProcAddr;
  pTable->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)gpa(device, "vkCreateSwapchainKHR");
  pTable->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)gpa(device, "vkDestroySwapchainKHR");
  pTable->GetSwapchainImagesKHR =
      (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
  pTable->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");
}

static HashMap<VkInstance, VkLayerInstanceDispatchTable*> instanceDispatchTable;
static HashMap<VkDevice, VkLayerDispatchTable*> deviceDispatchTable;
static HashMap<VkDevice, PFN_vkSetDeviceLoaderData> deviceLoaderDataFunc;

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
  if (nReason == DLL_PROCESS_DETACH) 
  {
    g_Rendering.reset();
  }
  else if (nReason == DLL_PROCESS_ATTACH) 
  {
    if (!g_fileDirectory.Initialize())
    {
      return FALSE;
    }
  }
  return TRUE;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkInstance* pInstance)
{
  VkLayerInstanceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
  assert(chain_info->u.pLayerInfo);

  PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
      chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  assert(fpGetInstanceProcAddr);

  PFN_vkCreateInstance fpCreateInstance =
      (PFN_vkCreateInstance)fpGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
  if (fpCreateInstance == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Advance the link info for the next element on the chain
  chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

  VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
  if (result != VK_SUCCESS) {
    return result;
  }

  auto instance_dispatch_table = new VkLayerInstanceDispatchTable;
  layer_init_instance_dispatch_table(*pInstance, instance_dispatch_table, fpGetInstanceProcAddr);
  instanceDispatchTable.Add(*pInstance, instance_dispatch_table);

  g_AppResources.CreateInstance(*pInstance, pCreateInfo);

  g_Rendering.reset(new Rendering(g_fileDirectory.GetDirectory(FileDirectory::DIR_BIN)));
  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable.Get(instance);
  if (pTable->DestroyInstance == NULL) {
    return;
  }

  g_AppResources.DestroyInstance(instance);

  pTable->DestroyInstance(instance, pAllocator);

  delete pTable;
  instanceDispatchTable.Remove(instance);
  g_Rendering.reset();
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable.Get(instance);
  if (pTable->EnumeratePhysicalDevices == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  VkResult result =
      pTable->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
  if (result != VK_SUCCESS) {
    return result;
  }

  g_AppResources.EnumeratePhysicalDevices(instance, pTable, pPhysicalDeviceCount, pPhysicalDevices);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
               const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
  VkLayerDeviceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
  assert(chain_info->u.pLayerInfo);

  PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
      chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;

  VkInstance instance = g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance;

  PFN_vkCreateDevice fpCreateDevice =
      (PFN_vkCreateDevice)fpGetInstanceProcAddr(instance, "vkCreateDevice");
  if (fpCreateDevice == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Advance the link info for the next element on the chain
  chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

  VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
  if (result != VK_SUCCESS) {
    return result;
  }

  chain_info = get_chain_info(pCreateInfo, VK_LOADER_DATA_CALLBACK);
  if (chain_info) {
    deviceLoaderDataFunc.Add(*pDevice, chain_info->u.pfnSetDeviceLoaderData);
  }

  auto device_dispatch_table = new VkLayerDispatchTable;
  layer_init_device_dispatch_table(*pDevice, device_dispatch_table, fpGetDeviceProcAddr);
  registerDeviceKHRExtFunctions(pCreateInfo, device_dispatch_table, *pDevice);
  deviceDispatchTable.Add(*pDevice, device_dispatch_table);

  g_AppResources.CreateDevice(*pDevice, physicalDevice, pCreateInfo);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device,
                                                           const VkAllocationCallbacks* pAllocator)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->DestroyDevice == NULL) {
    return;
  }

  g_AppResources.DestroyDevice(device);

  pTable->DestroyDevice(device, pAllocator);

  delete pTable;
  deviceDispatchTable.Remove(device);
  deviceLoaderDataFunc.Remove(device);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
  VkLayerInstanceDispatchTable* pTable =
      instanceDispatchTable.Get(g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance);
  if (pTable->GetPhysicalDeviceQueueFamilyProperties == NULL) {
    return;
  }

  pTable->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount,
                                                 pQueueFamilyProperties);
  if (pQueueFamilyProperties == nullptr || *pQueueFamilyPropertyCount == 0) {
    return;
  }

  g_AppResources.GetPhysicalDeviceMapping(physicalDevice)
      ->queueProperties.assign(pQueueFamilyProperties,
                               pQueueFamilyProperties + *pQueueFamilyPropertyCount);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                                            uint32_t queueFamilyIndex,
                                                            uint32_t queueIndex, VkQueue* pQueue)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->GetDeviceQueue == NULL) {
    return;
  }

  pTable->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

  g_AppResources.GetDeviceQueue(*pQueue, device, queueFamilyIndex, queueIndex);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
                     const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->CreateSwapchainKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Add VK_IMAGE_USAGE_STORAGE_BIT for compute present
  VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
  createInfo.imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

  VkResult result = pTable->CreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkPhysicalDevice physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;

  g_Rendering->OnCreateSwapchain(
      device, pTable, g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->memoryProperties,
      *pSwapchain, pCreateInfo->imageFormat, pCreateInfo->imageExtent);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->DestroySwapchainKHR == NULL) {
    return;
  }

  g_Rendering->OnDestroySwapchain(device, pTable, swapchain);

  pTable->DestroySwapchainKHR(device, swapchain, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount,
                        VkImage* pSwapchainImages)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->GetSwapchainImagesKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  VkResult result =
      pTable->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
  if (pSwapchainImages == nullptr || (result != VK_SUCCESS && result != VK_INCOMPLETE)) {
    return result;
  }

  g_Rendering->OnGetSwapchainImages(pTable, swapchain, *pSwapchainImageCount, pSwapchainImages);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
  auto device = g_AppResources.GetQueueMapping(queue)->device;
  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->QueuePresentKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  auto queueFamilyIndex = g_AppResources.GetQueueMapping(queue)->queueFamilyIndex;
  auto physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;
  auto queueProperties =
      g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[queueFamilyIndex];

  auto semaphore = g_Rendering->OnPresent(
      pTable, deviceLoaderDataFunc.Get(device), queue, queueFamilyIndex, queueProperties.queueFlags,
      pPresentInfo->pSwapchains[0], pPresentInfo->pImageIndices[0],
      pPresentInfo->waitSemaphoreCount, pPresentInfo->pWaitSemaphores);

  VkPresentInfoKHR newPresentInfo = *pPresentInfo;
  if (semaphore != VK_NULL_HANDLE) {
    newPresentInfo.waitSemaphoreCount = 1;
    newPresentInfo.pWaitSemaphores = &semaphore;
  }

  return pTable->QueuePresentKHR(queue, &newPresentInfo);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
  return util_GetLayerProperties(1, &global_layer, pPropertyCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
  return util_GetLayerProperties(1, &global_layer, pPropertyCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
  if (pLayerName && strcmp(pLayerName, global_layer.layerName) == 0) {
    return util_GetExtensionProperties(0, NULL, pPropertyCount, pProperties);
  }

  return VK_ERROR_LAYER_NOT_PRESENT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName,
                                     uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
  if (pLayerName && strcmp(pLayerName, global_layer.layerName) == NULL) {
    return util_GetExtensionProperties(0, NULL, pPropertyCount, pProperties);
  }

  auto instance = g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance;
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable.Get(instance);
  if (pTable->EnumerateDeviceExtensionProperties == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  return pTable->EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount,
                                                    pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance,
                                                                               const char* funcName)
{
  for (const auto& hf : hookedInstanceFunctions) {
    if (strcmp(funcName, hf.name) == 0) {
      return hf.ptr;
    }
  }

  for (const auto& hf : hookedDeviceFunctions) {
    if (strcmp(funcName, hf.name) == 0) {
      return hf.ptr;
    }
  }

  for (const auto& hf : hookedKHRExtFunctions) {
    if (strcmp(funcName, hf.name) == 0) {
      return hf.ptr;
    }
  }

  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable.Get(instance);
  if (pTable->GetInstanceProcAddr == NULL) {
    return NULL;
  }

  return pTable->GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,
                                                                             const char* funcName)
{
  for (const auto& hf : hookedDeviceFunctions) {
    if (strcmp(funcName, hf.name) == 0) {
      return hf.ptr;
    }
  }

  for (const auto& hf : hookedKHRExtFunctions) {
    if (strcmp(funcName, hf.name) == 0) {
      return hf.ptr;
    }
  }

  VkLayerDispatchTable* pTable = deviceDispatchTable.Get(device);
  if (pTable->GetDeviceProcAddr == NULL) {
    return NULL;
  }

  return pTable->GetDeviceProcAddr(device, funcName);
}