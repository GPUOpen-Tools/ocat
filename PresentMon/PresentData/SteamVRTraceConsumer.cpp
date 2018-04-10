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

#include "SteamVRTraceConsumer.hpp"

SteamVREvent::SteamVREvent(EVENT_HEADER const& hdr)
  : QpcTime (*(uint64_t*)&hdr.TimeStamp)
  , ProcessId(hdr.ProcessId)
  , AppRenderStart(0)
  , AppRenderEnd(0)
  , ReprojectionStart(0)
  , ReprojectionEnd(0)
  , MsSinceLastVSync(0)
  , TimeStampSinceLastVSync(0)
  , AppMiss(false)
  , WarpMiss(false)
  , Completed(false)
{}

void SteamVRTraceConsumer::CompleteEvent(std::shared_ptr<SteamVREvent> p)
{
  if (p->Completed)
  {
    return;
  }

  // finish earlier event chains which got stuck
  std::vector<uint64_t> erase;

  for (auto& event : mPresentsByFrameId)
  {
    if (event.first < p->FrameId) {
      auto lock = scoped_lock(mMutex);
      event.second->Completed = true;
      event.second->AppMiss = true;
      mCompletedEvents.push_back(event.second);
      erase.push_back(event.first);
    }
  }

  for (auto id : erase)
  {
    mPresentsByFrameId.erase(id);
  }

  p->Completed = true;
  {
    auto lock = scoped_lock(mMutex);
    mCompletedEvents.push_back(p);
  }
}

