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
#include "..\Logging\MessageLog.h"

#include "..\Recording\PerformanceCounter.hpp"
#include "..\Recording\RecordingState.h"

#include <sstream>

using namespace Microsoft::WRL;

const D2D1_COLOR_F OverlayBitmap::clearColor_ = { 0.0f, 0.0f, 0.0f, 0.0f };
const D2D1_COLOR_F OverlayBitmap::fpsBackgroundColor_ = { 0.0f, 0.0f, 0.0f, 0.8f };
const D2D1_COLOR_F OverlayBitmap::msBackgroundColor_ = { 0.0f, 0.0f, 0.0f, 0.7f };
const D2D1_COLOR_F OverlayBitmap::messageBackgroundColor_ = { 0.0f, 0.0f, 0.0f, 0.5f };
const D2D1_COLOR_F OverlayBitmap::fontColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
const D2D1_COLOR_F OverlayBitmap::numberColor_ = { 1.0f, 162.0f / 255.0f, 26.0f / 255.0f, 1.0f };

OverlayBitmap::RawData::RawData() 
  : dataPtr{nullptr}, size{0} 
{
  // Empty
}

OverlayBitmap::OverlayBitmap()
{
  // Empty
}

bool OverlayBitmap::Init(int screenWidth, int screenHeight)
{
  if (!InitFactories())
  {
    return false;
  }

  CalcSize(screenWidth, screenHeight);

  if (!InitBitmap())
  {
    return false;
  }

  if (!InitText())
  {
    return false;
  }
  return true;
}

