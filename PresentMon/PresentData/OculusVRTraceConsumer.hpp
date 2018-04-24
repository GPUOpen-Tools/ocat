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

struct __declspec(uuid("{553787FC-D3D7-4F5E-ACB2-1597C7209B3C}")) OCULUSVR_PROVIDER_GUID_HOLDER;
static const auto OCULUSVR_PROVIDER_GUID = __uuidof(OCULUSVR_PROVIDER_GUID_HOLDER);

struct OculusVREvent
{
  uint64_t QpcTime;
  uint32_t ProcessId;

  uint64_t AppRenderStart;
  uint64_t AppRenderEnd;
  uint64_t ReprojectionStart;
  uint64_t ReprojectionEnd;
  // usually fixed time before VSync
  uint64_t EndSpinWait;
  bool AppMiss;
  bool WarpMiss;
  bool Completed;

  OculusVREvent(EVENT_HEADER const& hdr);
};

struct OculusVRTraceConsumer
{
  OculusVRTraceConsumer(bool simple) : mSimpleMode(simple)
  {}

  const bool mSimpleMode;

  uint32_t mProcessId = 0;

  std::mutex mMutex;

  // completed SteamVR events -> either displayed or discarded
  std::vector<std::shared_ptr<OculusVREvent>> mCompletedEvents;

  std::shared_ptr<OculusVREvent> mActiveEvent;

  std::map<uint64_t, std::shared_ptr<OculusVREvent>> mPresentsByFrameId;

  std::queue<std::shared_ptr<OculusVREvent>>  mPresentsCompositorStart;
  std::queue<std::shared_ptr<OculusVREvent>>  mPresentsCompositorVSyncIndicator;
  std::queue<std::shared_ptr<OculusVREvent>>  mPresentsCompositorReprojection;

  bool DequeueEvents(std::vector<std::shared_ptr<OculusVREvent>>& outEvents)
  {
    if (mCompletedEvents.empty()) {
      return false;
    }

    auto lock = scoped_lock(mMutex);
    outEvents.swap(mCompletedEvents);
    return true;
  }

  void CompleteEvent(std::shared_ptr<OculusVREvent> p);
};

void HandleOculusVREvent(EVENT_RECORD* pEventRecord, OculusVRTraceConsumer* svrConsumer);