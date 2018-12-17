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

#include "TextMessage.h"

#include <iomanip>

TextMessage::~TextMessage()
{
  textLayout_.Reset();
  numberBrush_.Reset();
  textBrush_.Reset();
}

TextMessage::TextMessage(ID2D1RenderTarget* renderTarget, const D2D1_COLOR_F& textColor,
                         const D2D1_COLOR_F& numberColor)
{
  HRESULT hr = renderTarget->CreateSolidColorBrush(textColor, &textBrush_);
  if (FAILED(hr)) 
  {
    g_messageLog.LogError("TextMessage", "CreateTextFormat failed, HRESULT", hr);
  }

  hr = renderTarget->CreateSolidColorBrush(numberColor, &numberBrush_);
  if (FAILED(hr)) 
  {
    g_messageLog.LogError("TextMessage", "CreateTextFormat failed, HRESULT", hr);
  }
}

void TextMessage::WriteMessage(const std::wstring& msg) 
{ 
  message_ << msg; 
}

void TextMessage::WriteMessage(float value, const std::wstring& msg, int precision)
{
  message_ << std::fixed << std::setprecision(precision);
  const auto start = static_cast<UINT32>(message_.tellp());
  message_ << value;
  numberRanges_.push_back({start, static_cast<UINT32>(message_.tellp()) - start});
  message_ << msg;
}

void TextMessage::WriteMessage(int value, const std::wstring& msg)
{
  const auto start = static_cast<UINT32>(message_.tellp());
  message_ << value;
  numberRanges_.push_back({start, static_cast<UINT32>(message_.tellp()) - start});
  message_ << msg;
}

void TextMessage::WriteMessage(const std::wstring& msgA, const std::wstring& msgB)
{
  const auto start = static_cast<UINT32>(message_.tellp());
  message_ << msgA;
  numberRanges_.push_back({start, static_cast<UINT32 > (message_.tellp()) - start});
  message_ << msgB;
}

void TextMessage::SetText(IDWriteFactory* writeFactory, IDWriteTextFormat* textFormat)
{
  textLayout_ = nullptr;
  const auto message = message_.str();

  HRESULT hr = writeFactory->CreateTextLayout(message.c_str(), static_cast<UINT32>(message.size()),
                                              textFormat, maxWidth_, maxHeight_, &textLayout_);
  if (FAILED(hr)) 
  {
    g_messageLog.LogError("TextMessage", "CreateTextLayout failed, HRESULT", hr);
    textLayout_ = nullptr;
  }

  if (textLayout_) 
  {
    textLayout_->SetIncrementalTabStop(50.0f);
  }

  message_.clear();
  message_.str(std::wstring());
}

void TextMessage::SetArea(float x, float y, float width, float height)
{
  screenPos_.x = x;
  screenPos_.y = y;
  maxWidth_ = width;
  maxHeight_ = height;
}

void TextMessage::Draw(ID2D1RenderTarget* renderTarget)
{
  if (textLayout_) 
  {
    for (const auto& range : numberRanges_) 
    {
      HRESULT hr = textLayout_->SetDrawingEffect(numberBrush_.Get(), range);
      if (FAILED(hr)) 
      {
        g_messageLog.LogError("TextMessage", 
          "SetDrawingEffect failed HRESULT", hr);
      }
      hr = textLayout_->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
    }
    renderTarget->DrawTextLayout(screenPos_, textLayout_.Get(), textBrush_.Get(),
                                 D2D1_DRAW_TEXT_OPTIONS_CLIP);

    numberRanges_.clear();
  }
}