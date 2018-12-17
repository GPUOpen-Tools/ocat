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
#include <winsock2.h>
#include <vk_layer_table.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
#include "vk_layer_extension_utils.h"

#include "AppResMapping.h"
#include "Recording/Capturing.h"
#include "Utility/FileDirectory.h"
#include "HashMap.h"
#include "Rendering.h"
#include "Utility/ProcessHelper.h"
#include "hook_manager.hpp"
#include "Compositor/vk_oculus.h"
#include "Compositor/vk_steamvr.h"
#include "d3d/steamvr.h"

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
     reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceQueueFamilyProperties)},
    {"vkGetPhysicalDeviceQueueFamilyProperties2",
     reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceQueueFamilyProperties2)},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR",
     reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceQueueFamilyProperties2KHR) } };

VulkanFunction hookedDeviceFunctions[] = {
    {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDevice)},
    {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceQueue)}
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
  pTable->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
  pTable->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");
}

static HashMap<VkInstance, VkLayerInstanceDispatchTable*> instanceDispatchTable_;
static HashMap<VkDevice, VkLayerDispatchTable*> deviceDispatchTable_;
static HashMap<VkDevice, PFN_vkSetDeviceLoaderData> deviceLoaderDataFunc_;

void hook_openvr_vulkan_compositor_object() {
  if (GetVRCompositor() == nullptr)
    return;

  if (GameOverlay::replace_vtable_hook(VTABLE(GetVRCompositor()), 5,
    reinterpret_cast<GameOverlay::hook::address>(&IVRCompositor_Submit))) {
    g_messageLog.LogInfo("SteamVR Vulkan", "Successfully installed hook for compositor submit.");
  }
}

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
  if (nReason == DLL_PROCESS_DETACH)
  {
    g_Rendering.reset();
    g_OculusVk.reset();
  }
  else if (nReason == DLL_PROCESS_ATTACH)
  {
    if (!g_fileDirectory.Initialize())
    {
      return FALSE;
    }
  }

  // OCULUS
  if (!g_OculusVk)
  {
#if _WIN64
    GameOverlay::add_function_hooks(L"LibOVRRT64_1.dll", hDllHandle);
#else
    GameOverlay::add_function_hooks(L"LibOVRRT32_1.dll", hDllHandle);
#endif
  }

  return TRUE;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkInstance* pInstance)
{
  VkLayerInstanceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
  if (!chain_info || !chain_info->u.pLayerInfo) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
      chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  if (fpGetInstanceProcAddr == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

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

  auto instanceDispatchTable = new VkLayerInstanceDispatchTable;
  layer_init_instance_dispatch_table(*pInstance, instanceDispatchTable, fpGetInstanceProcAddr);
  instanceDispatchTable_.Add(*pInstance, instanceDispatchTable);

  g_AppResources.CreateInstance(*pInstance, pCreateInfo);

  g_Rendering.reset(new Rendering(g_fileDirectory.GetDirectory(DirectoryType::Bin)));
  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable_.Get(instance);
  if (!pTable || pTable->DestroyInstance == NULL) {
    return;
  }

  g_AppResources.DestroyInstance(instance);

  pTable->DestroyInstance(instance, pAllocator);

  delete pTable;
  instanceDispatchTable_.Remove(instance);
  g_Rendering.reset();
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable_.Get(instance);
  if (!pTable || pTable->EnumeratePhysicalDevices == NULL) {
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
  if (!chain_info || !chain_info->u.pLayerInfo) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
  chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  if (!fpGetInstanceProcAddr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
  if (!fpGetDeviceProcAddr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

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
    deviceLoaderDataFunc_.Add(*pDevice, chain_info->u.pfnSetDeviceLoaderData);
  }

  auto deviceDispatchTable = new VkLayerDispatchTable;
  layer_init_device_dispatch_table(*pDevice, deviceDispatchTable, fpGetDeviceProcAddr);
  registerDeviceKHRExtFunctions(pCreateInfo, deviceDispatchTable, *pDevice);
  deviceDispatchTable_.Add(*pDevice, deviceDispatchTable);

  g_AppResources.CreateDevice(*pDevice, physicalDevice, pCreateInfo);

  g_OculusVk.reset(new CompositorOverlay::Oculus_Vk());
  g_OculusVk->SetDevice(*pDevice);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device,
                                                           const VkAllocationCallbacks* pAllocator)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->DestroyDevice == NULL) {
    return;
  }

  // destroy compositor renderer
  if (g_OculusVk)
  {
    g_OculusVk->DestroyRenderer(device, pTable);
  }

  if (GetVRCompositor() != nullptr)
  {
    g_SteamVRVk->DestroyRenderer(device, pTable);
  }

  g_AppResources.DestroyDevice(device);
  pTable->DestroyDevice(device, pAllocator);

  delete pTable;
  deviceDispatchTable_.Remove(device);
  deviceLoaderDataFunc_.Remove(device);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
  VkLayerInstanceDispatchTable* pTable =
    instanceDispatchTable_.Get(g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance);
  if (!pTable || pTable->GetPhysicalDeviceQueueFamilyProperties == NULL) {
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

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(
  VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
  VkQueueFamilyProperties2* pQueueFamilyProperties2)
{
  VkLayerInstanceDispatchTable* pTable =
    instanceDispatchTable_.Get(g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance);
  if (!pTable || pTable->GetPhysicalDeviceQueueFamilyProperties2 == NULL) {
    return;
  }

  pTable->GetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount,
    pQueueFamilyProperties2);
  if (pQueueFamilyProperties2 == nullptr || *pQueueFamilyPropertyCount == 0) {
    return;
  }

  std::vector<VkQueueFamilyProperties> temp(*pQueueFamilyPropertyCount);
  for (uint32_t i = 0; i < *pQueueFamilyPropertyCount; i++)
  {
    temp[i] = (pQueueFamilyProperties2 + i)->queueFamilyProperties;
  }

  g_AppResources.GetPhysicalDeviceMapping(physicalDevice)
    ->queueProperties.assign(temp.data(),
      temp.data() + *pQueueFamilyPropertyCount);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR(
  VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
  VkQueueFamilyProperties2KHR* pQueueFamilyProperties2KHR)
{
  VkLayerInstanceDispatchTable* pTable =
    instanceDispatchTable_.Get(g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance);
  if (!pTable || pTable->GetPhysicalDeviceQueueFamilyProperties2KHR == NULL) {
    return;
  }

  pTable->GetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount,
    pQueueFamilyProperties2KHR);
  if (pQueueFamilyProperties2KHR == nullptr || *pQueueFamilyPropertyCount == 0) {
    return;
  }

  std::vector<VkQueueFamilyProperties> temp(*pQueueFamilyPropertyCount);
  for (uint32_t i = 0; i < *pQueueFamilyPropertyCount; i++)
  {
    temp[i] = (pQueueFamilyProperties2KHR + i)->queueFamilyProperties;
  }

  g_AppResources.GetPhysicalDeviceMapping(physicalDevice)
    ->queueProperties.assign(temp.data(),
      temp.data() + *pQueueFamilyPropertyCount);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex, VkQueue* pQueue)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->GetDeviceQueue == NULL) {
    return;
  }

  pTable->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

  g_AppResources.GetDeviceQueue(*pQueue, device, queueFamilyIndex, queueIndex);

  // if it's graphics queue, set the queue also for the compositor
  // if it's compute, only set it if compositor queue is still null handle
  // don't set if it's copy queue

  auto physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;
  if (g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties.size() > 0) {
    auto queueProperties =
      g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[queueFamilyIndex];

    if ((queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 1) {
      g_OculusVk->SetQueue(*pQueue);
    }
    else if ((queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 1
      && g_OculusVk->GetQueue() == VK_NULL_HANDLE) {
      g_OculusVk->SetQueue(*pQueue);
    }
  }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
                     const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->CreateSwapchainKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  VkPhysicalDevice physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;
  VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
  VkLayerInstanceDispatchTable* pInstanceTable =
    instanceDispatchTable_.Get(g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->instance);
  if (!pInstanceTable) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // check for storage bit support with given format and surface capabilities
  // if not: we can't support present on compute queue
  if (pInstanceTable->GetPhysicalDeviceFormatProperties != NULL &&
    pInstanceTable->GetPhysicalDeviceSurfaceCapabilitiesKHR != NULL)
  {
    VkFormatProperties formatProperties;
    pInstanceTable->GetPhysicalDeviceFormatProperties(physicalDevice, createInfo.imageFormat,
      &formatProperties);
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    pInstanceTable->GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, createInfo.surface,
      &surfaceCapabilities);
    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT &&
      surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)
    {
      createInfo.imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    else {
      g_messageLog.LogInfo("VulkanOverlay", "Can't set storage bit - don't support present on compute.");
    }
  }

  VkResult result = pTable->CreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
  if (result != VK_SUCCESS) {
    return result;
  }

  g_Rendering->OnCreateSwapchain(
      device, pTable, g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->memoryProperties,
      *pSwapchain, createInfo.imageFormat, pCreateInfo->imageExtent, createInfo.imageUsage);

  // if we find a VRCompositor, create swapchain for HMD overlay
  if (GetVRCompositor() != nullptr) {
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    g_SteamVRVk.reset(new CompositorOverlay::SteamVR_Vk());
    g_SteamVRVk->SetDevice(device);
    pTable->CreateSwapchainKHR(device, &createInfo, pAllocator, g_SteamVRVk->GetSwapchain());
    hook_openvr_vulkan_compositor_object();
    g_SteamVRVk->Init(pTable, g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->memoryProperties,
      createInfo.imageFormat, createInfo.imageUsage);
  }

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->DestroySwapchainKHR == NULL) {
    return;
  }

  g_Rendering->OnDestroySwapchain(device, pTable, swapchain);

  pTable->DestroySwapchainKHR(device, swapchain, pAllocator);

  if (g_SteamVRVk)
  {
    pTable->DestroySwapchainKHR(device, *g_SteamVRVk->GetSwapchain(), pAllocator);
  }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount,
                        VkImage* pSwapchainImages)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->GetSwapchainImagesKHR == NULL) {
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
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->QueuePresentKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  auto queueFamilyIndex = g_AppResources.GetQueueMapping(queue)->queueFamilyIndex;
  auto physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;

  VkQueueFamilyProperties queueProperties = {};
  if (g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties.size() > 0) {
    queueProperties =
      g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[queueFamilyIndex];
  }
  else {
    // we don't know the queue properties, so we assume it's on the graphics queue
    queueProperties.queueFlags = VK_QUEUE_GRAPHICS_BIT;
    queueProperties.queueCount = 1;
  }

  auto semaphore = g_Rendering->OnPresent(
    pTable, deviceLoaderDataFunc_.Get(device), queue, queueFamilyIndex, queueProperties.queueFlags,
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
  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable_.Get(instance);
  if (!pTable || pTable->EnumerateDeviceExtensionProperties == NULL) {
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

  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable_.Get(instance);
  if (!pTable || pTable->GetInstanceProcAddr == NULL) {
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

  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (!pTable || pTable->GetDeviceProcAddr == NULL) {
    return NULL;
  }

  return pTable->GetDeviceProcAddr(device, funcName);
}

// Oculus Compositor

OVR_PUBLIC_FUNCTION(ovrResult) ovr_EndFrame(ovrSession session, long long frameIndex,
                                            const ovrViewScaleDesc* viewScaleDesc,
                                            ovrLayerHeader const* const* layerPtrList,
                                            unsigned int layerCount)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(g_OculusVk->GetDevice());
  VkPhysicalDevice physicalDevice = g_AppResources.GetDeviceMapping(
    g_OculusVk->GetDevice())->physicalDevice;

  if (g_OculusVk->Init(pTable, g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->memoryProperties,
    session))
  {
    auto queueFamilyIndex = g_AppResources.GetQueueMapping(g_OculusVk->GetQueue())->queueFamilyIndex;
    auto physicalDevice = g_AppResources.GetDeviceMapping(g_OculusVk->GetDevice())->physicalDevice;
    auto queueProperties =
      g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[queueFamilyIndex];

   // if something failed during rendering, just submit frame without overlay
   if (!g_OculusVk->Render(pTable, deviceLoaderDataFunc_.Get(g_OculusVk->GetDevice()),
     queueFamilyIndex, queueProperties.queueFlags, session)) {
     return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
       session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
   }

    ovrLayerQuad overlayLayer;
    overlayLayer.Header.Type = ovrLayerType_Quad;
    overlayLayer.Header.Flags = ovrLayerFlag_HighQuality | ovrLayerFlag_HeadLocked;
    overlayLayer.ColorTexture = g_OculusVk->GetSwapChain();

    const auto overlayPosition = RecordingState::GetInstance().GetOverlayPosition();

    overlayLayer.QuadPoseCenter.Position.y = -0.10f;
    overlayLayer.QuadPoseCenter.Position.x = -0.10f;

    overlayLayer.QuadPoseCenter.Position.z = -1.00f;
    overlayLayer.QuadPoseCenter.Orientation.x = 0;
    overlayLayer.QuadPoseCenter.Orientation.y = 0;
    overlayLayer.QuadPoseCenter.Orientation.z = 0;
    overlayLayer.QuadPoseCenter.Orientation.w = 1;
    // HUD is 50cm wide, 30cm tall.
    overlayLayer.QuadSize.x = 0.50f;
    overlayLayer.QuadSize.y = 0.30f;
    // Display all of the HUD texture.
    overlayLayer.Viewport = g_OculusVk->GetViewport();

    // append overlayLayer to layer list
    std::vector<const ovrLayerHeader*> list;
    list.resize(layerCount + 1);
    for (uint32_t i = 0; i < layerCount; i++) {

      list[i] = *(layerPtrList + i);
    }
    list[layerCount] = &overlayLayer.Header;

    return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
      session, frameIndex, viewScaleDesc, list.data(), layerCount + 1);
  }

  return GameOverlay::find_hook_trampoline(&ovr_EndFrame)(
    session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
}


// SteamVR Compositor

vr::EVRCompositorError VR_CALLTYPE
IVRCompositor_Submit(vr::IVRCompositor* pCompositor, vr::EVREye eEye,
                     const vr::Texture_t *pTexture, const vr::VRTextureBounds_t* pBounds, vr::EVRSubmitFlags nSubmitFlags)
{
  // only update overlay once a frame and not twice, so just consider one left eye submit call
  if (eEye == vr::Eye_Left)
  {
    if (g_SteamVRVk->IsInitialized())
    {
      VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(g_SteamVRVk->GetDevice());
      VkPhysicalDevice physicalDevice = g_AppResources.GetDeviceMapping(
        g_SteamVRVk->GetDevice())->physicalDevice;
      vr::VRVulkanTextureData_t* vkTextureData =
        static_cast<vr::VRVulkanTextureData_t*>(pTexture->handle);

      auto queueProperties =
        g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[vkTextureData->m_nQueueFamilyIndex];

      g_SteamVRVk->Render(pTexture, pTable, deviceLoaderDataFunc_.Get(g_SteamVRVk->GetDevice()),
        vkTextureData->m_pQueue, vkTextureData->m_nQueueFamilyIndex,
        queueProperties.queueFlags);
    }
  }

  return  GameOverlay::find_hook_trampoline(&IVRCompositor_Submit)(
    pCompositor, eEye, pTexture, pBounds, nSubmitFlags);
}
