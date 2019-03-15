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

#include "OverlayBitmap.h"
#include "../Logging/MessageLog.h"

#include "../Recording/PerformanceCounter.hpp"
#include "../Recording/RecordingState.h"

#include <sstream>

using namespace Microsoft::WRL;

const D2D1_COLOR_F OverlayBitmap::clearColor_ = {0.0f, 0.0f, 0.0f, 0.01f};
const D2D1_COLOR_F OverlayBitmap::fpsBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.8f};
const D2D1_COLOR_F OverlayBitmap::msBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.7f};
const D2D1_COLOR_F OverlayBitmap::messageBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.5f};
const D2D1_COLOR_F OverlayBitmap::graphBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.6f};
const D2D1_COLOR_F OverlayBitmap::fontColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
const D2D1_COLOR_F OverlayBitmap::numberColor_ = {1.0f, 162.0f / 255.0f, 26.0f / 255.0f, 1.0f};
const D2D1_COLOR_F OverlayBitmap::recordingColor_ = {1.0f, 0.0f, 0.0f, 1.0f};

const D2D1_COLOR_F OverlayBitmap::colorBarSequence_[] = {
    {1.0f, 1.0f, 1.0f, 1.0f},                                   // White
    {0.0f, 1.0f, 0.0f, 1.0f},                                   // Lime
    {0.0f, 102.0f / 255.0f, 1.0f, 1.0f},                        // Blue
    {1.0f, 0.0f, 0.0f, 1.0f},                                   // Red
    {204.0f / 255.0f, 204.0f / 255.0f, 1.0f, 1.0f},             // Teal
    {51.0f / 255.0f, 204.0f / 255.0f, 1.0f, 1.0f},              // Navy
    {0.0f, 176.0f / 255.0f, 0.0f, 1.0f},                        // Green
    {0.0f, 1.0f, 1.0f, 1.0f},                                   // Aqua
    {138.0f / 255.0f, 135.0f / 255.0f, 0.0f, 1.0f},             // Dark green
    {221.0f / 255.0f, 221.0f / 255.0f, 221.0f / 255.0f, 1.0f},  // Silver
    {153.0f / 255.0f, 0.0f, 204.0f / 255.0f, 1.0f},             // Purple
    {185.0f / 255.0f, 172.0f / 255.0f, 3.0f / 255.0f, 1.0f},    // Olive
    {119.0f / 255.0f, 119.0f / 255.0f, 119.0f / 255.0f, 1.0f},  // Gray
    {128.0f / 255.0f, 0.0f, 128.0f / 255.0f, 1.0f},             // Fuchsia
    {1.0f, 255.0f / 255.0f, 0.0f, 1.0f},                        // Yellow
    {1.0f, 153.0f / 255.0f, 0.0f, 1.0f},                        // Orange
};

OverlayBitmap::RawData::RawData() : dataPtr{nullptr}, size{0}
{
  // Empty
}

OverlayBitmap::OverlayBitmap()
{
  // Empty
}

bool OverlayBitmap::Init(int screenWidth, int screenHeight, API api)
{
  if (!InitFactories()) {
    return false;
  }

  CalcSize(screenWidth, screenHeight);

  if (!InitBitmap()) {
    return false;
  }

  if (!InitText()) {
    return false;
  }

  switch (api) {
    case API::DX11:
      api_ = L"DX11";
      break;
    case API::DX12:
      api_ = L"DX12";
      break;
    case API::Vulkan:
      api_ = L"Vulkan";
      break;
    default:
      api_ = L"Unknown";
  }

  return true;
}

OverlayBitmap::~OverlayBitmap()
{
  for (int i = 0; i < alignmentCount_; ++i) {
    fpsMessage_[i].reset();
    msMessage_[i].reset();
    stateMessage_[i].reset();
    stopValueMessage_[i].reset();
    stopMessage_[i].reset();
    recordingMessage_[i].reset();
    apiMessage_[i].reset();
    graphLabelMessage_[i].reset();
  }

  renderTarget_.Reset();
  textBrush_.Reset();
  helperLineBrush_.Reset();
  textFormat_.Reset();
  messageFormat_.Reset();
  stopValueFormat_.Reset();
  stopMessageFormat_.Reset();
  recordingMessageFormat_.Reset();
  graphLabelMessagFormat_.Reset();
  iwicFactory_.Reset();
  bitmap_.Reset();
  bitmapLock_.Reset();
  writeFactory_.Reset();
  d2dFactory_.Reset();

  if (coInitialized_) {
    CoUninitialize();
    coInitialized_ = false;
  }
}

