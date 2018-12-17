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

#include "Rendering.h"
#include <algorithm>
#include <fstream>
#include <array>

#include "Recording/Capturing.h"

void Rendering::OnDestroyCompositor(VkLayerDispatchTable* pTable)
{
  // check if we actually created a compositor swapchain mapping before
  if (overlayBitmapInitialized)
    DestroySwapchain(pTable, &compositorSwapchainMapping_);
}

void Rendering::OnDestroySwapchain(VkDevice device, VkLayerDispatchTable* pTable,
  VkSwapchainKHR swapchain)
{
  SwapchainMapping* sm = swapchainMappings_.Get(swapchain);
  if (sm == nullptr)
  {
    return;
  }
  DestroySwapchain(pTable, sm);
  swapchainMappings_.Remove(swapchain);
  delete sm;
}

void Rendering::DestroySwapchain(VkLayerDispatchTable* pTable, SwapchainMapping* sm)
{
  for (int i = 0; i < 2; ++i)
  {
    if (sm->overlayImages[i].bufferView != VK_NULL_HANDLE)
    {
      pTable->DestroyBufferView(sm->device, sm->overlayImages[i].bufferView, nullptr);
    }

    if (sm->overlayImages[i].overlayHostBuffer != VK_NULL_HANDLE)
    {
      pTable->DestroyBuffer(sm->device, sm->overlayImages[i].overlayHostBuffer, nullptr);
    }

    if (sm->overlayImages[i].overlayHostMemory != VK_NULL_HANDLE)
    {
      pTable->FreeMemory(sm->device, sm->overlayImages[i].overlayHostMemory, nullptr);
    }

    if (sm->overlayImages[i].overlayBuffer != VK_NULL_HANDLE)
    {
      pTable->DestroyBuffer(sm->device, sm->overlayImages[i].overlayBuffer, nullptr);
    }

    if (sm->overlayImages[i].overlayMemory != VK_NULL_HANDLE)
    {
      pTable->FreeMemory(sm->device, sm->overlayImages[i].overlayMemory, nullptr);
    }

    if (sm->overlayImages[i].overlayCopySemaphore != VK_NULL_HANDLE)
    {
      pTable->DestroySemaphore(sm->device, sm->overlayImages[i].overlayCopySemaphore, nullptr);
    }

    if (sm->overlayImages[i].commandBufferFence[0] != VK_NULL_HANDLE)
    {
      pTable->DestroyFence(sm->device, sm->overlayImages[i].commandBufferFence[0], nullptr);
    }

    if (sm->overlayImages[i].commandBufferFence[1] != VK_NULL_HANDLE)
    {
      pTable->DestroyFence(sm->device, sm->overlayImages[i].commandBufferFence[1], nullptr);
    }
  }

  if (sm->uniformBuffer != VK_NULL_HANDLE)
  {
    pTable->DestroyBuffer(sm->device, sm->uniformBuffer, nullptr);
  }

  if (sm->uniformMemory != VK_NULL_HANDLE)
  {
    pTable->FreeMemory(sm->device, sm->uniformMemory, nullptr);
  }

  if (sm->renderPass != VK_NULL_HANDLE)
  {
    pTable->DestroyRenderPass(sm->device, sm->renderPass, nullptr);
  }

  if (sm->gfxPipelineLayout != VK_NULL_HANDLE)
  {
    pTable->DestroyPipelineLayout(sm->device, sm->gfxPipelineLayout, nullptr);
  }

  if (sm->gfxPipeline != VK_NULL_HANDLE)
  {
    pTable->DestroyPipeline(sm->device, sm->gfxPipeline, nullptr);
  }

  if (sm->computePipeline != VK_NULL_HANDLE)
  {
    pTable->DestroyPipeline(sm->device, sm->computePipeline, nullptr);
  }

  for (auto& qm : sm->queueMappings)
  {
    for (auto& im : qm.second.imageMappings)
    {
      pTable->DestroySemaphore(sm->device, im.semaphore, nullptr);
    }

    pTable->DestroyCommandPool(sm->device, qm.second.commandPool, nullptr);
  }

  sm->ClearImageData(pTable);
}

uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
  uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
  {
    if (memoryTypeBits & (1 << i))
    {
      if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
      {
        return i;
      }
    }
  }
  return UINT32_MAX;
}

Rendering::Rendering(const std::wstring& shaderDirectory) : shaderDirectory_(shaderDirectory)
{
  // Empty
}

