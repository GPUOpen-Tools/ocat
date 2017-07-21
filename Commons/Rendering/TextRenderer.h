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

#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <vulkan\vulkan.h>
#include <wincodec.h>
#include <wrl.h>
#include <string>

#include "..\Recording\PerformanceCounter.hpp"
#include "..\Recording\RecordingState.h"
#include "TextMessage.h"

class TextRenderer final {
 public:
  TextRenderer(const TextRenderer&) = delete;
  TextRenderer& operator=(const TextRenderer&) = delete;
  struct RawData {
    unsigned char* dataPtr;
    UINT size;
    RawData();
  };

  struct Position {
    int x;
    int y;
  };

  TextRenderer(int screenWidth, int screenHeight);
  ~TextRenderer();

  void Resize(int screenWidth, int screenHeight);
  void DrawOverlay();

  // Locks the bitmap data and returns a pointer to it, UnlockBitmapData needs to be called
  // afterwards
  RawData GetBitmapDataRead();
  void UnlockBitmapData();

  int GetFullWidth() const { return fullWidth_; }
  int GetFullHeight() const { return fullHeight_; }
  Position GetScreenPos() const { return screenPosition_; }
  const D2D1_RECT_F& GetCopyArea() const
  {
    return copyFullArea_ ? fullArea_.d2d1 : perFrameArea_.d2d1;
  }
  bool CopyFullArea() const { return copyFullArea_; }
  VkFormat GetVKFormat() { return VK_FORMAT_B8G8R8A8_UNORM; }
 private:
  struct Area {
    D2D1_RECT_F d2d1;
    WICRect wic;
  };

  void CalcSize(int screenWidth, int screenHeight);
  bool InitFactories();
  bool InitBitmap();
  bool InitText();

  void Update();
  void StartRendering();
  void DrawFrameInfo(const gameoverlay::PerformanceCounter::FrameInfo& frameInfo);
  void DrawMessages(TextureState textureState);
  void FinishRendering();

  IDWriteTextFormat* CreateTextFormat(float size, DWRITE_TEXT_ALIGNMENT textAlignment,
                                      DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment);

  static const D2D1_COLOR_F clearColor_;
  static const D2D1_COLOR_F fpsBackgroundColor_;
  static const D2D1_COLOR_F msBackgroundColor_;
  static const D2D1_COLOR_F messageBackgroundColor_;
  static const D2D1_COLOR_F fontColor_;
  static const D2D1_COLOR_F numberColor_;

  gameoverlay::PerformanceCounter performanceCounter_;

  Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
  Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget_;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;

  Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> messageFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> stopValueFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> stopMessageFormat_;

  Microsoft::WRL::ComPtr<IWICImagingFactory> iwicFactory_;
  Microsoft::WRL::ComPtr<IWICBitmap> bitmap_;
  Microsoft::WRL::ComPtr<IWICBitmapLock> bitmapLock_;

  std::unique_ptr<TextMessage> fpsMessage_;
  std::unique_ptr<TextMessage> msMessage_;
  std::unique_ptr<TextMessage> stateMessage_;
  std::unique_ptr<TextMessage> stopValueMessage_;
  std::unique_ptr<TextMessage> stopMessage_;

  TextureState prevTextureState_ = TextureState::UNDEFINED;
  int fullWidth_;
  int fullHeight_;
  Position screenPosition_;
  int precision_ = 1;

  Area fullArea_;
  Area perFrameArea_;
  D2D1_RECT_F messageArea_;
  D2D1_RECT_F messageValueArea_;
  D2D1_RECT_F fpsArea_;
  D2D1_RECT_F msArea_;
  bool copyFullArea_ = false;
  int lineHeight_ = 45;
  int offset_ = 5;

  bool coInitialized_ = false;
};
