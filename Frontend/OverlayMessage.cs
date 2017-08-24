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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Frontend
{
    // Keep in sync with OverlayMessageType in Backend.
    enum OverlayMessageType
  {
    OVERLAY_StartRecording,
    OVERLAY_StopRecording,
    OVERLAY_FreeLibrary,
    OVERLAY_AttachDll,
    OVERLAY_DetachDll,
    OVERLAY_ThreadInitialized,
    OVERLAY_ThreadTerminating,
    OVERLAY_Initialized,
    OVERLAY_ShowOverlay, // Visibility of the overlay while active
    OVERLAY_HideOverlay, // Visibility of the overlay while active
    OVERLAY_PositionUpperLeft,
    OVERLAY_PositionUpperRight,
    OVERLAY_PositionLowerLeft,
    OVERLAY_PositionLowerRight
    };

  class OverlayMessage
  {
    public const int WM_APP = 0x8000;
    public const int overlayMessage = WM_APP + 1;
  }
}
