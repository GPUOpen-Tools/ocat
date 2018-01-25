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
#include "hook_manager.hpp"
#include "Compositor\vk_oculus.h"

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

static HashMap<VkInstance, VkLayerInstanceDispatchTable*> instanceDispatchTable_;
static HashMap<VkDevice, VkLayerDispatchTable*> deviceDispatchTable_;
static HashMap<VkDevice, PFN_vkSetDeviceLoaderData> deviceLoaderDataFunc_;

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
  if (pTable->DestroyInstance == NULL) {
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
	if (pTable->DestroyDevice == NULL) {
		return;
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
	VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
	if (pTable->GetDeviceQueue == NULL) {
		return;
	}

	pTable->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

	g_AppResources.GetDeviceQueue(*pQueue, device, queueFamilyIndex, queueIndex);

	// if it's graphics queue, set the queue also for the compositor
	// if it's compute, only set it if compositor queue is still null handle
	// don't set if it's copy queue

	auto physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;
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

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
                     const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
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
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
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
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
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
  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (pTable->QueuePresentKHR == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  auto queueFamilyIndex = g_AppResources.GetQueueMapping(queue)->queueFamilyIndex;
  auto physicalDevice = g_AppResources.GetDeviceMapping(device)->physicalDevice;
  auto queueProperties =
      g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->queueProperties[queueFamilyIndex];

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

  VkLayerInstanceDispatchTable* pTable = instanceDispatchTable_.Get(instance);
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

  VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(device);
  if (pTable->GetDeviceProcAddr == NULL) {
    return NULL;
  }

  return pTable->GetDeviceProcAddr(device, funcName);
}

// Oculus Compositor

OVR_PUBLIC_FUNCTION(ovrResult) ovr_EndFrame(ovrSession session, long long frameIndex,
	const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const* const* layerPtrList,
	unsigned int layerCount)
{
	VkLayerDispatchTable* pTable = deviceDispatchTable_.Get(g_OculusVk->GetDevice());
	VkPhysicalDevice physicalDevice = g_AppResources.GetDeviceMapping(
		g_OculusVk->GetDevice())->physicalDevice;

	if (g_OculusVk->Init(pTable, g_AppResources.GetPhysicalDeviceMapping(physicalDevice)->memoryProperties, 
		session)) {
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