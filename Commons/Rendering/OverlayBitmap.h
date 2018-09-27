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
#include <vulkan/vulkan.h>
#include <wincodec.h>
#include <wrl.h>
#include <string>

#include "../Recording/PerformanceCounter.hpp"
#include "../Recording/RecordingState.h"
#include "TextMessage.h"

// Render overlay text with background into a bitmap.
class OverlayBitmap final 
{
public:
  OverlayBitmap(const OverlayBitmap&) = delete;
  OverlayBitmap& operator=(const OverlayBitmap&) = delete;
  
  struct RawData 
  {
    unsigned char* dataPtr = nullptr;
    UINT size = 0;
    
    RawData();
  };

  struct Position 
  {
    int x;
    int y;
  };

  OverlayBitmap();
  ~OverlayBitmap();

  bool Init(int screenWidth, int screenHeight);
  void Resize(int screenWidth, int screenHeight);
  void DrawOverlay();

  // Locks the bitmap data and returns a pointer to it, UnlockBitmapData needs to be called
  // afterwards
  RawData GetBitmapDataRead();
  void UnlockBitmapData();

  int GetFullWidth() const;
  int GetFullHeight() const;
  Position GetScreenPos() const;
  const D2D1_RECT_F& GetCopyArea() const;
  VkFormat GetVKFormat() const;

private:
  struct Area 
  {
    D2D1_RECT_F d2d1;
    WICRect wic;
  };

  enum class Alignment
  {
    UpperLeft, // = 0
	UpperRight, // = 1
    LowerLeft, // = 2
	LowerRight // = 3
  };

  void CalcSize(int screenWidth, int screenHeight);
  bool InitFactories();
  bool InitBitmap();
  bool InitText();
  void UpdateScreenPosition();
  void InitTextForAlignment(Alignment alignment);

  void Update();
  void StartRendering();
  void DrawFrameInfo(const GameOverlay::PerformanceCounter::FrameInfo& frameInfo);
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
  static const D2D1_COLOR_F recordingColor_;

  GameOverlay::PerformanceCounter performanceCounter_;

  Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
  Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget_;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;

  Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> messageFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> stopValueFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> stopMessageFormat_;
  Microsoft::WRL::ComPtr<IDWriteTextFormat> recordingMessageFormat_;

  Microsoft::WRL::ComPtr<IWICImagingFactory> iwicFactory_;
  Microsoft::WRL::ComPtr<IWICBitmap> bitmap_;
  Microsoft::WRL::ComPtr<IWICBitmapLock> bitmapLock_;

  static const int alignmentCount_ = 4;
  std::unique_ptr<TextMessage> fpsMessage_[alignmentCount_];
  std::unique_ptr<TextMessage> msMessage_[alignmentCount_];
  std::unique_ptr<TextMessage> stateMessage_[alignmentCount_];
  std::unique_ptr<TextMessage> stopValueMessage_[alignmentCount_];
  std::unique_ptr<TextMessage> stopMessage_[alignmentCount_];
  std::unique_ptr<TextMessage> recordingMessage_[alignmentCount_];

  int fullWidth_;
  int fullHeight_;
  int screenWidth_;
  int screenHeight_;

  // always the upper left corner of the full copy area on the screen.
  Position screenPosition_;
  int precision_ = 1;

  Area fullArea_;
  D2D1_RECT_F messageArea_[alignmentCount_];
  D2D1_RECT_F messageValueArea_[alignmentCount_];
  D2D1_RECT_F fpsArea_[alignmentCount_];
  D2D1_RECT_F msArea_[alignmentCount_];
  D2D1_RECT_F recordingArea_[alignmentCount_];

  int lineHeight_ = 45;
  int offset_ = 5;

  bool coInitialized_ = false;
  Alignment currentAlignment_ = Alignment::UpperLeft;

  bool recording_ = false;
};
