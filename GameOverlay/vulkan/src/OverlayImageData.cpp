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

#include "OverlayImageData.h"
#include "Logging/MessageLog.h"

bool OverlayImageData::CopyBuffer(VkDevice device, VkDeviceSize size, 
  VkLayerDispatchTable * pTable, PFN_vkSetDeviceLoaderData setDeviceLoaderDataFuncPtr, 
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
    g_messageLog.LogError("CopyBuffer", "Failed to begin command buffer." + std::to_string(static_cast<int>(result)));
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
    if (result == VK_ERROR_INITIALIZATION_FAILED)
    {
      g_messageLog.LogError("CopyBuffer", "Queue Submit: Initialization failed.");
    }
    pTable->FreeCommandBuffers(device, commandPool, 1, &commandBuffer[commandBufferIndex]);
    return false;
  }

  commandBufferIndex = 1 - commandBufferIndex;
  return true;
}
