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

#include "Capturing.h"

#include <tlhelp32.h>
#include <thread>
#include "../Utility/Constants.h"
#include "OverlayThread.h"
#include "../Logging/MessageLog.h"
#include "../Utility/ProcessHelper.h"
#include "RecordingState.h"
#include "../Overlay/VK_Environment.h"
#include "../Utility/FileDirectory.h"
#include "../Utility/StringUtils.h"

namespace GameOverlay {
  Config g_config;
  OverlayThread g_overlayThread;

#if _WIN64
  const std::wstring g_overlayLibName = L"GameOverlay64.dll";
#else
  const std::wstring g_overlayLibName = L"GameOverlay32.dll";
#endif

  void InitLogging(const std::string& callerName)
  {
    g_messageLog.Start(g_fileDirectory.GetDirectory(DirectoryType::Log) + L"GameOverlayLog",
      ConvertUTF8StringToUTF16String(callerName));
  }

  void InitCapturing()
  {
    static bool initialized = false;
    if (!initialized) {
      g_config.Load(g_fileDirectory.GetDirectory(DirectoryType::Config));
      RecordingState::GetInstance().SetDisplayTimes(g_config.startDisplayTime_,
        g_config.endDisplayTime_);
      RecordingState::GetInstance().SetRecordingTime(static_cast<float>(g_config.recordingTime_));
      const auto overlayPosition = GetOverlayPositionFromUint(g_config.overlayPosition_);
      RecordingState::GetInstance().SetOverlayPosition(overlayPosition);
      g_overlayThread.Start();
      initialized = true;
    }
  }
}