VkResult Rendering::CreateOverlayImageBuffer(VkDevice device,
  VkLayerDispatchTable* pTable, SwapchainMapping* sm,
  OverlayImageData& overlayImage, VkBuffer & uniformBuffer, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties)
{
  VkBufferCreateInfo overlayHostBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  overlayHostBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  overlayHostBufferInfo.size = sm->overlayRect.extent.width * sm->overlayRect.extent.height * 4;
  overlayHostBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VkResult result = pTable->CreateBuffer(device, &overlayHostBufferInfo, nullptr,
    &overlayImage.overlayHostBuffer);
  if (result != VK_SUCCESS)
  {
    return result;
  }

  VkMemoryRequirements memoryRequirements;
  pTable->GetBufferMemoryRequirements(device, overlayImage.overlayHostBuffer,
    &memoryRequirements);

  VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(
    physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
  {
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  result = pTable->AllocateMemory(device, &memoryAllocateInfo, nullptr,
    &overlayImage.overlayHostMemory);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  result = pTable->BindBufferMemory(device, overlayImage.overlayHostBuffer,
    overlayImage.overlayHostMemory, 0);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  VkBufferCreateInfo overlayBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  overlayBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  overlayBufferInfo.size = sm->overlayRect.extent.width * sm->overlayRect.extent.height * 4;
  overlayBufferInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

  result = pTable->CreateBuffer(device, &overlayBufferInfo, nullptr, &overlayImage.overlayBuffer);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  pTable->GetBufferMemoryRequirements(device, overlayImage.overlayBuffer,
    &memoryRequirements);
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex =
    GetMemoryTypeIndex(physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
  {
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  result = pTable->AllocateMemory(device, &memoryAllocateInfo, nullptr,
    &overlayImage.overlayMemory);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  result = pTable->BindBufferMemory(device, overlayImage.overlayBuffer,
    overlayImage.overlayMemory, 0);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  VkBufferViewCreateInfo bvCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
  bvCreateInfo.buffer = overlayImage.overlayBuffer;
  bvCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  bvCreateInfo.range = VK_WHOLE_SIZE;

  result = pTable->CreateBufferView(sm->device, &bvCreateInfo, nullptr,
    &overlayImage.bufferView);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    return result;
  }

  VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
  result = pTable->CreateSemaphore(device, &semaphoreCreateInfo, nullptr, &overlayImage.overlayCopySemaphore);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    pTable->DestroyBufferView(device, overlayImage.bufferView, nullptr);
    return result;
  }
#if _DEBUG
  VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
  result = pTable->CreateFence(device, &createInfo, nullptr, &overlayImage.commandBufferFence[0]);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    pTable->DestroyBufferView(device, overlayImage.bufferView, nullptr);
    return result;
  }

  result = pTable->CreateFence(device, &createInfo, nullptr, &overlayImage.commandBufferFence[1]);
  if (result != VK_SUCCESS)
  {
    pTable->FreeMemory(device, overlayImage.overlayMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayBuffer, nullptr);
    pTable->FreeMemory(device, overlayImage.overlayHostMemory, nullptr);
    pTable->DestroyBuffer(device, overlayImage.overlayHostBuffer, nullptr);
    pTable->DestroyBufferView(device, overlayImage.bufferView, nullptr);
    return result;
  }
#endif

  return VK_SUCCESS;
}


VkResult Rendering::CreateUniformBuffer(VkDevice device, VkLayerDispatchTable * pTable,
  SwapchainMapping * sm, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties)
{
  VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  bufferInfo.size = 4 * sizeof(int);
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = pTable->CreateBuffer(device, &bufferInfo, nullptr, &sm->uniformBuffer);
  if (result != VK_SUCCESS)
  {
    g_messageLog.LogError("CreateUniformBuffer", "Failed to create uniform buffer.");
    return result;
  }

  VkMemoryRequirements memoryRequirements;
  pTable->GetBufferMemoryRequirements(device, sm->uniformBuffer, &memoryRequirements);
  VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  allocateInfo.allocationSize = memoryRequirements.size;
  allocateInfo.memoryTypeIndex = GetMemoryTypeIndex(
    physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  result = pTable->AllocateMemory(device, &allocateInfo, nullptr, &sm->uniformMemory);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyBuffer(device, sm->uniformBuffer, nullptr);
    g_messageLog.LogError("CreateUniformBuffer", "Failed to allocate buffer memory.");
    return result;
  }

  result = pTable->BindBufferMemory(device, sm->uniformBuffer, sm->uniformMemory, 0);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyBuffer(device, sm->uniformBuffer, nullptr);
    pTable->FreeMemory(device, sm->uniformMemory, nullptr);
    g_messageLog.LogError("CreateUniformBuffer", "Failed to bind buffer memory.");
    return result;
  }

  return VK_SUCCESS;
}

bool Rendering::InitRenderPass(VkLayerDispatchTable* pTable,
  const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
  SwapchainMapping* sm)
{
  if (!overlayBitmapInitialized)
  {
    GameOverlay::InitLogging("VulkanOverlay");
    GameOverlay::InitCapturing();
  
    overlayBitmap_.reset(new OverlayBitmap());
    if (!overlayBitmap_->Init(
      static_cast<int>(sm->extent.width),
      static_cast<int>(sm->extent.height),
      OverlayBitmap::API::Vulkan))
    {
      return false;
    }
    overlayBitmapInitialized = true;
  }
  overlayBitmap_->Resize(sm->extent.width, sm->extent.height);

  sm->overlayFormat = overlayBitmap_->GetVKFormat();

  auto screenPos = overlayBitmap_->GetScreenPos();
  sm->overlayRect.offset.x = screenPos.x;
  sm->overlayRect.offset.y = screenPos.y;
  sm->overlayRect.extent.width = overlayBitmap_->GetFullWidth();
  sm->overlayRect.extent.height = overlayBitmap_->GetFullHeight();

  for (int i = 0; i < 2; ++i)
  {
    VkResult result = CreateOverlayImageBuffer(sm->device, pTable, sm, sm->overlayImages[i], sm->uniformBuffer, physicalDeviceMemoryProperties);
    if (result != VK_SUCCESS)
    {
      return false;
    }
  }

  VkResult result = CreateUniformBuffer(sm->device, pTable, sm, physicalDeviceMemoryProperties);
  if (result != VK_SUCCESS)
  {
    return false;
  }

  VkAttachmentDescription colorAttachmentDesc = { 0,
    sm->format,
    VK_SAMPLE_COUNT_1_BIT,
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

  VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpassDesc = {};
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDesc.colorAttachmentCount = 1;
  subpassDesc.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo rpCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    nullptr,
    0,
    1,
    &colorAttachmentDesc,
    1,
    &subpassDesc,
    1,
    &dependency };

  result = pTable->CreateRenderPass(sm->device, &rpCreateInfo, nullptr, &sm->renderPass);
  if (result != VK_SUCCESS)
  {
    return false;
  }

  return true;
}