void OverlayBitmap::CalcSize(int screenWidth, int screenHeight)
{
  fullWidth_ = 256;
  fullHeight_ = lineHeight_ * 8;
  messageHeight_ = lineHeight_ * 3;
  barWidth_ = 24;

  Resize(screenWidth, screenHeight);

  const auto fullWidth = static_cast<FLOAT>(fullWidth_);
  const auto fullHeight = static_cast<FLOAT>(fullHeight_);
  const auto lineHeight = static_cast<FLOAT>(lineHeight_);
  const auto halfWidth = static_cast<FLOAT>(fullWidth_ / 2);
  const auto messageHeight = static_cast<FLOAT>(messageHeight_);

  const auto barHeight = static_cast<FLOAT>(screenHeight);
  const auto barWidth = static_cast<FLOAT>(barWidth_);

  // Areas depending on vertical alignment, as we switch FPS/ms with
  // the start/stop message when displaying the overlay at the bottom of the page.

  // |---|------------------------
  // |---||        apiArea       |
  // |---|------------------------
  // |---|| fpsArea | msArea | o |
  // |---|------------------------
  // |---|| |                    |
  // |---||L|       graph        |
  // |---|| |                    |
  // |---|------------------------
  // |---||      messageArea     |
  // |---||          +           |
  // |---||   messageValueArea   |
  // |---|------------------------
  const int indexUpperLeft = static_cast<int>(Alignment::UpperLeft);
  apiArea_[indexUpperLeft] = D2D1::RectF(barWidth, 0.0f, barWidth + fullWidth, lineHeight);
  fpsArea_[indexUpperLeft] =
      D2D1::RectF(barWidth, lineHeight, barWidth + halfWidth - 10.0f, lineHeight * 2);
  msArea_[indexUpperLeft] = D2D1::RectF(barWidth + halfWidth - 10.0f, lineHeight,
                                        barWidth + fullWidth - 20.0f, lineHeight * 2);
  recordingArea_[indexUpperLeft] =
      D2D1::RectF(barWidth + fullWidth - 20.0f, lineHeight, barWidth + fullWidth, lineHeight * 2);
  graphLabelArea_[indexUpperLeft] =
      D2D1::RectF(barWidth, lineHeight * 2, barWidth + 20.0f, lineHeight * 2 + messageHeight);
  graphArea_[indexUpperLeft] =
      D2D1::RectF(barWidth + 20.0f, lineHeight * 2, barWidth + fullWidth,
                  lineHeight * 2 + messageHeight);
  messageArea_[indexUpperLeft] =
      D2D1::RectF(barWidth, lineHeight * 2 + messageHeight, barWidth + fullWidth, fullHeight);
  messageValueArea_[indexUpperLeft] = D2D1::RectF(barWidth, lineHeight * 2 + messageHeight,
                                                  barWidth + fullWidth * 0.35f, fullHeight);
  colorBarArea_[indexUpperLeft] = D2D1::RectF(0.0f, 0.0f, barWidth, barHeight);

  // ------------------------|---|
  // |        apiArea       ||---|
  // ------------------------|---|
  // | o | fpsArea | msArea ||---|
  // ------------------------|---|
  // | |                    ||---|
  // |L|       graph        ||---|
  // | |                    ||---|
  // ------------------------|---|
  // |      messageArea     ||---|
  // |          +           ||---|
  // |   messageValueArea   ||---|
  // ------------------------|---|
  const int indexUpperRight = static_cast<int>(Alignment::UpperRight);
  apiArea_[indexUpperRight] = D2D1::RectF(0.0f, 0.0f, fullWidth, lineHeight);
  recordingArea_[indexUpperRight] = D2D1::RectF(0.0f, lineHeight, 20.0f, lineHeight * 2);
  fpsArea_[indexUpperRight] = D2D1::RectF(20.0f, lineHeight, halfWidth + 10, lineHeight * 2);
  msArea_[indexUpperRight] = D2D1::RectF(halfWidth + 10, lineHeight, fullWidth, lineHeight * 2);
  graphLabelArea_[indexUpperRight] =
      D2D1::RectF(0.0f, lineHeight * 2, 20.0f, lineHeight * 2 + messageHeight);
  graphArea_[indexUpperRight] =
      D2D1::RectF(20.0f, lineHeight * 2, fullWidth, lineHeight * 2 + messageHeight);
  messageArea_[indexUpperRight] =
      D2D1::RectF(0.0f, lineHeight * 2 + messageHeight, fullWidth, fullHeight);
  messageValueArea_[indexUpperRight] =
      D2D1::RectF(0.0f, lineHeight * 2 + messageHeight, fullWidth * 0.35f, fullHeight);
  colorBarArea_[indexUpperRight] = D2D1::RectF(fullWidth, 0.0f, fullWidth + barWidth, barHeight);

  // |---|------------------------
  // |---||      messageArea     |
  // |---||          +           |
  // |---||   messageValueArea   |
  // |---|------------------------
  // |---|| |                    |
  // |---||L|      graph         |
  // |---|| |                    |
  // |---|------------------------
  // |---|| fpsArea | msArea | o |
  // |---|------------------------
  // |---||        apiArea       |
  // |---|------------------------
  const int indexLowerLeft = static_cast<int>(Alignment::LowerLeft);
  messageArea_[indexLowerLeft] = D2D1::RectF(barWidth, barHeight - fullHeight_,
                                             barWidth + fullWidth, barHeight - lineHeight * 2);
  messageValueArea_[indexLowerLeft] =
      D2D1::RectF(barWidth, barHeight - fullHeight_, barWidth + fullWidth * 0.35f,
                  barHeight - lineHeight * 2 - messageHeight);
  graphLabelArea_[indexLowerLeft] =
      D2D1::RectF(barWidth, barHeight - lineHeight * 2 - messageHeight, barWidth + 20.0f,
                  barHeight - lineHeight * 2);
  graphArea_[indexLowerLeft] =
      D2D1::RectF(barWidth + 20.0f, barHeight - lineHeight * 2 - messageHeight,
                  barWidth + fullWidth, barHeight - lineHeight * 2);
  fpsArea_[indexLowerLeft] = D2D1::RectF(barWidth, barHeight - lineHeight * 2,
                                         barWidth + halfWidth - 10, barHeight - lineHeight);
  msArea_[indexLowerLeft] = D2D1::RectF(barWidth + halfWidth - 10, barHeight - lineHeight * 2,
                                        barWidth + fullWidth - 20.0f, barHeight - lineHeight);
  recordingArea_[indexLowerLeft] =
      D2D1::RectF(barWidth + fullWidth - 20.0f, barHeight - lineHeight * 2, barWidth + fullWidth,
                  barHeight - lineHeight);
  apiArea_[indexLowerLeft] =
      D2D1::RectF(barWidth + 0.0f, barHeight - lineHeight, barWidth + fullWidth, barHeight);
  colorBarArea_[indexLowerLeft] = D2D1::RectF(0.0f, 0.0f, barWidth, barHeight);

  // ------------------------|---|
  // |      messageArea     ||---|
  // |          +           ||---|
  // |   messageValueArea   ||---|
  // ------------------------|---|
  // | |                    ||---|
  // |L|       graph        ||---|
  // | |                    ||---|
  // ------------------------|---|
  // | o | fpsArea | msArea ||---|
  // ------------------------|---|
  // |        apiArea       ||---|
  // ------------------------|---|
  const int indexLowerRight = static_cast<int>(Alignment::LowerRight);
  messageArea_[indexLowerRight] =
      D2D1::RectF(0.0f, barHeight - fullHeight_, fullWidth, barHeight - lineHeight * 2);
  messageValueArea_[indexLowerRight] = D2D1::RectF(0.0f, barHeight - fullHeight_, fullWidth * 0.35f,
                                                   barHeight - lineHeight * 2 - messageHeight);
  graphLabelArea_[indexLowerRight] = D2D1::RectF(0.0f, barHeight - lineHeight * 2 - messageHeight,
                                                 20.0f, barHeight - lineHeight * 2);
  graphArea_[indexLowerRight] = D2D1::RectF(20.0f, barHeight - lineHeight * 2 - messageHeight,
                                            fullWidth, barHeight - lineHeight * 2);
  recordingArea_[indexLowerRight] =
      D2D1::RectF(0.0f, barHeight - lineHeight * 2, 20.0f, barHeight - lineHeight);
  fpsArea_[indexLowerRight] =
      D2D1::RectF(20.0f, barHeight - lineHeight * 2, halfWidth + 10, barHeight - lineHeight);
  msArea_[indexLowerRight] =
      D2D1::RectF(halfWidth + 10, barHeight - lineHeight * 2, fullWidth, barHeight - lineHeight);
  apiArea_[indexLowerRight] = D2D1::RectF(0.0f, barHeight - lineHeight, fullWidth, barHeight);
  colorBarArea_[indexLowerRight] = D2D1::RectF(fullWidth, 0.0f, fullWidth + barWidth, barHeight);

  // Full area is not depending on vertical alignment.
  fullArea_.d2d1 = D2D1::RectF(0.0f, 0.0f, fullWidth + barWidth, barHeight);
  fullArea_.wic = {0, 0, fullWidth_ + barWidth_, screenHeight_};
}

