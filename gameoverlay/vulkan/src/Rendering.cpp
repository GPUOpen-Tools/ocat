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

#include "Recording\Capturing.h"

void Rendering::OnDestroySwapchain(VkDevice device, VkLayerDispatchTable* pTable,
                                   VkSwapchainKHR swapchain)
{
    SwapchainMapping* sm = swapchainMappings_.Get(swapchain);
    if (sm == nullptr)
    {
        return;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (sm->overlayImages[i].bufferView != VK_NULL_HANDLE)
        {
            pTable->DestroyBufferView(device, sm->overlayImages[i].bufferView, nullptr);
        }

        if (sm->overlayImages[i].overlayHostMemory != VK_NULL_HANDLE)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
        }

        if (sm->overlayImages[i].overlayHostBuffer != VK_NULL_HANDLE)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
        }

        if (sm->overlayImages[i].overlayMemory != VK_NULL_HANDLE)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
        }

        if (sm->overlayImages[i].overlayBuffer != VK_NULL_HANDLE)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
        }

        if (sm->overlayImages[i].overlayCopySemaphore != VK_NULL_HANDLE)
        {
            pTable->DestroySemaphore(device, sm->overlayImages[i].overlayCopySemaphore, nullptr);
        }

        if (sm->overlayImages[i].commandBufferFence[0] != VK_NULL_HANDLE)
        {
            pTable->DestroyFence(device, sm->overlayImages[i].commandBufferFence[0], nullptr);
        }

        if (sm->overlayImages[i].commandBufferFence[1] != VK_NULL_HANDLE)
        {
            pTable->DestroyFence(device, sm->overlayImages[i].commandBufferFence[1], nullptr);
        }
    }

    if (sm->renderPass != VK_NULL_HANDLE)
    {
        pTable->DestroyRenderPass(device, sm->renderPass, nullptr);
    }

    if (sm->gfxPipelineLayout != VK_NULL_HANDLE)
    {
        pTable->DestroyPipelineLayout(device, sm->gfxPipelineLayout, nullptr);
    }

    if (sm->gfxPipeline != VK_NULL_HANDLE)
    {
        pTable->DestroyPipeline(device, sm->gfxPipeline, nullptr);
    }

    if (sm->computePipeline != VK_NULL_HANDLE)
    {
        pTable->DestroyPipeline(device, sm->computePipeline, nullptr);
    }

    for (auto& qm : sm->queueMappings)
    {
        for (auto& im : qm.imageMappings)
        {
            pTable->DestroySemaphore(device, im.semaphore, nullptr);
        }

        pTable->DestroyCommandPool(device, qm.commandPool, nullptr);
    }

    sm->ClearImageData(pTable);

    swapchainMappings_.Remove(swapchain);
    delete sm;
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

