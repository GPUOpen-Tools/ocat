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

#include <windows.h>

// Keep in sync with OverlayMessageType in Frontend.
enum class OverlayMessageType
{
  StartRecording,
  StopRecording,
  FreeLibrary,
  AttachDll,
  DetachDll,
  ThreadInitialized,
  ThreadTerminating,
  Initialized,
  // Visibility of the overlay while active
  ShowOverlay, 
  HideOverlay,
  // Position of the overlay while active
  UpperLeft,
  UpperRight,
  LowerLeft,
  LowerRight
};

class OverlayMessage
{
public:
  // Send a message to the frontend window.
  static bool PostFrontendMessage(HWND window, OverlayMessageType type, LPARAM message);
  // Send a message to a specific overlay thread.
  static bool PostOverlayMessage(DWORD threadID, OverlayMessageType type, LPARAM message);
  static const unsigned int overlayMessageType = WM_APP + 1;
};

