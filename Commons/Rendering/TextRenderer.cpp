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

#include "TextRenderer.h"
#include "..\Logging\MessageLog.h"

#include "..\Recording\PerformanceCounter.hpp"
#include "..\Recording\RecordingState.h"

#include <sstream>

using namespace Microsoft::WRL;

const D2D1_COLOR_F TextRenderer::clearColor_ = {0.0f, 0.0f, 0.0f, 0.0f};
const D2D1_COLOR_F TextRenderer::fpsBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.8f};
const D2D1_COLOR_F TextRenderer::msBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.7f};
const D2D1_COLOR_F TextRenderer::messageBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.5f};
const D2D1_COLOR_F TextRenderer::fontColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
const D2D1_COLOR_F TextRenderer::numberColor_ = {1.0f, 162.0f / 255.0f, 26.0f / 255.0f, 1.0f};

TextRenderer::RawData::RawData() : dataPtr{nullptr}, size{0} {}
TextRenderer::TextRenderer(int screenWidth, int screenHeight)
{
  if (!InitFactories()) {
    throw std::runtime_error("Could not initialize factories");
  }

  CalcSize(screenWidth, screenHeight);

  if (!InitBitmap()) {
    throw std::runtime_error("Could not initialize Bitmap");
  }

  if (!InitText()) {
    throw std::runtime_error("Could not initialize Text");
  }
}

TextRenderer::~TextRenderer()
{
  fpsMessage_.reset();
  msMessage_.reset();
  stateMessage_.reset();
  stopValueMessage_.reset();
  stopMessage_.reset();
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

void TextRenderer::CalcSize(int screenWidth, int screenHeight)
{
  fullWidth_ = 256;
  fullHeight_ = lineHeight_ * 4;

  Resize(screenWidth, screenHeight);

  perFrameArea_.d2d1 =
      D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(fullWidth_), static_cast<FLOAT>(lineHeight_));
  perFrameArea_.wic = {0, 0, fullWidth_, lineHeight_};
  const auto halfWidth = static_cast<FLOAT>(fullWidth_ / 2);
  fpsArea_ = D2D1::RectF(0.0f, 0.0f, halfWidth, static_cast<FLOAT>(lineHeight_));
  msArea_ =
      D2D1::RectF(halfWidth, 0, static_cast<FLOAT>(fullWidth_), static_cast<FLOAT>(lineHeight_));

  messageArea_ = D2D1::RectF(0.0f, static_cast<FLOAT>(lineHeight_), static_cast<FLOAT>(fullWidth_),
                             static_cast<FLOAT>(fullHeight_));
  messageValueArea_ =
      D2D1::RectF(0.0f, static_cast<FLOAT>(lineHeight_), static_cast<FLOAT>(fullWidth_ * 0.35f),
                  static_cast<FLOAT>(fullHeight_));

  fullArea_.d2d1 =
      D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(fullWidth_), static_cast<FLOAT>(fullHeight_));
  fullArea_.wic = {0, 0, fullWidth_, fullHeight_};
}

void TextRenderer::Resize(int screenWidth, int screenHeight)
{
  screenPosition_.x = screenWidth - fullWidth_ - offset_;
  screenPosition_.y = offset_;
}

void TextRenderer::DrawOverlay()
{
  StartRendering();

  Update();

  FinishRendering();
}

void TextRenderer::StartRendering()
{
  renderTarget_->BeginDraw();

  renderTarget_->SetTransform(D2D1::IdentityMatrix());
}

void TextRenderer::Update()
{
  const auto textureState = RecordingState::GetInstance().Update();
  if (RecordingState::GetInstance().Started()) {
    performanceCounter_.Start();
  }
  const auto frameInfo = performanceCounter_.NextFrame();
  if (RecordingState::GetInstance().Stopped()) {
    performanceCounter_.Stop();
  }

  DrawFrameInfo(frameInfo);
  DrawMessages(textureState);
}

void TextRenderer::DrawFrameInfo(const gameoverlay::PerformanceCounter::FrameInfo& frameInfo)
{
  // fps counter
  renderTarget_->PushAxisAlignedClip(fpsArea_, D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(fpsBackgroundColor_);

  fpsMessage_->WriteMessage(frameInfo.fps, L" FPS");
  fpsMessage_->SetText(writeFactory_.Get(), textFormat_.Get());
  fpsMessage_->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();

  // ms counter
  renderTarget_->PushAxisAlignedClip(msArea_, D2D1_ANTIALIAS_MODE_ALIASED);
  renderTarget_->Clear(msBackgroundColor_);

  msMessage_->WriteMessage(frameInfo.ms, L" ms", precision_);
  msMessage_->SetText(writeFactory_.Get(), textFormat_.Get());
  msMessage_->Draw(renderTarget_.Get());

  renderTarget_->PopAxisAlignedClip();
}

void TextRenderer::DrawMessages(TextureState textureState)
{
  if (textureState != prevTextureState_) {
    renderTarget_->PushAxisAlignedClip(messageArea_, D2D1_ANTIALIAS_MODE_ALIASED);

    copyFullArea_ = true;

    if (textureState != TextureState::DEFAULT) {
      renderTarget_->Clear(messageBackgroundColor_);
      if (textureState == TextureState::START) {
        stateMessage_->WriteMessage(L"Capture Started");
        stateMessage_->SetText(writeFactory_.Get(), messageFormat_.Get());
        stateMessage_->Draw(renderTarget_.Get());
      }
      else if (textureState == TextureState::STOP) {
        const auto capture = performanceCounter_.GetLastCaptureResults();
        stateMessage_->WriteMessage(L"Capture Ended\n");
        stateMessage_->SetText(writeFactory_.Get(), messageFormat_.Get());
        stateMessage_->Draw(renderTarget_.Get());

        stopValueMessage_->WriteMessage(capture.averageFPS, L"\n", precision_);
        stopValueMessage_->WriteMessage(capture.averageMS, L"\n", precision_);
        stopValueMessage_->WriteMessage(capture.frameTimePercentile, L"", precision_);
        stopValueMessage_->SetText(writeFactory_.Get(), stopValueFormat_.Get());
        stopValueMessage_->Draw(renderTarget_.Get());

        stopMessage_->WriteMessage(L"FPS Average\n");
        stopMessage_->WriteMessage(L"ms  Average\n");
        stopMessage_->WriteMessage(L"99th Percentile");
        stopMessage_->SetText(writeFactory_.Get(), stopMessageFormat_.Get());
        stopMessage_->Draw(renderTarget_.Get());
      }
    }
    else {
      renderTarget_->Clear(clearColor_);
    }

    renderTarget_->PopAxisAlignedClip();
  }
  else {
    copyFullArea_ = false;
  }

  prevTextureState_ = textureState;
}

void TextRenderer::FinishRendering()
{
  HRESULT hr = renderTarget_->EndDraw();
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "TextRenderer", "EndDraw failed, HRESULT", hr);
  }
}