void OverlayBitmap::Resize(int screenWidth, int screenHeight)
{
  screenWidth_ = screenWidth;
  screenHeight_ = screenHeight;
  UpdateScreenPosition();
}

void OverlayBitmap::UpdateScreenPosition()
{
  const auto overlayPosition = RecordingState::GetInstance().GetOverlayPosition();
  if (IsLowerOverlayPosition(overlayPosition)) {
    if (IsLeftOverlayPosition(overlayPosition)) {
      screenPosition_.x = 0;
      screenPosition_.y = 0;
      currentAlignment_ = Alignment::LowerLeft;
    }
    else {
      screenPosition_.x = screenWidth_ - (fullWidth_ + barWidth_);
      screenPosition_.y = 0;
      currentAlignment_ = Alignment::LowerRight;
    }
  }
  else {
    if (IsLeftOverlayPosition(overlayPosition)) {
      screenPosition_.x = 0;
      screenPosition_.y = 0;
      currentAlignment_ = Alignment::UpperLeft;
    }
    else {
      screenPosition_.x = screenWidth_ - (fullWidth_ + barWidth_);
      screenPosition_.y = 0;
      currentAlignment_ = Alignment::UpperRight;
    }
  }
}

void OverlayBitmap::DrawOverlay()
{
  StartRendering();
  Update();
  FinishRendering();
}

