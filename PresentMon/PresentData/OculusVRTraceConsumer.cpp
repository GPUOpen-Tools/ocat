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

#include <codecvt>
#include <sstream>

#include "OculusVRTraceConsumer.hpp"
#include "TraceConsumer.hpp"

OculusVREvent::OculusVREvent(EVENT_HEADER const& hdr)
  : QpcTime(*(uint64_t*)&hdr.TimeStamp)
  , ProcessId(hdr.ProcessId)
  , AppRenderStart(0)
  , AppRenderEnd(0)
  , ReprojectionStart(0)
  , ReprojectionEnd(0)
  , EndSpinWait(0)
  , AppMiss(false)
  , WarpMiss(false)
  , Completed(false)
{}

void OculusVRTraceConsumer::CompleteEvent(std::shared_ptr<OculusVREvent> p)
{
  if (p->Completed)
  {
    return;
  }

  p->Completed = true;
  {
    auto lock = scoped_lock(mMutex);
    mCompletedEvents.push_back(p);
  }
}

void HandleOculusVREvent(EVENT_RECORD* pEventRecord, OculusVRTraceConsumer* ovrConsumer)
{
  const auto& hdr = pEventRecord->EventHeader;
  const auto taskName = GetEventTaskName(pEventRecord);

  if (taskName.compare(L"PhaseSync") == 0) {
    const auto frameID = GetEventData<uint64_t>(pEventRecord, L"Frame");

    enum {
      BeginFrame = 63,
      CompleteFrame = 65
    };

    switch (hdr.EventDescriptor.Id) {
    case BeginFrame:
    {
      auto eventIter = ovrConsumer->mPresentsByFrameId.find(frameID);
      if (eventIter == ovrConsumer->mPresentsByFrameId.end())
        return;

      ovrConsumer->mProcessId = hdr.ProcessId;
      eventIter->second->AppRenderStart = *(uint64_t*)&hdr.TimeStamp;
      break;
    }
    case CompleteFrame:
    {
      auto eventIter = ovrConsumer->mPresentsByFrameId.find(frameID);
      if (eventIter == ovrConsumer->mPresentsByFrameId.end())
        return;

      eventIter->second->AppRenderEnd = *(uint64_t*)&hdr.TimeStamp;
      ovrConsumer->mPresentsCompositorStart.emplace(eventIter->second);
      ovrConsumer->mPresentsByFrameId.erase(frameID);
      break;
    }
    }
  }
  else if (taskName.compare(L"Compositor run loop (render thread) events.") == 0) {
    enum {
      CompositionBegin = 48,
      CompositionEndSpinWait = 53,
      CompositionEnd = 49,
      CompositionMissedCompositorFrame = 56
    };

    switch (hdr.EventDescriptor.Id) {
    case CompositionBegin:
    {
      std::shared_ptr<OculusVREvent> pEvent;

      if (ovrConsumer->mPresentsCompositorStart.empty())
      {
        if (ovrConsumer->mProcessId)
        {
          pEvent = std::make_shared<OculusVREvent>(hdr);
          pEvent->ProcessId = ovrConsumer->mProcessId;
        }
        else {
          return;
        }
      }
      else {
        pEvent = ovrConsumer->mPresentsCompositorStart.front();
        ovrConsumer->mPresentsCompositorStart.pop();
      }
      ovrConsumer->mActiveEvent = pEvent;
      pEvent->ReprojectionStart=(*(uint64_t*)&hdr.TimeStamp);
      ovrConsumer->mPresentsCompositorVSyncIndicator.emplace(pEvent);
      break;
    }
    case CompositionEndSpinWait:
    {
      if (ovrConsumer->mPresentsCompositorVSyncIndicator.empty())
      {
        return;
      }

      auto pEvent = ovrConsumer->mPresentsCompositorVSyncIndicator.front();
      ovrConsumer->mPresentsCompositorVSyncIndicator.pop();
      pEvent->EndSpinWait=(*(uint64_t*)&hdr.TimeStamp);
      ovrConsumer->mPresentsCompositorReprojection.emplace(pEvent);
      break;
    }
    case CompositionEnd:
    {
      if (ovrConsumer->mPresentsCompositorReprojection.empty())
      {
        return;
      }

      auto pEvent = ovrConsumer->mPresentsCompositorReprojection.front();
      ovrConsumer->mPresentsCompositorReprojection.pop();
      pEvent->ReprojectionEnd=(*(uint64_t*)&hdr.TimeStamp);
      ovrConsumer->CompleteEvent(pEvent);
      // get vsync Id to correlate to vsync timestamp event

      break;
    }
    case CompositionMissedCompositorFrame:
    {
      if (ovrConsumer->mActiveEvent)
      {
        ovrConsumer->mActiveEvent->WarpMiss = true;
      }
      break;
    }
    }
  }
  else if (taskName.compare(L"VirtualDisplay") == 0) {
    enum {
      ClientFrameMissed = 47
    };

    switch (hdr.EventDescriptor.Id) {
    case ClientFrameMissed:
    {
      if (ovrConsumer->mActiveEvent)
      {
        const auto processID = GetEventData<uint64_t>(pEventRecord, L"ProcessID");
        if (std::to_string(processID).compare(std::to_string(ovrConsumer->mActiveEvent->ProcessId)) == 0)
        {
          ovrConsumer->mActiveEvent->AppMiss = true;
        }
      }
      break;
    }
    }
  }
  else if (taskName.compare(L"Function") == 0) {
    const auto frameID = GetEventData<uint64_t>(pEventRecord, L"FrameID");
    // Call Compositor function
    if (hdr.EventDescriptor.Id == 0) {
      auto pEvent = std::make_shared<OculusVREvent>(hdr);
      ovrConsumer->mPresentsByFrameId.emplace(frameID, pEvent);
    }
  }
}