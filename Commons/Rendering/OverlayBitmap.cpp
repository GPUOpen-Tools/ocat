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
const D2D1_COLOR_F OverlayBitmap::fontColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
const D2D1_COLOR_F OverlayBitmap::numberColor_ = {1.0f, 162.0f / 255.0f, 26.0f / 255.0f, 1.0f};
const D2D1_COLOR_F OverlayBitmap::recordingColor_ = {1.0f, 0.0f, 0.0f, 1.0f};

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

  switch (api)
  {
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
  }

  renderTarget_.Reset();
  textBrush_.Reset();
  textFormat_.Reset();
  messageFormat_.Reset();
  stopValueFormat_.Reset();
  stopMessageFormat_.Reset();
  recordingMessageFormat_.Reset();
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
  fullHeight_ = lineHeight_ * 5;

  Resize(screenWidth, screenHeight);

  const auto fullWidth = static_cast<FLOAT>(fullWidth_);
  const auto fullHeight = static_cast<FLOAT>(fullHeight_);
  const auto lineHeight = static_cast<FLOAT>(lineHeight_);
  const auto halfWidth = static_cast<FLOAT>(fullWidth_ / 2);

  // Areas depending on vertical alignment, as we switch FPS/ms with
  // the start/stop message when displaying the overlay at the bottom of the page.

  // ------------------------
  // |        apiArea       |
  // ------------------------
  // | fpsArea | msArea | o |
  // ------------------------
  // |      messageArea     |
  // |          +           |
  // |   messageValueArea   |
  // ------------------------
  const int indexUpperLeft = static_cast<int>(Alignment::UpperLeft);
  apiArea_[indexUpperLeft] = D2D1::RectF(0.0f, 0.0f, fullWidth, lineHeight);
  fpsArea_[indexUpperLeft] = D2D1::RectF(0.0f, lineHeight, halfWidth - 10.0f, lineHeight * 2);
  msArea_[indexUpperLeft] = D2D1::RectF(halfWidth - 10.0f, lineHeight, fullWidth - 20.0f, lineHeight * 2);
  recordingArea_[indexUpperLeft] = D2D1::RectF(fullWidth - 20.0f, lineHeight, fullWidth, lineHeight * 2);
  messageArea_[indexUpperLeft] = D2D1::RectF(0.0f, lineHeight * 2, fullWidth, fullHeight);
  messageValueArea_[indexUpperLeft] = D2D1::RectF(0.0f, lineHeight * 2, fullWidth * 0.35f, fullHeight);

  // ------------------------
  // |        apiArea       |
  // ------------------------
  // | o | fpsArea | msArea |
  // ------------------------
  // |      messageArea     |
  // |          +           |
  // |   messageValueArea   |
  // ------------------------
  const int indexUpperRight = static_cast<int>(Alignment::UpperRight);
  apiArea_[indexUpperRight] = D2D1::RectF(0.0f, 0.0f, fullWidth, lineHeight);
  recordingArea_[indexUpperRight] = D2D1::RectF(0.0f, lineHeight, 20.0f, lineHeight * 2);
  fpsArea_[indexUpperRight] = D2D1::RectF(20.0f, lineHeight, halfWidth + 10, lineHeight * 2);
  msArea_[indexUpperRight] = D2D1::RectF(halfWidth + 10, lineHeight, fullWidth, lineHeight * 2);
  messageArea_[indexUpperRight] = D2D1::RectF(0.0f, lineHeight * 2, fullWidth, fullHeight);
  messageValueArea_[indexUpperRight] = D2D1::RectF(0.0f, lineHeight * 2, fullWidth * 0.35f, fullHeight);

  // ------------------------
  // |      messageArea     |
  // |          +           |
  // |   messageValueArea   |
  // ------------------------
  // | fpsArea | msArea | o |
  // ------------------------
  // |        apiArea       |
  // ------------------------
  const int indexLowerLeft = static_cast<int>(Alignment::LowerLeft);
  messageArea_[indexLowerLeft] = D2D1::RectF(0.0f, 0.0f, fullWidth, fullHeight - lineHeight * 2);
  messageValueArea_[indexLowerLeft] = D2D1::RectF(0.0f, 0.0f, fullWidth * 0.35f, fullHeight - lineHeight * 2);
  fpsArea_[indexLowerLeft] = D2D1::RectF(0.0f, fullHeight - lineHeight * 2, halfWidth - 10, fullHeight - lineHeight);
  msArea_[indexLowerLeft] = D2D1::RectF(halfWidth - 10, fullHeight - lineHeight * 2, fullWidth - 20.0f, fullHeight - lineHeight);
  recordingArea_[indexLowerLeft] = D2D1::RectF(fullWidth - 20.0f, fullHeight - lineHeight * 2, fullWidth, fullHeight - lineHeight);
  apiArea_[indexLowerLeft] = D2D1::RectF(0.0f, fullHeight - lineHeight, fullWidth, fullHeight);

  // ------------------------
  // |      messageArea     |
  // |          +           |
  // |   messageValueArea   |
  // ------------------------
  // | o | fpsArea | msArea |
  // ------------------------
  // |        apiArea       |
  // ------------------------
  const int indexLowerRight = static_cast<int>(Alignment::LowerRight);
  messageArea_[indexLowerRight] = D2D1::RectF(0.0f, 0.0f, fullWidth, fullHeight - lineHeight * 2);
  messageValueArea_[indexLowerRight] = D2D1::RectF(0.0f, 0.0f, fullWidth * 0.35f, fullHeight - lineHeight * 2);
  recordingArea_[indexLowerRight] = D2D1::RectF(0.0f, fullHeight - lineHeight * 2, 20.0f, fullHeight - lineHeight);
  fpsArea_[indexLowerRight] = D2D1::RectF(20.0f, fullHeight - lineHeight * 2, halfWidth + 10, fullHeight - lineHeight);
  msArea_[indexLowerRight] = D2D1::RectF(halfWidth + 10, fullHeight - lineHeight * 2, fullWidth, fullHeight - lineHeight);
  apiArea_[indexLowerRight] = D2D1::RectF(0.0f, fullHeight - lineHeight, fullWidth, fullHeight);

  // Full area is not depending on vertical alignment.
  fullArea_.d2d1 = D2D1::RectF(0.0f, 0.0f, fullWidth, fullHeight);
  fullArea_.wic = {0, 0, fullWidth_, fullHeight_};
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
      screenPosition_.x = offset_;
      screenPosition_.y = screenHeight_ - fullHeight_ - offset_;
      currentAlignment_ = Alignment::LowerLeft;
    }
    else {
      screenPosition_.x = screenWidth_ - fullWidth_ - offset_;
      screenPosition_.y = screenHeight_ - fullHeight_ - offset_;
      currentAlignment_ = Alignment::LowerRight;
    }
  }
  else {
    if (IsLeftOverlayPosition(overlayPosition)) {
      screenPosition_.x = offset_;
      screenPosition_.y = offset_;
      currentAlignment_ = Alignment::UpperLeft;
    }
    else {
      screenPosition_.x = screenWidth_ - fullWidth_ - offset_;
      screenPosition_.y = offset_;
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

  UpdateScreenPosition();
  renderTarget_->Clear(clearColor_);  // clear full bitmap
  if (RecordingState::GetInstance().IsOverlayShowing()) {
    DrawFrameInfo(frameInfo);
    DrawMessages(textureState);
  }
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

int OverlayBitmap::GetFullWidth() const { return fullWidth_; }

int OverlayBitmap::GetFullHeight() const { return fullHeight_; }

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
  HRESULT hr = iwicFactory_->CreateBitmap(fullWidth_, fullHeight_, GUID_WICPixelFormat32bppPBGRA,
                                          WICBitmapCacheOnLoad, &bitmap_);
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
