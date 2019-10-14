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

#include "OverlayPosition.h"

OverlayPosition GetOverlayPositionFromMessageType(OverlayMessageType messageType)
{
  switch (messageType)
  {
  case OverlayMessageType::UpperLeft:
    return OverlayPosition::UpperLeft;
  case OverlayMessageType::UpperRight:
    return OverlayPosition::UpperRight;
  case OverlayMessageType::LowerLeft:
    return OverlayPosition::LowerLeft;
  case OverlayMessageType::LowerRight:
    return OverlayPosition::LowerRight;
  default:
    return OverlayPosition::UpperRight;
  }
}

OverlayPosition GetOverlayPositionFromUint(unsigned int overlayPosition)
{
  // Explicitly state the enums, as we do not want to give out an unexpected but valid enum.
  switch (overlayPosition)
  {
  case 0:
    return OverlayPosition::UpperLeft;
  case 1:
    return OverlayPosition::UpperRight;
  case 2:
    return OverlayPosition::LowerLeft;
  case 3:
    return OverlayPosition::LowerRight;
  default:
    return OverlayPosition::UpperRight;
  }
}

bool IsLowerOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LowerLeft) || (overlayPosition == OverlayPosition::LowerRight);
}

bool IsLeftOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LowerLeft) || (overlayPosition == OverlayPosition::UpperLeft);
}