void HandleSteamVREvent(EVENT_RECORD* pEventRecord, SteamVRTraceConsumer* svrConsumer)
{
  const auto& hdr = pEventRecord->EventHeader;

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  const auto eventData = converter.to_bytes((wchar_t*)pEventRecord->UserData);

  std::stringstream datastream(eventData);
  std::string task;
  getline(datastream, task, '=');

  if (task.compare("[Compositor Client] Received Idx") == 0) 
  {
    auto pEvent = std::make_shared<SteamVREvent>(hdr);
    pEvent->AppRenderStart = *(uint64_t*)&hdr.TimeStamp;
    svrConsumer->mProcessId = hdr.ProcessId;

  // get frame id from event data;
  // [Compositor Client] Received Idx=... Id=... - we want Id not Idx
    std::string id;
    getline(datastream, id, '=');
    getline(datastream, id, '=');
    svrConsumer->mPresentsByFrameId.emplace(std::stoi(id), pEvent);
    svrConsumer->mPresentsCompositorSubmitLeft.emplace(std::stoi(id), pEvent);
    pEvent->FrameId = std::stoi(id);
  }
  else if (task.compare("[Compositor Client] Submit Left") == 0)
  {
    if (svrConsumer->mPresentsCompositorSubmitLeft.empty())
      return;

    auto pEvent = svrConsumer->mPresentsCompositorSubmitLeft.front();
    svrConsumer->mPresentsCompositorSubmitEnd.emplace(pEvent);
  }
  else if (task.compare("[Compositor Client] Submit Right") == 0)
  {
    if (svrConsumer->mPresentsCompositorSubmitRight.empty())
      return;

    auto pEvent = svrConsumer->mPresentsCompositorSubmitRight.front();
    svrConsumer->mPresentsCompositorSubmitEnd.emplace(pEvent);
  }
  else if (task.compare("[Compositor Client] Submit End") == 0)
  {
    if (svrConsumer->mPresentsCompositorSubmitEnd.empty())
      return;

    auto pEvent = svrConsumer->mPresentsCompositorSubmitEnd.front();
    if (svrConsumer->mPresentsCompositorSubmitLeft.empty()) {
      pEvent.second->AppRenderEnd = *(uint64_t*)&hdr.TimeStamp;
      // at this stage only the app has finished, but it is not presented yet
      pEvent.second->AppRenderingCompleted = true;
      svrConsumer->mPresentsCompositorSubmitRight.pop();
      svrConsumer->mPresentsCompositorSubmitEnd.pop();
      return;
    }

    svrConsumer->mPresentsCompositorSubmitRight.emplace(pEvent);
    svrConsumer->mPresentsCompositorSubmitEnd.pop();
    svrConsumer->mPresentsCompositorSubmitLeft.pop();
  }
  else if (task.compare("[Compositor] NewFrame id") == 0)
  {
    std::shared_ptr<SteamVREvent> pEvent;
    // get frame id from event data
    // [Compositor] NewFrame id=... idx=...
    std::string id;
    getline(datastream, id, ' ');
    auto eventIter = svrConsumer->mPresentsByFrameId.find(std::stoi(id));
    // we don't have a corresponding app rendering event with the same frame id
    if (eventIter == svrConsumer->mPresentsByFrameId.end())
    {
      // compositor could already be one frame counter ahead
      auto eventIter = svrConsumer->mPresentsByFrameId.find(std::stoi(id) + 1);
      if (eventIter != svrConsumer->mPresentsByFrameId.end())
      {
        pEvent = eventIter->second;
        svrConsumer->mPresentsByFrameId.erase(std::stoi(id) + 1);
      }
    // no luck, create new event chain
    else if (svrConsumer->mProcessId) {
      pEvent = std::make_shared<SteamVREvent>(hdr);
      pEvent->ProcessId = svrConsumer->mProcessId;
      pEvent->FrameId = std::stoi(id);
    }
    // seems like it is a compositor event chain with unknown corresponding app -> skip
      else {
        return;
      }
    }
    else 
    {
      pEvent = eventIter->second;
      svrConsumer->mPresentsByFrameId.erase(std::stoi(id));
    }

    // place in compositor chain queues process the events sequentially
    svrConsumer->mPresentsCompositorReprojection.emplace(std::stoi(id), pEvent);
    pEvent->ReprojectionStart = *(uint64_t*)&hdr.TimeStamp;

  // if IDX is 0, app rendering is not completed yet -> app miss
  // [Compositor] NewFrame id=... idx=...
    getline(datastream, id, '=');
    getline(datastream, id, '=');
    if (std::stoi(id) == 0)
    {
      pEvent->AppMiss = true;
    }
  }
  else if (task.compare("[Compositor] End Present") == 0) {
    if (svrConsumer->mPresentsCompositorReprojection.empty())
      return;

    auto pEvent = svrConsumer->mPresentsCompositorReprojection.front();
    svrConsumer->mPresentsCompositorReprojection.pop();
    pEvent.second->ReprojectionEnd = *(uint64_t*)&hdr.TimeStamp;
    svrConsumer->mPresentsCompositorLastTextureIndex.emplace(pEvent);
  }
  else if (task.compare("[Compositor] LastSceneTextureIndex") == 0) {
    if (svrConsumer->mPresentsCompositorLastTextureIndex.empty())
      return;

  // get frame id from event data
  // [Compositor] LastSceneTextureIndex=... id=... vsync=...
  std::string id;
  getline(datastream, id, '=');
  getline(datastream, id, ' ');

    auto pEvent = svrConsumer->mPresentsCompositorLastTextureIndex.front();
    svrConsumer->mPresentsCompositorLastTextureIndex.pop();
    // last scene texture index is behind the expected frame index -> warp miss?
    if (std::stoi(id) < pEvent.first && !pEvent.second->AppMiss)
    {
      pEvent.second->WarpMiss = true;
    }
    svrConsumer->mPresentsCompositorVSyncIndicator.emplace(pEvent.second);
  }
  else
  {
    std::stringstream datastream_vsync(eventData);
    getline(datastream_vsync, task, ':');
    // [Compositor] TimeSinceLastVSync: 1.070581(701642)
    if (task.compare("[Compositor] TimeSinceLastVSync") == 0)
    {
      if (svrConsumer->mPresentsCompositorVSyncIndicator.empty())
        return;

      std::string timeSinceLastVSync;
      getline(datastream_vsync, timeSinceLastVSync, '(');
      auto pEvent = svrConsumer->mPresentsCompositorVSyncIndicator.front();
      svrConsumer->mPresentsCompositorVSyncIndicator.pop();
      pEvent->MsSinceLastVSync = stof(timeSinceLastVSync);
      pEvent->TimeStampSinceLastVSync = *(uint64_t*)&hdr.TimeStamp;
      svrConsumer->CompleteEvent(pEvent);
    }
  }
}