void OverlayBitmap::StartRendering()
{
  renderTarget_->BeginDraw();
  renderTarget_->SetTransform(D2D1::IdentityMatrix());
}

void OverlayBitmap::Update()
{
  const auto textureState = RecordingState::GetInstance().Update();
  if (RecordingState::GetInstance().Started()) {
    performanceCounter_.Start();
  }
  const auto frameInfo = performanceCounter_.NextFrame();
  if (RecordingState::GetInstance().Stopped()) {
    performanceCounter_.Stop();
  }

  frameTimes_[currentFrame_] = frameInfo.frameTime;

  UpdateScreenPosition();
  renderTarget_->Clear(clearColor_);  // clear full bitmap
  if (RecordingState::GetInstance().IsOverlayShowing()) {
    DrawFrameInfo(frameInfo);
    DrawMessages(textureState);
  }
  if (RecordingState::GetInstance().IsGraphOverlayShowing()) {
    DrawGraph();
  }
  if (RecordingState::GetInstance().IsBarOverlayShowing()) {
    DrawBar();
  }

  currentFrame_ = (currentFrame_ + 1) % 512;
}

void OverlayBitmap::DrawFrameInfo(const GameOverlay::PerformanceCounter::FrameInfo& frameInfo)
{
  const int alignment = static_cast<int>(currentAlignment_);
  // api
  renderTarget_->PushAxisAlignedClip(apiArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(messageBackgroundColor_);
  apiMessage_[alignment]->WriteMessage(api_, L" API");
  apiMessage_[alignment]->SetText(writeFactory_.Get(), textFormat_.Get());
  apiMessage_[alignment]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();

  // recording dot
  renderTarget_->PushAxisAlignedClip(recordingArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(fpsBackgroundColor_);
  if (recording_) {
    recordingMessage_[alignment]->WriteMessage(L"\x2022");
  }
  else {
    recordingMessage_[alignment]->WriteMessage(L" ");
  }
  recordingMessage_[alignment]->SetText(writeFactory_.Get(), textFormat_.Get());
  recordingMessage_[alignment]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();

  // fps counter
  renderTarget_->PushAxisAlignedClip(fpsArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(fpsBackgroundColor_);
  fpsMessage_[alignment]->WriteMessage(frameInfo.fps, L" FPS");
  fpsMessage_[alignment]->SetText(writeFactory_.Get(), textFormat_.Get());
  fpsMessage_[alignment]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();

  // ms counter
  renderTarget_->PushAxisAlignedClip(msArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(msBackgroundColor_);
  msMessage_[alignment]->WriteMessage(frameInfo.ms, L" ms", precision_);
  msMessage_[alignment]->SetText(writeFactory_.Get(), textFormat_.Get());
  msMessage_[alignment]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::DrawMessages(TextureState textureState)
{
  if (textureState == TextureState::Default) {
    const int alignment = static_cast<int>(currentAlignment_);
    renderTarget_->PushAxisAlignedClip(messageArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
    renderTarget_->Clear(clearColor_);
    renderTarget_->PopAxisAlignedClip();
    return;
  }

  const int alignment = static_cast<int>(currentAlignment_);
  renderTarget_->PushAxisAlignedClip(messageArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(messageBackgroundColor_);
  if (textureState == TextureState::Start) {
    stateMessage_[alignment]->WriteMessage(L"Capture Started");
    stateMessage_[alignment]->SetText(writeFactory_.Get(), messageFormat_.Get());
    stateMessage_[alignment]->Draw(renderTarget_.Get());
    recording_ = true;
  }
  else if (textureState == TextureState::Stop) {
    const auto capture = performanceCounter_.GetLastCaptureResults();
    stateMessage_[alignment]->WriteMessage(L"Capture Ended\n");
    stateMessage_[alignment]->SetText(writeFactory_.Get(), messageFormat_.Get());
    stateMessage_[alignment]->Draw(renderTarget_.Get());

    stopValueMessage_[alignment]->WriteMessage(capture.averageFPS, L"\n", precision_);
    stopValueMessage_[alignment]->WriteMessage(capture.averageMS, L"\n", precision_);
    stopValueMessage_[alignment]->WriteMessage(capture.frameTimePercentile, L"", precision_);
    stopValueMessage_[alignment]->SetText(writeFactory_.Get(), stopValueFormat_.Get());
    stopValueMessage_[alignment]->Draw(renderTarget_.Get());

    stopMessage_[alignment]->WriteMessage(L"FPS Average\n");
    stopMessage_[alignment]->WriteMessage(L"ms  Average\n");
    stopMessage_[alignment]->WriteMessage(L"99th Percentile");
    stopMessage_[alignment]->SetText(writeFactory_.Get(), stopMessageFormat_.Get());
    stopMessage_[alignment]->Draw(renderTarget_.Get());
    recording_ = false;
  }
  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::DrawGraph()
{
  const int alignment = static_cast<int>(currentAlignment_);

  renderTarget_->PushAxisAlignedClip(graphArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(graphBackgroundColor_);

  renderTarget_->DrawLine(D2D1::Point2F(0.0f, graphArea_[alignment].bottom),
                          D2D1::Point2F(fullWidth_ + barWidth_, graphArea_[alignment].bottom),
                          helperLineBrush_.Get());
  renderTarget_->DrawLine(D2D1::Point2F(0.0f, graphArea_[alignment].bottom - 33.33f),
      D2D1::Point2F(fullWidth_ + barWidth_, graphArea_[alignment].bottom - 33.33f),
                          helperLineBrush_.Get());
  renderTarget_->DrawLine(D2D1::Point2F(0.0f, graphArea_[alignment].bottom - 66.66f),
      D2D1::Point2F(fullWidth_ + barWidth_, graphArea_[alignment].bottom - 66.66f),
                          helperLineBrush_.Get());
  renderTarget_->DrawLine(D2D1::Point2F(0.0f, graphArea_[alignment].bottom - 99.99f),
      D2D1::Point2F(fullWidth_ + barWidth_, graphArea_[alignment].bottom - 99.99f),
                          helperLineBrush_.Get());

  points_[0] = D2D1::Point2F(0.0f, graphArea_[alignment].bottom - frameTimes_[currentFrame_ % 512]);
  float time = frameTimes_[((currentFrame_ + 512) - 1) % 512] * 0.2f;

  for (int i = 1; i < 512; i++) {
    points_[i] = D2D1::Point2F(
        time, graphArea_[alignment].bottom - frameTimes_[((currentFrame_ + 512) - i) % 512]);
    time = time + frameTimes_[((currentFrame_ + 512) - i - 1) % 512] * 0.2f;

    renderTarget_->DrawLine(points_[i - 1], points_[i], textBrush_.Get());
  }

  renderTarget_->PopAxisAlignedClip();

  renderTarget_->PushAxisAlignedClip(graphLabelArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(graphBackgroundColor_);
  graphLabelMessage_[alignment]->WriteMessage(L"ms\n\n\n100\n\n\n66\n\n\n33\n\n\n0");
  graphLabelMessage_[alignment]->SetText(writeFactory_.Get(), graphLabelMessagFormat_.Get());
  graphLabelMessage_[alignment]->Draw(renderTarget_.Get());
  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::DrawBar()
{
  const int alignment = static_cast<int>(currentAlignment_);

  // colored bar
  renderTarget_->PushAxisAlignedClip(colorBarArea_[alignment], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(colorBarSequence_[colorSequenceIndex_]);

  colorSequenceIndex_ = (colorSequenceIndex_ + 1) % 16;

  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::FinishRendering()
{
  HRESULT hr = renderTarget_->EndDraw();
  if (FAILED(hr)) {
    g_messageLog.LogWarning("OverlayBitmap", "EndDraw failed, HRESULT", hr);
  }
}

OverlayBitmap::RawData OverlayBitmap::GetBitmapDataRead()
{
  if (bitmapLock_) {
    g_messageLog.LogWarning("OverlayBitmap", "Bitmap lock was not released");
  }
  bitmapLock_.Reset();

  RawData rawData = {};
  const auto& currBitmapArea = fullArea_.wic;
  HRESULT hr = bitmap_->Lock(&currBitmapArea, WICBitmapLockRead, &bitmapLock_);
  if (FAILED(hr)) {
    g_messageLog.LogWarning("OverlayBitmap", "Bitmap lock failed, HRESULT", hr);
    return rawData;
  }

  hr = bitmapLock_->GetDataPointer(&rawData.size, &rawData.dataPtr);
  if (FAILED(hr)) {
    g_messageLog.LogWarning("OverlayBitmap", "Bitmap lock GetDataPointer failed, HRESULT", hr);
    rawData = {};
  }
  return rawData;
}

void OverlayBitmap::UnlockBitmapData() { bitmapLock_.Reset(); }

int OverlayBitmap::GetFullWidth() const { return fullWidth_ * 2; }

int OverlayBitmap::GetFullHeight() const { return screenHeight_; }

OverlayBitmap::Position OverlayBitmap::GetScreenPos() const { return screenPosition_; }

const D2D1_RECT_F& OverlayBitmap::GetCopyArea() const { return fullArea_.d2d1; }

VkFormat OverlayBitmap::GetVKFormat() const { return VK_FORMAT_B8G8R8A8_UNORM; }

bool OverlayBitmap::InitFactories()
{
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "D2D1CreateFactory failed, HRESULT", hr);
    return false;
  }

  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory_),
                           reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "DWriteCreateFactory failed, HRESULT", hr);
    return false;
  }

  hr = CoInitialize(NULL);
  if (hr == S_OK || hr == S_FALSE) {
    coInitialized_ = true;
  }
  else {
    g_messageLog.LogWarning("OverlayBitmap", "CoInitialize failed, HRESULT", hr);
  }

  hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&iwicFactory_));
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CoCreateInstance failed, HRESULT", hr);
    return false;
  }
  return true;
}

bool OverlayBitmap::InitBitmap()
{
  HRESULT hr = iwicFactory_->CreateBitmap(
      fullWidth_ * 2, screenHeight_, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &bitmap_);
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CreateBitmap failed, HRESULT", hr);
    return false;
  }

  const auto rtProperties = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f,
      D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

  hr = d2dFactory_->CreateWicBitmapRenderTarget(bitmap_.Get(), rtProperties, &renderTarget_);
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CreateWicBitmapRenderTarget failed, HRESULT", hr);
    return false;
  }

  hr = renderTarget_->CreateSolidColorBrush(fontColor_, &textBrush_);
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }

  hr = renderTarget_->CreateSolidColorBrush(numberColor_, &helperLineBrush_);
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }

  return true;
}

bool OverlayBitmap::InitText()
{
  textFormat_ =
      CreateTextFormat(25.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  messageFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
  stopValueFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  stopMessageFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  recordingMessageFormat_ =
      CreateTextFormat(25.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  graphLabelMessagFormat_ =
      CreateTextFormat(8.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_FAR);

  InitTextForAlignment(Alignment::LowerLeft);
  InitTextForAlignment(Alignment::LowerRight);
  InitTextForAlignment(Alignment::UpperLeft);
  InitTextForAlignment(Alignment::UpperRight);
  return true;
}

void OverlayBitmap::InitTextForAlignment(Alignment alignment)
{
  const int ialignment = static_cast<int>(alignment);
  auto apiArea = apiArea_[ialignment];
  apiMessage_[ialignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  apiMessage_[ialignment]->SetArea(apiArea.left, apiArea.top, apiArea.right - apiArea.left,
                                   apiArea.bottom - apiArea.top);

  auto fpsArea = fpsArea_[ialignment];
  fpsMessage_[ialignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  fpsMessage_[ialignment]->SetArea(fpsArea.left, fpsArea.top, fpsArea.right - fpsArea.left,
                                   fpsArea.bottom - fpsArea.top);

  auto msArea = msArea_[ialignment];
  msMessage_[ialignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  msMessage_[ialignment]->SetArea(msArea.left, msArea.top, msArea.right - msArea.left,
                                  msArea.bottom - msArea.top);

  auto recordingArea = recordingArea_[ialignment];
  recordingMessage_[ialignment].reset(
      new TextMessage(renderTarget_.Get(), recordingColor_, recordingColor_));
  recordingMessage_[ialignment]->SetArea(recordingArea.left, recordingArea.top,
                                         recordingArea.right - recordingArea.left,
                                         recordingArea.bottom - recordingArea.top);

  auto graphLabelArea = graphLabelArea_[ialignment];
  graphLabelMessage_[ialignment].reset(
      new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  graphLabelMessage_[ialignment]->SetArea(graphLabelArea.left, graphLabelArea.top,
                                          graphLabelArea.right - graphLabelArea.left,
                                          graphLabelArea.bottom - graphLabelArea.top);

  const auto offset2 = offset_ * 2;

  auto messageArea = messageArea_[ialignment];
  stateMessage_[ialignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stateMessage_[ialignment]->SetArea(messageArea.left + offset2, messageArea.top + offset_,
                                     messageArea.right - messageArea.left - offset2,
                                     messageArea.bottom - messageArea.top - offset_);

  auto messageValueArea = messageValueArea_[ialignment];
  stopValueMessage_[ialignment].reset(
      new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopValueMessage_[ialignment]->SetArea(messageValueArea.left + offset2,
                                         messageValueArea.top + offset_,
                                         messageValueArea.right - messageValueArea.left - offset2,
                                         messageValueArea.bottom - messageValueArea.top - offset_);

  stopMessage_[ialignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopMessage_[ialignment]->SetArea(messageValueArea.right + offset2, messageArea.top + offset_,
                                    messageArea.right - messageArea.left - offset2,
                                    messageArea.bottom - messageArea.top - offset_);
}

IDWriteTextFormat* OverlayBitmap::CreateTextFormat(float size, DWRITE_TEXT_ALIGNMENT textAlignment,
                                                   DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment)
{
  IDWriteTextFormat* textFormat;
  HRESULT hr = writeFactory_->CreateTextFormat(L"Verdane", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                               DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                               size, L"en-us", &textFormat);
  if (FAILED(hr)) {
    g_messageLog.LogError("OverlayBitmap", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }
  textFormat->SetTextAlignment(textAlignment);
  textFormat->SetParagraphAlignment(paragraphAlignment);
  return textFormat;
}
