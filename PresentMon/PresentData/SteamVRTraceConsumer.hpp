//
// Copyright(c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all
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

#include "PresentMonTraceConsumer.hpp"

struct __declspec(uuid("{8C8F13B1-60EB-4B6A-A433-DE86104115AC}")) STEAMVR_PROVIDER_GUID_HOLDER;
static const auto STEAMVR_PROVIDER_GUID = __uuidof(STEAMVR_PROVIDER_GUID_HOLDER);

struct SteamVREvent
{
  uint64_t QpcTime;
  uint32_t ProcessId;
  uint64_t FrameId;

  uint64_t AppRenderStart;
  uint64_t AppRenderEnd;
  uint64_t ReprojectionStart;
  uint64_t ReprojectionEnd;
  uint64_t TimeStampSinceLastVSync;
  double MsSinceLastVSync;
  bool AppMiss;
  bool WarpMiss;
  bool AppRenderingCompleted;
  bool Completed;

  SteamVREvent(EVENT_HEADER const& hdr);
};

struct SteamVRTraceConsumer
{
  SteamVRTraceConsumer(bool simple) : mSimpleMode(simple)
  {}

  const bool mSimpleMode;

  std::mutex mMutex;

  // completed SteamVR events -> either displayed or discarded
  std::vector<std::shared_ptr<SteamVREvent>> mCompletedEvents;

  // Connect App frames with Compositor frames
  std::map<uint64_t, std::shared_ptr<SteamVREvent>> mPresentsByFrameId;

  // app submit to compositor chain
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorSubmitLeft;
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorSubmitRight;
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorSubmitEnd;

  // compositor event chain
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorStart;
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorReprojection;

  // SteamVR
  std::queue<std::pair<uint64_t, std::shared_ptr<SteamVREvent>>>  mPresentsCompositorLastTextureIndex;
  std::queue<std::shared_ptr<SteamVREvent>>  mPresentsCompositorVSyncIndicator;

  uint32_t mProcessId = 0;

  bool DequeueEvents(std::vector<std::shared_ptr<SteamVREvent>>& outEvents)
  {
    if (mCompletedEvents.empty()) {
      return false;
    }

    auto lock = scoped_lock(mMutex);
    outEvents.swap(mCompletedEvents);
    return true;
  }

  void CompleteEvent(std::shared_ptr<SteamVREvent> p);
};

void HandleSteamVREvent(EVENT_RECORD* pEventRecord, SteamVRTraceConsumer* svrConsumer);