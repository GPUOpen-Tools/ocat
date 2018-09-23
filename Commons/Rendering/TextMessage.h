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
#include <wincodec.h>
#include <wrl.h>
#include <string>
#include <vector>

#include "../Logging/MessageLog.h"

class TextMessage final {
 public:
  TextMessage(ID2D1RenderTarget* renderTarget, const D2D1_COLOR_F& textColor,
              const D2D1_COLOR_F& numberColor);
  ~TextMessage();

  TextMessage(const TextMessage&) = delete;
  TextMessage& operator=(const TextMessage&) = delete;

  void SetArea(float x, float y, float width, float height);

  void WriteMessage(const std::wstring& msg);
  void WriteMessage(float value, const std::wstring& msg, int precision);
  void WriteMessage(int value, const std::wstring& msg);

  void SetText(IDWriteFactory* writeFactory, IDWriteTextFormat* textFormat);
  void Draw(ID2D1RenderTarget* renderTarget);

 private:
  Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout_;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> numberBrush_;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;

  std::wstringstream message_;
  std::vector<DWRITE_TEXT_RANGE> numberRanges_;

  D2D1_POINT_2F screenPos_;
  float maxWidth_;
  float maxHeight_;
};