Rendering::Rendering(const std::string& dir) : shaderDirectory_(dir) {}
void Rendering::OnCreateSwapchain(
    VkDevice device, VkLayerDispatchTable* pTable,
    const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    VkSwapchainKHR swapchain, VkFormat format, const VkExtent2D& extent)
{
    SwapchainMapping* sm = new SwapchainMapping{ device, format, VK_FORMAT_B8G8R8A8_UNORM, extent };
    swapchainMappings_.Add(swapchain, sm);

    static bool overlayInitialized = false;
    if (!overlayInitialized)
    {
        gameoverlay::InitLogging("VulkanOverlay");
        gameoverlay::InitCapturing();

        try
        {
            textRenderer_.reset(
                new TextRenderer{ static_cast<int>(extent.width), static_cast<int>(extent.height) });
        }
        catch (const std::exception&)
        {
            return;
        }

        overlayInitialized = true;
    }
    textRenderer_->Resize(extent.width, extent.height);

    sm->overlayFormat = textRenderer_->GetVKFormat();

    const auto screenPos = textRenderer_->GetScreenPos();
    sm->overlayRect.offset.x = screenPos.x;
    sm->overlayRect.offset.y = screenPos.y;
    sm->overlayRect.extent.width = textRenderer_->GetFullWidth();
    sm->overlayRect.extent.height = textRenderer_->GetFullHeight();

    for (int i = 0; i < 2; ++i)
    {
        VkBufferCreateInfo overlayHostBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        overlayHostBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        overlayHostBufferInfo.size = sm->overlayRect.extent.width * sm->overlayRect.extent.height * 4;
        overlayHostBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VkResult result = pTable->CreateBuffer(device, &overlayHostBufferInfo, nullptr,
                                               &sm->overlayImages[i].overlayHostBuffer);
        if (result != VK_SUCCESS)
        {
            return;
        }

        VkMemoryRequirements memoryRequirements;
        pTable->GetBufferMemoryRequirements(device, sm->overlayImages[i].overlayHostBuffer,
                                            &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(
            physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        result = pTable->AllocateMemory(device, &memoryAllocateInfo, nullptr,
                                        &sm->overlayImages[i].overlayHostMemory);
        if (result != VK_SUCCESS)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        result = pTable->BindBufferMemory(device, sm->overlayImages[i].overlayHostBuffer,
                                          sm->overlayImages[i].overlayHostMemory, 0);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        VkBufferCreateInfo overlayBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        overlayBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        overlayBufferInfo.size = sm->overlayRect.extent.width * sm->overlayRect.extent.height * 4;
        overlayBufferInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

        result = pTable->CreateBuffer(device, &overlayBufferInfo, nullptr, &sm->overlayImages[i].overlayBuffer);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        pTable->GetBufferMemoryRequirements(device, sm->overlayImages[i].overlayBuffer,
                                            &memoryRequirements);
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex =
            GetMemoryTypeIndex(physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        result = pTable->AllocateMemory(device, &memoryAllocateInfo, nullptr,
                                        &sm->overlayImages[i].overlayMemory);
        if (result != VK_SUCCESS)
        {
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        result = pTable->BindBufferMemory(device, sm->overlayImages[i].overlayBuffer,
                                          sm->overlayImages[i].overlayMemory, 0);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        VkBufferViewCreateInfo bvCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
        bvCreateInfo.buffer = sm->overlayImages[i].overlayBuffer;
        bvCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        bvCreateInfo.range = VK_WHOLE_SIZE;

        result = pTable->CreateBufferView(sm->device, &bvCreateInfo, nullptr,
                                          &sm->overlayImages[i].bufferView);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            return;
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };

        result = pTable->CreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sm->overlayImages[i].overlayCopySemaphore);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            pTable->DestroyBufferView(device, sm->overlayImages[i].bufferView, nullptr);
            return;
        }
#if _DEBUG
        VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
        result = pTable->CreateFence(device, &createInfo, nullptr, &sm->overlayImages[i].commandBufferFence[0]);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            pTable->DestroyBufferView(device, sm->overlayImages[i].bufferView, nullptr);
            return;
        }

        result = pTable->CreateFence(device, &createInfo, nullptr, &sm->overlayImages[i].commandBufferFence[1]);
        if (result != VK_SUCCESS)
        {
            pTable->FreeMemory(device, sm->overlayImages[i].overlayMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayBuffer, nullptr);
            pTable->FreeMemory(device, sm->overlayImages[i].overlayHostMemory, nullptr);
            pTable->DestroyBuffer(device, sm->overlayImages[i].overlayHostBuffer, nullptr);
            pTable->DestroyBufferView(device, sm->overlayImages[i].bufferView, nullptr);
            return;
        }
#endif
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

    VkResult result = pTable->CreateRenderPass(sm->device, &rpCreateInfo, nullptr, &sm->renderPass);
    if (result != VK_SUCCESS)
    {
        return;
    }
}

void Rendering::SwapchainMapping::ClearImageData(VkLayerDispatchTable* pTable)
{
    for (auto& id : imageData)
    {
        if (id.view != VK_NULL_HANDLE)
        {
            pTable->DestroyImageView(device, id.view, nullptr);
        }

        if (id.framebuffer != VK_NULL_HANDLE)
        {
            pTable->DestroyFramebuffer(device, id.framebuffer, nullptr);
        }
    }
    imageData.clear();

    if (descriptorPool != VK_NULL_HANDLE)
    {
        pTable->DestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    if (computePipelineLayout != VK_NULL_HANDLE)
    {
        pTable->DestroyPipelineLayout(device, computePipelineLayout, nullptr);
    }
}

void Rendering::OnGetSwapchainImages(VkLayerDispatchTable* pTable, VkSwapchainKHR swapchain,
                                     uint32_t imageCount, VkImage* images)
{
    SwapchainMapping* sm = swapchainMappings_.Get(swapchain);
    if (sm == nullptr || sm->renderPass == VK_NULL_HANDLE)
    {
        return;
    }

    if (sm->imageData.size() != 0)
    {
        sm->ClearImageData(pTable);
    }

    sm->imageData.resize(imageCount);

    for (uint32_t i = 0; i < imageCount; ++i)
    {
        SwapchainImageData& sid = sm->imageData[i];

        sid.image = images[i];

        VkImageViewCreateInfo ivCreateInfo = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            images[i],
            VK_IMAGE_VIEW_TYPE_2D,
            sm->format,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };

        VkResult result = pTable->CreateImageView(sm->device, &ivCreateInfo, nullptr, &sid.view);
        if (result != VK_SUCCESS)
        {
            sm->ClearImageData(pTable);
            return;
        }

        VkFramebufferCreateInfo fbCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                nullptr,
                                                0,
                                                sm->renderPass,
                                                1,
                                                &sid.view,
                                                sm->extent.width,
                                                sm->extent.height,
                                                1 };

        result = pTable->CreateFramebuffer(sm->device, &fbCreateInfo, nullptr, &sid.framebuffer);
        if (result != VK_SUCCESS)
        {
            sm->ClearImageData(pTable);
            return;
        }
    }

    VkDescriptorPoolSize imageStoreDescriptorPoolSizes[2] = { {}, {} };
    imageStoreDescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageStoreDescriptorPoolSizes[0].descriptorCount = 2 * imageCount;

    imageStoreDescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    imageStoreDescriptorPoolSizes[1].descriptorCount = 2 * imageCount + 2;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptorPoolCreateInfo.poolSizeCount = 2;
    descriptorPoolCreateInfo.pPoolSizes = imageStoreDescriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = 2 * imageCount + 2;

    VkResult result = pTable->CreateDescriptorPool(sm->device, &descriptorPoolCreateInfo, nullptr,
                                                   &sm->descriptorPool);
    if (result != VK_SUCCESS)
    {
        return;
    }

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[2] = { {}, {} };
    descriptorSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    descriptorSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorSetLayoutBinding[0].binding = 0;
    descriptorSetLayoutBinding[0].descriptorCount = 1;
    descriptorSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorSetLayoutBinding[1].binding = 1;
    descriptorSetLayoutBinding[1].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBinding;
    descriptorSetLayoutCreateInfo.bindingCount = 2;

    VkDescriptorSetLayout computeDescriptorSetLayout;
    result = pTable->CreateDescriptorSetLayout(sm->device, &descriptorSetLayoutCreateInfo, nullptr,
                                               &computeDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        return;
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
        return;
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
            return;
        }

        result = pTable->AllocateDescriptorSets(sm->device, &descriptorSetAllocateInfo,
                                                &sid.computeDescriptorSet[1]);
        if (result != VK_SUCCESS)
        {
            pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);
            return;
        }

        VkDescriptorImageInfo descriptorImageInfo = {};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptorImageInfo.imageView = sid.view;

        VkWriteDescriptorSet writeDescriptorSet[2] = {
            {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET} };
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

        pTable->UpdateDescriptorSets(sm->device, 2, writeDescriptorSet, 0, NULL);

        writeDescriptorSet[0].dstSet = sid.computeDescriptorSet[1];
        writeDescriptorSet[0].pTexelBufferView = &sm->overlayImages[1].bufferView;
        writeDescriptorSet[1].dstSet = sid.computeDescriptorSet[1];

        pTable->UpdateDescriptorSets(sm->device, 2, writeDescriptorSet, 0, NULL);
    }

    VkSpecializationMapEntry specializationEntries[4] = {
        {0, 0, sizeof(int32_t)},
        {1, sizeof(int32_t), sizeof(int32_t)},
        {2, 2 * sizeof(int32_t), sizeof(uint32_t)},
        {3, 2 * sizeof(int32_t) + sizeof(uint32_t), sizeof(uint32_t)} };

    VkSpecializationInfo specializationInfo;
    specializationInfo.dataSize = 2 * sizeof(int32_t) + 2 * sizeof(uint32_t);
    specializationInfo.mapEntryCount = 4;
    specializationInfo.pMapEntries = specializationEntries;
    specializationInfo.pData = &sm->overlayRect;

    VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    computeShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageCreateInfo.module =
        CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + "comp.spv");
    computeShaderStageCreateInfo.pName = "main";
    computeShaderStageCreateInfo.pSpecializationInfo = &specializationInfo;

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    computePipelineCreateInfo.layout = sm->computePipelineLayout;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.stage = computeShaderStageCreateInfo;

    pTable->CreateComputePipelines(sm->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
                                   &sm->computePipeline);

    pTable->DestroyShaderModule(sm->device, computeShaderStageCreateInfo.module, nullptr);
    pTable->DestroyDescriptorSetLayout(sm->device, computeDescriptorSetLayout, nullptr);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO} };

    VkShaderModule shaderModules[2];

    shaderModules[0] = CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + "vert.spv");
    if (shaderModules[0] == VK_NULL_HANDLE)
    {
        return;
    }

    shaderModules[1] = CreateShaderModuleFromFile(sm->device, pTable, shaderDirectory_ + "frag.spv");
    if (shaderModules[1] == VK_NULL_HANDLE)
    {
        pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
        return;
    }

    VkSpecializationMapEntry gfxSpecializationEntries[4] = { {0, 0, sizeof(int32_t)},
                                                            {1, sizeof(int32_t), sizeof(int32_t)},
                                                            {2, 2*sizeof(int32_t), sizeof(int32_t)}};

    specializationInfo;
    specializationInfo.dataSize = 3 * sizeof(int32_t);
    specializationInfo.mapEntryCount = 3;
    specializationInfo.pMapEntries = gfxSpecializationEntries;
    specializationInfo.pData = &sm->overlayRect;

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = shaderModules[0];
    shaderStages[0].pName = "main";
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = shaderModules[1];
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = &specializationInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // static viewport for testing
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

    VkDescriptorSetLayoutBinding gfxDescriptorSetLayoutBinding = {};
    gfxDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    gfxDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    gfxDescriptorSetLayoutBinding.binding = 0;
    gfxDescriptorSetLayoutBinding.descriptorCount = 1;

    descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.pBindings = &gfxDescriptorSetLayoutBinding;
    descriptorSetLayoutCreateInfo.bindingCount = 1;

    VkDescriptorSetLayout gfxDescriptorSetLayout;
    result = pTable->CreateDescriptorSetLayout(sm->device, &descriptorSetLayoutCreateInfo, nullptr,
                                               &gfxDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
        pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
        return;
    }

    pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &gfxDescriptorSetLayout;

    result = pTable->CreatePipelineLayout(sm->device, &pipelineLayoutCreateInfo, nullptr,
                                          &sm->gfxPipelineLayout);
    if (result != VK_SUCCESS)
    {
        pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
        pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
        pTable->DestroyDescriptorSetLayout(sm->device, gfxDescriptorSetLayout, nullptr);
        return;
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
            return;
        }

        VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptorSet.dstSet = sm->overlayImages[i].descriptorSet;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pTexelBufferView = &sm->overlayImages[i].bufferView;
        writeDescriptorSet.descriptorCount = 1;

        pTable->UpdateDescriptorSets(sm->device, 1, &writeDescriptorSet, 0, NULL);
    }

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

    result = pTable->CreateGraphicsPipelines(sm->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                                             nullptr, &sm->gfxPipeline);

    pTable->DestroyShaderModule(sm->device, shaderModules[0], nullptr);
    pTable->DestroyShaderModule(sm->device, shaderModules[1], nullptr);
    pTable->DestroyDescriptorSetLayout(sm->device, gfxDescriptorSetLayout, nullptr);

    if (result != VK_SUCCESS)
    {
        return;
    }
}