void Rendering::OnCreateSwapchain(
  VkDevice device, VkLayerDispatchTable* pTable,
  const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
  VkSwapchainKHR swapchain, VkFormat format, const VkExtent2D& extent,
  const VkImageUsageFlags usage)
{
  SwapchainMapping* sm = new SwapchainMapping{ device, format, VK_FORMAT_B8G8R8A8_UNORM, extent, usage };
  swapchainMappings_.Add(swapchain, sm);

  InitRenderPass(pTable, physicalDeviceMemoryProperties, sm);
}

VkResult Rendering::CreateFrameBuffer(VkLayerDispatchTable * pTable,
  SwapchainMapping * sm, SwapchainImageData & imageData, VkImage & image)
{
  imageData.image = image;

  VkImageViewCreateInfo ivCreateInfo = {
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    nullptr,
    0,
    image,
    VK_IMAGE_VIEW_TYPE_2D,
    sm->format,
    { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
    { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };

  VkResult result = pTable->CreateImageView(sm->device, &ivCreateInfo, nullptr, &imageData.view);
  if (result != VK_SUCCESS)
  {
    sm->ClearImageData(pTable);
    g_messageLog.LogError("CreateFrameBuffer", "Failed to create image view.");
    return result;
  }

  VkFramebufferCreateInfo fbCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    nullptr,
    0,
    sm->renderPass,
    1,
    &imageData.view,
    sm->extent.width,
    sm->extent.height,
    1 };

  result = pTable->CreateFramebuffer(sm->device, &fbCreateInfo, nullptr, &imageData.framebuffer);
  if (result != VK_SUCCESS)
  {
    sm->ClearImageData(pTable);
    g_messageLog.LogError("CreateFrameBuffer", "Failed to create frame buffer.");
    return result;
  }
  return VK_SUCCESS;
}

bool Rendering::InitPipeline(VkLayerDispatchTable* pTable, uint32_t imageCount,
  VkImage* images, SwapchainMapping* sm)
{
  if (sm == nullptr || sm->renderPass == VK_NULL_HANDLE)
  {
    return false;
  }

  if (sm->imageData.size() != 0)
  {
    sm->ClearImageData(pTable);
  }

  sm->imageData.resize(imageCount);

  for (uint32_t i = 0; i < imageCount; ++i)
  {
    VkResult result = CreateFrameBuffer(pTable, sm, sm->imageData[i], images[i]);
    if (result != VK_SUCCESS)
    {
      return false;
    }
  }

  VkDescriptorPoolSize descriptorPoolSizes[3] = { {},{},{} };
  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorPoolSizes[0].descriptorCount = 2 * imageCount;
  descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  descriptorPoolSizes[1].descriptorCount = 2 * imageCount + 2;
  descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSizes[2].descriptorCount = 2 * imageCount + 2;

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  descriptorPoolCreateInfo.poolSizeCount = 3;
  descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
  descriptorPoolCreateInfo.maxSets = 2 * imageCount + 2;

  VkResult result = pTable->CreateDescriptorPool(sm->device, &descriptorPoolCreateInfo, nullptr,
    &sm->descriptorPool);
  if (result != VK_SUCCESS)
  {
    g_messageLog.LogError("OnGetSwapchainImages", "Failed to create descriptor pool.");
    return false;
  }

  // compute pipeline for present on compute
  if (sm->usage & VK_IMAGE_USAGE_STORAGE_BIT)
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[3] = { {},{},{} };
    descriptorSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    descriptorSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorSetLayoutBinding[0].binding = 0;
    descriptorSetLayoutBinding[0].descriptorCount = 1;
    descriptorSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorSetLayoutBinding[1].binding = 1;
    descriptorSetLayoutBinding[1].descriptorCount = 1;
    descriptorSetLayoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorSetLayoutBinding[2].binding = 2;
    descriptorSetLayoutBinding[2].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBinding;
    descriptorSetLayoutCreateInfo.bindingCount = 3;

    VkDescriptorSetLayout computeDescriptorSetLayout;
    result = pTable->CreateDescriptorSetLayout(sm->device, &descriptorSetLayoutCreateInfo, nullptr,
      &computeDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
      g_messageLog.LogError("OnGetSwapchainImages", "Failed to create compute descriptor set layout.");
      return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &computeDescriptorSetLayout;

    result = pTable->CreatePipelineLayout(sm->device, &pipelineLayoutCreateInfo, nullptr,
      &sm->computePipelineLayout);
    if (result != VK_SUCCESS)
    {
      pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);
      g_messageLog.LogError("OnGetSwapchainImages", "Fa�led to create compute pipeline layout.");
      return false;
    }

    for (auto& sid : sm->imageData)
    {
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
      descriptorSetAllocateInfo.descriptorPool = sm->descriptorPool;
      descriptorSetAllocateInfo.pSetLayouts = &computeDescriptorSetLayout;
      descriptorSetAllocateInfo.descriptorSetCount = 1;

      result = pTable->AllocateDescriptorSets(sm->device, &descriptorSetAllocateInfo,
        &sid.computeDescriptorSet[0]);
      if (result != VK_SUCCESS)
      {
        pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);
        return false;
      }

      result = pTable->AllocateDescriptorSets(sm->device, &descriptorSetAllocateInfo,
        &sid.computeDescriptorSet[1]);
      if (result != VK_SUCCESS)
      {
        pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);
        return false;
      }

      VkDescriptorImageInfo descriptorImageInfo = {};
      descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      descriptorImageInfo.imageView = sid.view;

      VkDescriptorBufferInfo descriptorBufferInfo = {};
      descriptorBufferInfo.range = 4 * sizeof(int);
      descriptorBufferInfo.offset = 0;
      descriptorBufferInfo.buffer = sm->uniformBuffer;

      VkWriteDescriptorSet writeDescriptorSet[3] = {
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };
      writeDescriptorSet[0].dstSet = sid.computeDescriptorSet[0];
      writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
      writeDescriptorSet[0].dstBinding = 0;
      writeDescriptorSet[0].pTexelBufferView = &sm->overlayImages[0].bufferView;
      writeDescriptorSet[0].descriptorCount = 1;
      writeDescriptorSet[1].dstSet = sid.computeDescriptorSet[0];
      writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      writeDescriptorSet[1].dstBinding = 1;
      writeDescriptorSet[1].pImageInfo = &descriptorImageInfo;
      writeDescriptorSet[1].descriptorCount = 1;
      writeDescriptorSet[2].dstSet = sid.computeDescriptorSet[0];
      writeDescriptorSet[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSet[2].dstBinding = 2;
      writeDescriptorSet[2].pBufferInfo = &descriptorBufferInfo;
      writeDescriptorSet[2].descriptorCount = 1;

      pTable->UpdateDescriptorSets(sm->device, 3, writeDescriptorSet, 0, NULL);

      writeDescriptorSet[0].dstSet = sid.computeDescriptorSet[1];
      writeDescriptorSet[0].pTexelBufferView = &sm->overlayImages[1].bufferView;
      writeDescriptorSet[1].dstSet = sid.computeDescriptorSet[1];
      writeDescriptorSet[2].dstSet = sid.computeDescriptorSet[1];

      pTable->UpdateDescriptorSets(sm->device, 3, writeDescriptorSet, 0, NULL);
    }

    VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    computeShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageCreateInfo.module =
      CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + L"comp.spv");
    computeShaderStageCreateInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    computePipelineCreateInfo.layout = sm->computePipelineLayout;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.stage = computeShaderStageCreateInfo;

    pTable->CreateComputePipelines(sm->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
      &sm->computePipeline);

    pTable->DestroyShaderModule(sm->device, computeShaderStageCreateInfo.module, nullptr);
    pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);
  }

  VkPipelineShaderStageCreateInfo shaderStages[2] = {
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
  { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };

  VkShaderModule shaderModules[2];

  shaderModules[0] = CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + L"vert.spv");
  if (shaderModules[0] == VK_NULL_HANDLE)
  {
    return false;
  }

  shaderModules[1] = CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + L"frag.spv");
  if (shaderModules[1] == VK_NULL_HANDLE)
  {
    pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
    return false;
  }

  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = shaderModules[0];
  shaderStages[0].pName = "main";
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = shaderModules[1];
  shaderStages[1].pName = "main";

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkViewport viewport;
  viewport.maxDepth = 1.0f;
  viewport.minDepth = 0.0f;
  viewport.x = static_cast<float>(sm->overlayRect.offset.x);
  viewport.y = static_cast<float>(sm->overlayRect.offset.y);
  viewport.width = static_cast<float>(sm->overlayRect.extent.width);
  viewport.height = static_cast<float>(sm->overlayRect.extent.height);

  VkRect2D scissor = {};
  scissor.extent.width = sm->extent.width;
  scissor.extent.height = sm->extent.height;

  VkPipelineViewportStateCreateInfo viewportState = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizationState = {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.cullMode = VK_CULL_MODE_NONE;
  rasterizationState.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampleState = {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
  colorBlendAttachmentState.blendEnable = VK_TRUE;
  colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlendState = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  colorBlendState.attachmentCount = 1;
  colorBlendState.pAttachments = &colorBlendAttachmentState;
  colorBlendState.blendConstants[0] = 1.0f;
  colorBlendState.blendConstants[1] = 1.0f;
  colorBlendState.blendConstants[2] = 1.0f;
  colorBlendState.blendConstants[3] = 1.0f;

  VkPipelineVertexInputStateCreateInfo vertexInputState = {
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

  VkDescriptorSetLayoutBinding gfxDescriptorSetLayoutBinding[2] = { {},{} };
  gfxDescriptorSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  gfxDescriptorSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  gfxDescriptorSetLayoutBinding[0].binding = 0;
  gfxDescriptorSetLayoutBinding[0].descriptorCount = 1;
  gfxDescriptorSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  gfxDescriptorSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  gfxDescriptorSetLayoutBinding[1].binding = 1;
  gfxDescriptorSetLayoutBinding[1].descriptorCount = 1;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  descriptorSetLayoutCreateInfo.pBindings = gfxDescriptorSetLayoutBinding;
  descriptorSetLayoutCreateInfo.bindingCount = 2;

  VkDescriptorSetLayout gfxDescriptorSetLayout;
  result = pTable->CreateDescriptorSetLayout(sm->device, &descriptorSetLayoutCreateInfo, nullptr,
    &gfxDescriptorSetLayout);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
    pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
    g_messageLog.LogError("OnGetSwapchainImages", "Failed to create graphics descriptor set layout.");
    return false;
  }

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &gfxDescriptorSetLayout;

  result = pTable->CreatePipelineLayout(sm->device, &pipelineLayoutCreateInfo, nullptr,
    &sm->gfxPipelineLayout);
  if (result != VK_SUCCESS)
  {
    pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
    pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
    pTable->DestroyDescriptorSetLayout(sm->device, gfxDescriptorSetLayout, nullptr);
    return false;
  }

  for (int i = 0; i < 2; ++i)
  {
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    descriptorSetAllocateInfo.descriptorPool = sm->descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = &gfxDescriptorSetLayout;
    descriptorSetAllocateInfo.descriptorSetCount = 1;

    result = pTable->AllocateDescriptorSets(sm->device, &descriptorSetAllocateInfo,
      &sm->overlayImages[i].descriptorSet);
    if (result != VK_SUCCESS)
    {
      pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
      pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
      pTable->DestroyDescriptorSetLayout(sm->device, gfxDescriptorSetLayout, nullptr);
      g_messageLog.LogError("OnGetSwapchainImages", "Failed to allocate graphics descriptor sets.");
      return false;
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = sm->uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = 4 * sizeof(int);

    VkWriteDescriptorSet writeDescriptorSets[2] = { { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
    { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };
    writeDescriptorSets[0].dstSet = sm->overlayImages[i].descriptorSet;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].pTexelBufferView = &sm->overlayImages[i].bufferView;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[1].dstSet = sm->overlayImages[i].descriptorSet;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].pBufferInfo = &bufferInfo;
    writeDescriptorSets[1].descriptorCount = 1;

    pTable->UpdateDescriptorSets(sm->device, 2, writeDescriptorSets, 0, NULL);
  }

  VkDynamicState dynamicState = VK_DYNAMIC_STATE_VIEWPORT;

  VkPipelineDynamicStateCreateInfo pipelineDynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
  pipelineDynamicState.dynamicStateCount = 1;
  pipelineDynamicState.pDynamicStates = &dynamicState;

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages;
  pipelineCreateInfo.pVertexInputState = &vertexInputState;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.layout = sm->gfxPipelineLayout;
  pipelineCreateInfo.renderPass = sm->renderPass;
  pipelineCreateInfo.pDynamicState = &pipelineDynamicState;

  result = pTable->CreateGraphicsPipelines(sm->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
    nullptr, &sm->gfxPipeline);

  pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
  pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
  pTable->DestroyDescriptorSetLayout(sm->device, gfxDescriptorSetLayout, nullptr);
  if (result != VK_SUCCESS)
  {
    g_messageLog.LogError("OnGetSwapchainImages", "Failed to create graphics pipeline.");
    return false;
  }

  pipelineInitialized = true;

  return true;
}

void Rendering::OnGetSwapchainImages(VkLayerDispatchTable* pTable, VkSwapchainKHR swapchain,
  uint32_t imageCount, VkImage* images)
{
  SwapchainMapping* sm = swapchainMappings_.Get(swapchain);
  InitPipeline(pTable, imageCount, images, sm);
}

bool Rendering::OnInitCompositor(VkDevice device, VkLayerDispatchTable* pTable,
  const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
  VkFormat format, const VkExtent2D& extent, VkImageUsageFlags usage,
  uint32_t imageCount, VkImage* images)
{
  // compositor swapchain, use R8G8B8A8 ordering
  bool initialized = false;
  compositorSwapchainMapping_ = SwapchainMapping{ device, format, VK_FORMAT_B8G8R8A8_UNORM, extent, usage };
  initialized = InitRenderPass(pTable, physicalDeviceMemoryProperties, &compositorSwapchainMapping_);
  initialized = InitPipeline(pTable, imageCount, images, &compositorSwapchainMapping_);

  return initialized;
}

VkResult Rendering::UpdateUniformBuffer(VkLayerDispatchTable* pTable, SwapchainMapping* sm)
{
  void* data;
  VkResult result = pTable->MapMemory(sm->device, sm->uniformMemory, 0, 4 * sizeof(int), 0, &data);
  if (result != VK_SUCCESS)
  {
    g_messageLog.LogError("UpdateOverlayPosition", "Failed to map memory.");
    return result;
  }

  int* constants = static_cast<int*>(data);
  constants[0] = static_cast<int>(sm->overlayRect.offset.x);
  constants[1] = static_cast<int>(sm->overlayRect.offset.y);
  constants[2] = static_cast<int>(sm->overlayRect.extent.width);
  constants[3] = static_cast<int>(sm->overlayRect.extent.height);
  pTable->UnmapMemory(sm->device, sm->uniformMemory);

  return VK_SUCCESS;
}

VkResult Rendering::UpdateOverlayPosition(VkLayerDispatchTable* pTable,
  SwapchainMapping* sm, const OverlayBitmap::Position& position)
{
  sm->overlayRect.offset.x = position.x;
  sm->overlayRect.offset.y = position.y;

  // Update uniform buffer
  VkResult result = UpdateUniformBuffer(pTable, sm);
  if (result != VK_SUCCESS)
  {
    g_messageLog.LogError("UpdateOverlayPosition", "Failed to update uniform buffer.");
    return result;
  }
  return VK_SUCCESS;
}

VkSemaphore Rendering::Present(VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr, VkQueue queue,
  uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
  uint32_t imageIndex, uint32_t waitSemaphoreCount,
  const VkSemaphore* pWaitSemaphores, SwapchainMapping* swapchainMapping)
{
  if ((queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0)
  {
    return VK_NULL_HANDLE;
  }

  int32_t graphicsQueue = queueFlags & VK_QUEUE_GRAPHICS_BIT;

  if (graphicsQueue == 0) 
  {
    // compute queue -> check if storage bit is set, otherwise return
    if (!(swapchainMapping->usage & VK_IMAGE_USAGE_STORAGE_BIT))
    {
      return VK_NULL_HANDLE;
    }
  }

  VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
  SwapchainImageMapping* imageMapping = nullptr;
  SwapchainQueueMapping* queueMapping = nullptr;

  for (auto& qm : swapchainMapping->queueMappings)
  {
    if (qm.second.queue == queue)
    {
      queueMapping = &qm.second;
      for (auto& im : qm.second.imageMappings)
      {
        if (im.imageIndex == imageIndex)
        {
          imageMapping = &im;
          break;
        }
      }
      break;
    }
  }

  if (cmdBuffer == VK_NULL_HANDLE)
  {
    if (queueMapping == nullptr)
    {
      VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex };
      VkCommandPool cmdPool;
      VkResult result = pTable->CreateCommandPool(swapchainMapping->device, &cmdPoolCreateInfo,
        nullptr, &cmdPool);
      g_messageLog.LogInfo("VulkanOverlay", "Create command pool");
      if (result != VK_SUCCESS)
      {
        return VK_NULL_HANDLE;
      }

      swapchainMapping->queueMappings.insert(std::make_pair(queueFamilyIndex, SwapchainQueueMapping{ queue, graphicsQueue, cmdPool }));
      queueMapping = &swapchainMapping->queueMappings.at(queueFamilyIndex);
    }

  if (imageMapping == nullptr)
  {
    SwapchainImageMapping im = {};
    im.imageIndex = imageIndex;
    CreateImageMapping(pTable, setDeviceLoaderDataFuncPtr, swapchainMapping,
      queueMapping, queueFamilyIndex, &im);
    if (im.commandBuffer == VK_NULL_HANDLE)
    {
      return VK_NULL_HANDLE;
    }

      queueMapping->imageMappings.push_back(im);
      imageMapping = &queueMapping->imageMappings.back();
    }

    VkResult result = UpdateUniformBuffer(pTable, swapchainMapping);
    if (result != VK_SUCCESS)
    {
      g_messageLog.LogError("OnPresent", "Failed to update uniform buffer.");
      return VK_NULL_HANDLE;
    }
  }

  overlayBitmap_->DrawOverlay();

  auto textureData = overlayBitmap_->GetBitmapDataRead();
  auto& overlayImageIdx = swapchainMapping->overlayImages[swapchainMapping->nextOverlayImage];

  uint32_t bufferSize;
  if (textureData.dataPtr && textureData.size)
  {
    bufferSize = max(textureData.size, swapchainMapping->lastOverlayBufferSize);
    void* data;
    VkResult result = pTable->MapMemory(swapchainMapping->device, overlayImageIdx.overlayHostMemory,
      0, bufferSize, 0, &data);
    if (result != VK_SUCCESS)
    {
      return VK_NULL_HANDLE;
    }
    memcpy(data, textureData.dataPtr, bufferSize);
    pTable->UnmapMemory(swapchainMapping->device, overlayImageIdx.overlayHostMemory);
    swapchainMapping->lastOverlayBufferSize = textureData.size;
  }

  overlayBitmap_->UnlockBitmapData();

  if (!overlayImageIdx.CopyBuffer(swapchainMapping->device, bufferSize, pTable, setDeviceLoaderDataFuncPtr,
    swapchainMapping->queueMappings[queueFamilyIndex].commandPool, queue))
  {
    return VK_NULL_HANDLE;
  }
  overlayImageIdx.valid = true;

  swapchainMapping->nextOverlayImage = 1 - swapchainMapping->nextOverlayImage;
  if (!swapchainMapping->overlayImages[swapchainMapping->nextOverlayImage].valid)
  {
    return VK_NULL_HANDLE;
  }

  cmdBuffer = imageMapping->commandBuffer[swapchainMapping->nextOverlayImage];

  auto position = overlayBitmap_->GetScreenPos();
  if (swapchainMapping->overlayRect.offset.x != position.x
    || swapchainMapping->overlayRect.offset.y != position.y)
  {
    // Position changed.
    VkResult result = UpdateOverlayPosition(pTable, swapchainMapping, overlayBitmap_->GetScreenPos());
    if (result != VK_SUCCESS)
    {
      g_messageLog.LogError("OnPresent", "Failed to update overlay position.");
      return VK_NULL_HANDLE;
    }

    remainingRecordRenderPassUpdates_ = static_cast<int>(swapchainMapping->imageData.size());
  }

  // Make sure that the render pass of every image in the image mapping gets updated.
  if (remainingRecordRenderPassUpdates_ > 0)
  {
    // Record render pass to update the viewport changes.
    VkResult result = RecordRenderPass(pTable, setDeviceLoaderDataFuncPtr,
      swapchainMapping, queueMapping, queueFamilyIndex, imageMapping);
    if (result != VK_SUCCESS)
    {
      g_messageLog.LogError("OnPresent", "Failed to record render pass.");
      return VK_NULL_HANDLE;
    }
    remainingRecordRenderPassUpdates_--;
  }

  std::vector<VkPipelineStageFlags> pPipelineStageFlags(waitSemaphoreCount + 1);
  std::vector<VkSemaphore> waitSemaphores(waitSemaphoreCount + 1);
  VkPipelineStageFlagBits pipelineStageFlagBit = graphicsQueue
    ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
  {
    pPipelineStageFlags[i] = pipelineStageFlagBit;
    waitSemaphores[i] = pWaitSemaphores[i];
  }

  pPipelineStageFlags[waitSemaphoreCount] = pipelineStageFlagBit;
  waitSemaphores[waitSemaphoreCount] =
    swapchainMapping->overlayImages[swapchainMapping->nextOverlayImage].overlayCopySemaphore;
  ++waitSemaphoreCount;

  VkSubmitInfo submitInfo = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, waitSemaphoreCount, waitSemaphores.data(),
    pPipelineStageFlags.data(),    1,       &cmdBuffer,         1,
    &imageMapping->semaphore };

  VkResult result = pTable->QueueSubmit(queueMapping->queue, 1, &submitInfo, VK_NULL_HANDLE);
  if (result != VK_SUCCESS)
  {
    return VK_NULL_HANDLE;
  }
  return imageMapping->semaphore;
}

VkSemaphore Rendering::OnPresent(VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
  VkQueue queue, uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
  VkSwapchainKHR swapchain, uint32_t imageIndex,
  uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores)
{
  if (!pipelineInitialized)
    return VK_NULL_HANDLE;

  const auto swapchainMapping = swapchainMappings_.Get(swapchain);

  return Present(pTable, setDeviceLoaderDataFuncPtr, queue, queueFamilyIndex, queueFlags,
    imageIndex, waitSemaphoreCount, pWaitSemaphores, swapchainMapping);
}

VkSemaphore Rendering::OnSubmitFrameCompositor(VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr, VkQueue queue,
  uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
  uint32_t imageIndex)
{
  if (!pipelineInitialized)
    return VK_NULL_HANDLE;

  // synchronizing should be done via the compositor
  return Present(pTable, setDeviceLoaderDataFuncPtr, queue, queueFamilyIndex, queueFlags,
    imageIndex, 0, nullptr, &compositorSwapchainMapping_);
}

VkResult Rendering::RecordRenderPass(VkLayerDispatchTable * pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
  SwapchainMapping * sm, SwapchainQueueMapping * qm, uint32_t queueFamilyIndex,
  SwapchainImageMapping * im)
{
  SwapchainImageData& sid = sm->imageData[im->imageIndex];
  for (int i = 0; i < 2; ++i)
  {
    VkDevice cmdBuf = static_cast<VkDevice>(static_cast<void*>(im->commandBuffer[i]));
    setDeviceLoaderDataFuncPtr(sm->device, cmdBuf);

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
      VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr };

    VkResult result = pTable->BeginCommandBuffer(im->commandBuffer[i], &cmdBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
      g_messageLog.LogError("CreateImageMapping", "Failed to begin command buffer." + std::to_string(static_cast<int>(result)));
      pTable->FreeCommandBuffers(sm->device, qm->commandPool, 1, &im->commandBuffer[i]);
      im->commandBuffer[i] = VK_NULL_HANDLE;
      return result;
    }

    if (qm->isGraphicsQueue)
    {
      VkRenderPassBeginInfo rpBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        sm->renderPass,
        sid.framebuffer,
        { { 0, 0 }, sm->extent },
        0,
        nullptr };

      pTable->CmdBeginRenderPass(im->commandBuffer[i], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      pTable->CmdBindPipeline(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
        sm->gfxPipeline);
      pTable->CmdBindDescriptorSets(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
        sm->gfxPipelineLayout, 0, 1,
        &sm->overlayImages[i].descriptorSet, 0, 0);

      VkViewport viewport;
      viewport.maxDepth = 1.0f;
      viewport.minDepth = 0.0f;
      viewport.x = static_cast<float>(sm->overlayRect.offset.x);
      viewport.y = static_cast<float>(sm->overlayRect.offset.y);
      viewport.width = static_cast<float>(sm->overlayRect.extent.width);
      viewport.height = static_cast<float>(sm->overlayRect.extent.height);
      pTable->CmdSetViewport(im->commandBuffer[i], 0, 1, &viewport);

      pTable->CmdDraw(im->commandBuffer[i], 3, 1, 0, 0);
      pTable->CmdEndRenderPass(im->commandBuffer[i]);
    }
    else
    {
      VkImageMemoryBarrier presentToComputeBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
      presentToComputeBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      presentToComputeBarrier.dstAccessMask =
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
      presentToComputeBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      presentToComputeBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      presentToComputeBarrier.srcQueueFamilyIndex = queueFamilyIndex;
      presentToComputeBarrier.dstQueueFamilyIndex = queueFamilyIndex;
      presentToComputeBarrier.image = sm->imageData[im->imageIndex].image;
      presentToComputeBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

      VkImageMemoryBarrier computeToPresentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
      computeToPresentBarrier.srcAccessMask =
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
      computeToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      computeToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      computeToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      computeToPresentBarrier.srcQueueFamilyIndex = queueFamilyIndex;
      computeToPresentBarrier.dstQueueFamilyIndex = queueFamilyIndex;
      computeToPresentBarrier.image = sm->imageData[im->imageIndex].image;
      computeToPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

      pTable->CmdPipelineBarrier(im->commandBuffer[i], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &presentToComputeBarrier);
      pTable->CmdBindPipeline(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_COMPUTE,
        sm->computePipeline);
      pTable->CmdBindDescriptorSets(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_COMPUTE,
        sm->computePipelineLayout, 0, 1,
        &sm->imageData[im->imageIndex].computeDescriptorSet[i], 0, 0);
      pTable->CmdDispatch(im->commandBuffer[i], sm->overlayRect.extent.width / 16,
        sm->overlayRect.extent.height / 16, 1);
      pTable->CmdPipelineBarrier(im->commandBuffer[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &computeToPresentBarrier);
    }

    result = pTable->EndCommandBuffer(im->commandBuffer[i]);
    if (result != VK_SUCCESS)
    {
      pTable->FreeCommandBuffers(sm->device, qm->commandPool, 1, &im->commandBuffer[i]);
      im->commandBuffer[i] = VK_NULL_HANDLE;
    }
  }

  return VK_SUCCESS;
}

void Rendering::CreateImageMapping(VkLayerDispatchTable* pTable,
  PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
  SwapchainMapping* sm, SwapchainQueueMapping* qm, uint32_t queueFamilyIndex,
  SwapchainImageMapping* im)
{
  SwapchainImageData& sid = sm->imageData[im->imageIndex];

  VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };

  VkResult result = pTable->CreateSemaphore(sm->device, &semaphoreCreateInfo, nullptr, &im->semaphore);
  if (result != VK_SUCCESS)
  {
    return;
  }

  VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, qm->commandPool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, 2 };

  result = pTable->AllocateCommandBuffers(sm->device, &cmdBufferAllocateInfo, im->commandBuffer);
  if (result != VK_SUCCESS)
  {
    return;
  }

  result = RecordRenderPass(pTable, setDeviceLoaderDataFuncPtr,
    sm, qm, queueFamilyIndex, im);
}