OverlayBitmap::~OverlayBitmap()
{
  for (int i = 0; i < VerticalAlignment::COUNT; ++i)
  {
    fpsMessage_[i].reset();
    msMessage_[i].reset();
    stateMessage_[i].reset();
    stopValueMessage_[i].reset();
    stopMessage_[i].reset();
  }
  
  renderTarget_.Reset();
  textBrush_.Reset();
  textFormat_.Reset();
  messageFormat_.Reset();
  stopValueFormat_.Reset();
  stopMessageFormat_.Reset();
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
  fullHeight_ = lineHeight_ * 4;

  Resize(screenWidth, screenHeight);

  const auto fullWidth = static_cast<FLOAT>(fullWidth_);
  const auto fullHeight = static_cast<FLOAT>(fullHeight_);
  const auto lineHeight = static_cast<FLOAT>(lineHeight_);
  const auto halfWidth = static_cast<FLOAT>(fullWidth_ / 2);

  // Areas depending on vertical alignment, as we switch FPS/ms with 
  // the start/stop message when displaying the overlay at the bottom of the page.

  // --------------------
  // | fpsArea | msArea |
  // --------------------
  // |    messageArea   |
  // |        +         |
  // | messageValueArea |
  // --------------------
  fpsArea_[VerticalAlignment::UPPER] = D2D1::RectF(0.0f, 0.0f, halfWidth, lineHeight);
  msArea_[VerticalAlignment::UPPER] = D2D1::RectF(halfWidth, 0.0f, fullWidth, lineHeight);
  messageArea_[VerticalAlignment::UPPER] = D2D1::RectF(0.0f, lineHeight, fullWidth, fullHeight);
  messageValueArea_[VerticalAlignment::UPPER] = D2D1::RectF(0.0f, lineHeight, fullWidth * 0.35f, fullHeight);

  // --------------------
  // |    messageArea   |
  // |        +         |
  // | messageValueArea |
  // --------------------
  // | fpsArea | msArea |
  // --------------------
  messageArea_[VerticalAlignment::LOWER] = D2D1::RectF(0.0f, 0.0f, fullWidth, fullHeight - lineHeight);
  messageValueArea_[VerticalAlignment::LOWER] = D2D1::RectF(0.0f, 0.0f, fullWidth * 0.35f, fullHeight - lineHeight);
  fpsArea_[VerticalAlignment::LOWER] = D2D1::RectF(0.0f, fullHeight - lineHeight, halfWidth, fullHeight);
  msArea_[VerticalAlignment::LOWER] = D2D1::RectF(halfWidth, fullHeight - lineHeight, fullWidth, fullHeight);

  // Full area is not depending on vertical alignment.
  fullArea_.d2d1 = D2D1::RectF(0.0f, 0.0f, fullWidth, fullHeight);
  fullArea_.wic = { 0, 0, fullWidth_, fullHeight_ };
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
  if (IsLowerOverlayPosition(overlayPosition))
  {
    currentAlignment_ = VerticalAlignment::LOWER;
    screenPosition_.y = screenHeight_ - fullHeight_ - offset_;
  }
  else
  {
    currentAlignment_ = VerticalAlignment::UPPER;
    screenPosition_.y = offset_;
  }

  if (IsLeftOverlayPosition(overlayPosition))
  {
    screenPosition_.x = offset_;
  }
  else
  {
    screenPosition_.x = screenWidth_ - fullWidth_ - offset_;
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
  if (RecordingState::GetInstance().Started()) 
  {
    performanceCounter_.Start();
  }
  const auto frameInfo = performanceCounter_.NextFrame();
  if (RecordingState::GetInstance().Stopped()) 
  {
    performanceCounter_.Stop();
  }

  UpdateScreenPosition();
  renderTarget_->Clear(clearColor_); // clear full bitmap
  if (RecordingState::GetInstance().DisplayOverlay())
  {
    DrawFrameInfo(frameInfo);
    DrawMessages(textureState);
  }
}

void OverlayBitmap::DrawFrameInfo(const GameOverlay::PerformanceCounter::FrameInfo& frameInfo)
{
  // fps counter
  renderTarget_->PushAxisAlignedClip(fpsArea_[currentAlignment_], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(fpsBackgroundColor_);
  fpsMessage_[currentAlignment_]->WriteMessage(frameInfo.fps, L" FPS");
  fpsMessage_[currentAlignment_]->SetText(writeFactory_.Get(), textFormat_.Get());
  fpsMessage_[currentAlignment_]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();

  // ms counter
  renderTarget_->PushAxisAlignedClip(msArea_[currentAlignment_], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(msBackgroundColor_);

  msMessage_[currentAlignment_]->WriteMessage(frameInfo.ms, L" ms", precision_);
  msMessage_[currentAlignment_]->SetText(writeFactory_.Get(), textFormat_.Get());
  msMessage_[currentAlignment_]->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::DrawMessages(TextureState textureState)
{
  if (textureState == TextureState::DEFAULT)
  {
    return;
  }

  renderTarget_->PushAxisAlignedClip(messageArea_[currentAlignment_], D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(messageBackgroundColor_);
  if (textureState == TextureState::START)
  {
    stateMessage_[currentAlignment_]->WriteMessage(L"Capture Started");
    stateMessage_[currentAlignment_]->SetText(writeFactory_.Get(), messageFormat_.Get());
    stateMessage_[currentAlignment_]->Draw(renderTarget_.Get());
  }
  else if (textureState == TextureState::STOP)
  {
    const auto capture = performanceCounter_.GetLastCaptureResults();
    stateMessage_[currentAlignment_]->WriteMessage(L"Capture Ended\n");
    stateMessage_[currentAlignment_]->SetText(writeFactory_.Get(), messageFormat_.Get());
    stateMessage_[currentAlignment_]->Draw(renderTarget_.Get());

    stopValueMessage_[currentAlignment_]->WriteMessage(capture.averageFPS, L"\n", precision_);
    stopValueMessage_[currentAlignment_]->WriteMessage(capture.averageMS, L"\n", precision_);
    stopValueMessage_[currentAlignment_]->WriteMessage(capture.frameTimePercentile, L"", precision_);
    stopValueMessage_[currentAlignment_]->SetText(writeFactory_.Get(), stopValueFormat_.Get());
    stopValueMessage_[currentAlignment_]->Draw(renderTarget_.Get());

    stopMessage_[currentAlignment_]->WriteMessage(L"FPS Average\n");
    stopMessage_[currentAlignment_]->WriteMessage(L"ms  Average\n");
    stopMessage_[currentAlignment_]->WriteMessage(L"99th Percentile");
    stopMessage_[currentAlignment_]->SetText(writeFactory_.Get(), stopMessageFormat_.Get());
    stopMessage_[currentAlignment_]->Draw(renderTarget_.Get());
  }
  renderTarget_->PopAxisAlignedClip();
}

void OverlayBitmap::FinishRendering()
{
  HRESULT hr = renderTarget_->EndDraw();
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayBitmap", "EndDraw failed, HRESULT", hr);
  }
}

OverlayBitmap::RawData OverlayBitmap::GetBitmapDataRead()
{
  if (bitmapLock_) 
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayBitmap", "Bitmap lock was not released");
  }
  bitmapLock_.Reset();

  RawData rawData = {};
  const auto& currBitmapArea = fullArea_.wic;
  HRESULT hr = bitmap_->Lock(&currBitmapArea, WICBitmapLockRead, &bitmapLock_);
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayBitmap", "Bitmap lock failed, HRESULT", hr);
    return rawData;
  }

  hr = bitmapLock_->GetDataPointer(&rawData.size, &rawData.dataPtr);
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayBitmap",
                     "Bitmap lock GetDataPointer failed, HRESULT", hr);
    rawData = {};
  }
  return rawData;
}