TextRenderer::RawData TextRenderer::GetBitmapDataRead()
{
  if (bitmapLock_) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "TextRenderer", "Bitmap lock was not released");
  }
  bitmapLock_.Reset();

  RawData rawData{};
  const auto& currBitmapArea = copyFullArea_ ? fullArea_.wic : perFrameArea_.wic;
  HRESULT hr = bitmap_->Lock(&currBitmapArea, WICBitmapLockRead, &bitmapLock_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "TextRenderer", "Bitmap lock failed, HRESULT", hr);
    return rawData;
  }

  hr = bitmapLock_->GetDataPointer(&rawData.size, &rawData.dataPtr);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_WARNING, "TextRenderer",
                     "Bitmap lock GetDataPointer failed, HRESULT", hr);
    rawData = {};
  }
  return rawData;
}

void TextRenderer::UnlockBitmapData() { bitmapLock_.Reset(); }
bool TextRenderer::InitFactories()
{
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "D2D1CreateFactory failed, HRESULT",
                     hr);
    return false;
  }

  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory_),
                           reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "DWriteCreateFactory failed, HRESULT",
                     hr);
    return false;
  }

  hr = CoInitialize(NULL);
  if (hr == S_OK || hr == S_FALSE) {
    coInitialized_ = true;
  }
  else {
    g_messageLog.Log(MessageLog::LOG_WARNING, "TextRenderer", "CoInitialize failed, HRESULT", hr);
  }

  hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&iwicFactory_));
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "CoCreateInstance failed, HRESULT", hr);
    return false;
  }
  return true;
}

bool TextRenderer::InitBitmap()
{
  HRESULT hr = iwicFactory_->CreateBitmap(fullWidth_, fullHeight_, GUID_WICPixelFormat32bppPBGRA,
                                          WICBitmapCacheOnLoad, &bitmap_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "CreateBitmap failed, HRESULT", hr);
    return false;
  }

  const auto rtProperties = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f,
      D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

  hr = d2dFactory_->CreateWicBitmapRenderTarget(bitmap_.Get(), rtProperties, &renderTarget_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer",
                     "CreateWicBitmapRenderTarget failed, HRESULT", hr);
    return false;
  }

  hr = renderTarget_->CreateSolidColorBrush(fontColor_, &textBrush_);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }

  return true;
}

bool TextRenderer::InitText()
{
  textFormat_ =
      CreateTextFormat(25.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  messageFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
  stopValueFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  stopMessageFormat_ =
      CreateTextFormat(20.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  fpsMessage_.reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  fpsMessage_->SetArea(fpsArea_.left, fpsArea_.top, fpsArea_.right - fpsArea_.left,
                       fpsArea_.bottom - fpsArea_.top);
  msMessage_.reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  msMessage_->SetArea(msArea_.left, msArea_.top, msArea_.right - msArea_.left,
                      msArea_.bottom - msArea_.top);

  stateMessage_.reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  const auto offset2 = offset_ * 2;
  stateMessage_->SetArea(messageArea_.left + offset2, messageArea_.top,
                         messageArea_.right - messageArea_.left - offset2,
                         messageArea_.bottom - messageArea_.top);

  stopValueMessage_.reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopValueMessage_->SetArea(messageValueArea_.left + offset2, messageValueArea_.top,
                             messageValueArea_.right - messageValueArea_.left - offset2,
                             messageValueArea_.bottom - messageValueArea_.top);

  stopMessage_.reset(new TextMessage(renderTarget_.Get(), fontColor_, numberColor_));
  stopMessage_->SetArea(messageValueArea_.right + offset2, messageArea_.top,
                        messageArea_.right - messageArea_.left - offset2,
                        messageArea_.bottom - messageArea_.top);

  return true;
}

IDWriteTextFormat* TextRenderer::CreateTextFormat(float size, DWRITE_TEXT_ALIGNMENT textAlignment,
                                                  DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment)
{
  IDWriteTextFormat* textFormat;
  HRESULT hr = writeFactory_->CreateTextFormat(L"Verdane", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                               DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                               size, L"en-us", &textFormat);
  if (FAILED(hr)) {
    g_messageLog.Log(MessageLog::LOG_ERROR, "TextRenderer", "CreateTextFormat failed, HRESULT", hr);
    return false;
  }
  textFormat->SetTextAlignment(textAlignment);
  textFormat->SetParagraphAlignment(paragraphAlignment);
  return textFormat;
}