VkShaderModule Rendering::CreateShaderModuleFromFile(VkDevice device, VkLayerDispatchTable* pTable,
  const std::wstring& fileName) const
{
  std::ifstream shaderFile(fileName, std::ios::ate | std::ios::binary);
  if (!shaderFile.is_open())
  {
    return VK_NULL_HANDLE;
  }

  size_t fileSize = static_cast<::size_t>(shaderFile.tellg());
  shaderFile.seekg(0);
  std::vector<char> shaderBuffer(fileSize);
  shaderFile.read(shaderBuffer.data(), fileSize);
  shaderFile.close();

  return CreateShaderModuleFromBuffer(device, pTable, shaderBuffer.data(),
    static_cast<uint32_t>(fileSize));
}

VkShaderModule Rendering::CreateShaderModuleFromBuffer(VkDevice device,
  VkLayerDispatchTable* pTable,
  const char* shaderBuffer,
  uint32_t bufferSize) const
{
  VkShaderModuleCreateInfo smCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                                           bufferSize,
                                           reinterpret_cast<const uint32_t*>(shaderBuffer) };

  VkShaderModule shaderModule;
  VkResult result = pTable->CreateShaderModule(device, &smCreateInfo, nullptr, &shaderModule);
  if (result != VK_SUCCESS)
  {
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}