void OverlayBitmap::UnlockBitmapData() 
{ 
  bitmapLock_.Reset(); 
}

int OverlayBitmap::GetFullWidth() const
{
  return fullWidth_;
}

int OverlayBitmap::GetFullHeight() const
{
  return fullHeight_;
}

OverlayBitmap::Position OverlayBitmap::GetScreenPos() const
{
  return screenPosition_;
}

const D2D1_RECT_F & OverlayBitmap::GetCopyArea() const
{
  return fullArea_.d2d1;
}

VkFormat OverlayBitmap::GetVKFormat() const
{
  return VK_FORMAT_B8G8R8A8_UNORM;
}

bool OverlayBitmap::InitFactories()
{
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", 
      "D2D1CreateFactory failed, HRESULT", hr);
    return false;
  }

  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory_),
                           reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", 
      "DWriteCreateFactory failed, HRESULT", hr);
    return false;
  }

  hr = CoInitialize(NULL);
  if (hr == S_OK || hr == S_FALSE) 
  {
    coInitialized_ = true;
  }
  else 
  {
    g_messageLog.Log(MessageLog::LOG_WARNING, "OverlayBitmap", "CoInitialize failed, HRESULT", hr);
  }

  hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&iwicFactory_));
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", 
      "CoCreateInstance failed, HRESULT", hr);
    return false;
  }
  return true;
}

bool OverlayBitmap::InitBitmap()
{
  HRESULT hr = iwicFactory_->CreateBitmap(fullWidth_, fullHeight_, GUID_WICPixelFormat32bppPBGRA,
                                          WICBitmapCacheOnLoad, &bitmap_);
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", "CreateBitmap failed, HRESULT", hr);
    return false;
  }

  const auto rtProperties = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f,
      D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

  hr = d2dFactory_->CreateWicBitmapRenderTarget(bitmap_.Get(), rtProperties, &renderTarget_);
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap",
                     "CreateWicBitmapRenderTarget failed, HRESULT", hr);
    return false;
  }

  hr = renderTarget_->CreateSolidColorBrush(fontColor_, &textBrush_);
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", 
      "CreateTextFormat failed, HRESULT", hr);
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

  InitTextForAlignment(VerticalAlignment::LOWER);
  InitTextForAlignment(VerticalAlignment::UPPER);
  return true;
}

void OverlayBitmap::InitTextForAlignment(VerticalAlignment verticalAlignment)
{
  auto fpsArea = fpsArea_[verticalAlignment];
  fpsMessage_[verticalAlignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  fpsMessage_[verticalAlignment]->SetArea(
    fpsArea.left, fpsArea.top,
    fpsArea.right - fpsArea.left,
    fpsArea.bottom - fpsArea.top);

  auto msArea = msArea_[verticalAlignment];
  msMessage_[verticalAlignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  msMessage_[verticalAlignment]->SetArea(
    msArea.left, msArea.top,
    msArea.right - msArea.left,
    msArea.bottom - msArea.top);

  const auto offset2 = offset_ * 2;

  auto messageArea = messageArea_[verticalAlignment];
  stateMessage_[verticalAlignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stateMessage_[verticalAlignment]->SetArea(
    messageArea.left + offset2, messageArea.top + offset_,
    messageArea.right - messageArea.left - offset2,
    messageArea.bottom - messageArea.top - offset_);

  auto messageValueArea = messageValueArea_[verticalAlignment];
  stopValueMessage_[verticalAlignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopValueMessage_[verticalAlignment]->SetArea(
    messageValueArea.left + offset2, messageValueArea.top + offset_,
    messageValueArea.right - messageValueArea.left - offset2,
    messageValueArea.bottom - messageValueArea.top - offset_);

  stopMessage_[verticalAlignment].reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopMessage_[verticalAlignment]->SetArea(
    messageValueArea.right + offset2, messageArea.top + offset_,
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
  if (FAILED(hr)) 
  {
    g_messageLog.Log(MessageLog::LOG_ERROR, "OverlayBitmap", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }
  textFormat->SetTextAlignment(textAlignment);
  textFormat->SetParagraphAlignment(paragraphAlignment);
  return textFormat;
}