bool Rendering::OverlayImageData::CopyBuffer(VkDevice device, VkDeviceSize size,
                                             VkLayerDispatchTable* pTable, 
                                             PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
                                             VkCommandPool commandPool, VkQueue queue)
{
    if (commandBuffer[commandBufferIndex] != VK_NULL_HANDLE)
    {
#if _DEBUG
        pTable->WaitForFences(device, 1, &commandBufferFence[commandBufferIndex], VK_TRUE, UINT64_MAX);
        pTable->ResetFences(device, 1, &commandBufferFence[commandBufferIndex]);
#endif
        pTable->FreeCommandBuffers(device, commandPool, 1, &commandBuffer[commandBufferIndex]);
    }

    VkCommandBufferAllocateInfo cmdBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocateInfo.commandPool = commandPool;
    cmdBufferAllocateInfo.commandBufferCount = 1;

    VkResult result = pTable->AllocateCommandBuffers(device, &cmdBufferAllocateInfo, &commandBuffer[commandBufferIndex]);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkDevice cmdBuf = static_cast<VkDevice>(static_cast<void*>(commandBuffer[commandBufferIndex]));
    setDeviceLoaderDataFuncPtr(device, cmdBuf);

    VkCommandBufferBeginInfo cmdBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = pTable->BeginCommandBuffer(commandBuffer[commandBufferIndex], &cmdBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        pTable->FreeCommandBuffers(device, commandPool, 1, &commandBuffer[commandBufferIndex]);
        return false;
    }

    VkBufferCopy bufferCopy = {};
    bufferCopy.size = size;
    pTable->CmdCopyBuffer(commandBuffer[commandBufferIndex], overlayHostBuffer, overlayBuffer, 1, &bufferCopy);

    result = pTable->EndCommandBuffer(commandBuffer[commandBufferIndex]);
    if (result != VK_SUCCESS)
    {
        pTable->FreeCommandBuffers(device, commandPool, 1, &commandBuffer[commandBufferIndex]);
        return false;
    }

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer[commandBufferIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &overlayCopySemaphore;

#if _DEBUG
    result = pTable->QueueSubmit(queue, 1, &submitInfo, commandBufferFence[commandBufferIndex]);
#else
    result = pTable->QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
#endif
    if (result != VK_SUCCESS)
    {
        pTable->FreeCommandBuffers(device, commandPool, 1, &commandBuffer[commandBufferIndex]);
        return false;
    }

    commandBufferIndex = 1 - commandBufferIndex;

    return true;
}

VkSemaphore Rendering::OnPresent(VkLayerDispatchTable* pTable,
                                 PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
                                 VkQueue queue, uint32_t queueFamilyIndex, VkQueueFlags queueFlags,
                                 VkSwapchainKHR swapchain, uint32_t imageIndex,
                                 uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores)
{
    if ((queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0)
    {
        return VK_NULL_HANDLE;
    }

    int32_t graphicsQueue = queueFlags & VK_QUEUE_GRAPHICS_BIT;

    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    auto swapchainMapping = swapchainMappings_.Get(swapchain);
    SwapchainImageMapping* imageMapping = nullptr;
    SwapchainQueueMapping* queueMapping = nullptr;

    for (auto& qm : swapchainMapping->queueMappings)
    {
        if (qm.queue == queue)
        {
            queueMapping = &qm;
            for (auto& im : qm.imageMappings)
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
                                                         nullptr, 0, queueFamilyIndex };
            VkCommandPool cmdPool;
            VkResult result = pTable->CreateCommandPool(swapchainMapping->device, &cmdPoolCreateInfo,
                                                        nullptr, &cmdPool);
            if (result != VK_SUCCESS)
            {
                return VK_NULL_HANDLE;
            }

            swapchainMapping->queueMappings.push_back({ queue, graphicsQueue, cmdPool });
            queueMapping = &swapchainMapping->queueMappings.back();
        }

        if (imageMapping == nullptr)
        {
            SwapchainImageMapping im = {};
            im.imageIndex = imageIndex;
            CreateImageMapping(pTable, setDeviceLoaderDataFuncPtr, swapchain, swapchainMapping,
                               queueMapping, queueFamilyIndex, &im);
            if (im.commandBuffer == VK_NULL_HANDLE)
            {
                return VK_NULL_HANDLE;
            }

            queueMapping->imageMappings.push_back(im);
            imageMapping = &queueMapping->imageMappings.back();
        }
    }

    textRenderer_->DrawOverlay();

    auto textureData = textRenderer_->GetBitmapDataRead();
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

    textRenderer_->UnlockBitmapData();

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

void Rendering::CreateImageMapping(VkLayerDispatchTable* pTable,
                                   PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr,
                                   VkSwapchainKHR swapchain, SwapchainMapping* sm,
                                   SwapchainQueueMapping* qm, uint32_t queueFamilyIndex,
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

    for (int i = 0; i < 2; ++i)
    {
        VkDevice cmdBuf = static_cast<VkDevice>(static_cast<void*>(im->commandBuffer[i]));
        setDeviceLoaderDataFuncPtr(sm->device, cmdBuf);

        VkCommandBufferBeginInfo cmdBufferBeginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr };

        result = pTable->BeginCommandBuffer(im->commandBuffer[i], &cmdBufferBeginInfo);
        if (result != VK_SUCCESS)
        {
            pTable->FreeCommandBuffers(sm->device, qm->commandPool, 1, &im->commandBuffer[i]);
            im->commandBuffer[i] = VK_NULL_HANDLE;
            return;
        }

        if (qm->isGraphicsQueue)
        {
            VkRenderPassBeginInfo rpBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                 nullptr,
                                                 sm->renderPass,
                                                 sid.framebuffer,
                                                 {{0, 0}, sm->extent},
                                                 0,
                                                 nullptr };

            pTable->CmdBeginRenderPass(im->commandBuffer[i], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            pTable->CmdBindPipeline(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    sm->gfxPipeline);
            pTable->CmdBindDescriptorSets(im->commandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                          sm->gfxPipelineLayout, 0, 1,
                                          &sm->overlayImages[i].descriptorSet, 0, 0);
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
}

VkShaderModule Rendering::CreateShaderModuleFromFile(VkDevice device, VkLayerDispatchTable* pTable,
                                                     const std::string& fileName) const
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