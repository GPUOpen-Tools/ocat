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

#include "RecordingState.h"
#include "../Logging/MessageLog.h"

using Clock = std::chrono::high_resolution_clock;
using fSeconds = std::chrono::duration<float>;

RecordingState& RecordingState::GetInstance()
{
  static RecordingState instance;
  return instance;
}

RecordingState::RecordingState() 
{ 
  currentStateStart_ = Clock::now(); 
}

bool RecordingState::Started()
{
  if (stateChanged_ && recording_) {
    stateChanged_ = false;
    return true;
  }
  return false;
}

bool RecordingState::Stopped()
{
  if (stateChanged_ && !recording_) {
    stateChanged_ = false;
    return true;
  }
  return false;
}

bool RecordingState::IsOverlayShowing() 
{ 
  return showOverlay_; 
}

TextureState RecordingState::Update()
{
  const fSeconds duration = Clock::now() - currentStateStart_;
  if (recording_) { // recording
    if ((currentTextureState_ == TextureState::Start) && (duration.count() > startDisplayTime_)) 
    {
      currentTextureState_ = TextureState::Default;
    }
    if (recordingTime_ > 0.0f && (duration.count() > recordingTime_)) {
      Stop();
    }
  }
  else // not recording
  {
    if ((currentTextureState_ == TextureState::Stop) &&
      (duration.count() > endDisplayTime_))
    {
      currentTextureState_ = TextureState::Default;
    }
  }
  return currentTextureState_;
}

void RecordingState::SetDisplayTimes(float start, float end)
{
  startDisplayTime_ = start;
  endDisplayTime_ = end;
}

void RecordingState::SetRecordingTime(float time) 
{ 
  recordingTime_ = time; 
}

void RecordingState::ShowOverlay() 
{ 
  showOverlay_ = true; 
}

void RecordingState::HideOverlay() 
{ 
  showOverlay_ = false; 
}

OverlayPosition RecordingState::GetOverlayPosition()
{
  return overlayPosition_;
}

void RecordingState::SetOverlayPosition(OverlayPosition overlayPosition)
{
  overlayPosition_ = overlayPosition;
}

void RecordingState::Start()
{
  recording_ = true;
  currentTextureState_ = TextureState::Start;
  currentStateStart_ = Clock::now();
  stateChanged_ = true;
}

void RecordingState::Stop()
{
  recording_ = false;
  currentTextureState_ = TextureState::Stop;
  currentStateStart_ = Clock::now();
  stateChanged_ = true;
}
