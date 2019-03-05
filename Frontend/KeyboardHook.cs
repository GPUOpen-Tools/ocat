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
using System.Runtime.InteropServices;
using System.Windows;

namespace Frontend {
class KeyboardHook {
  [DllImport("kernel32.dll")] static extern int GetLastError();
  [DllImport("user32.dll")] static extern bool RegisterHotKey(IntPtr hWnd, int id, int fsModifiers, int vlc);
  [DllImport("user32.dll")] static extern bool UnregisterHotKey(IntPtr hWnd, int id);

 private
  int keyCode_;
  IntPtr globalHook_ = IntPtr.Zero;

 public
  KeyboardHook() { }
  ~KeyboardHook() { }
 public
  bool ActivateHook(int newKeyCode, IntPtr handler)
  {
    if (globalHook_ != IntPtr.Zero) {
      // already set return true
      if (keyCode_ == newKeyCode) {
        return true;
      }
      // unhook old
      else {
        UnHook();
      }
    }
    else {
      globalHook_ = handler;
    }

    return Hook(newKeyCode);
  }

 public
  void UnHook()
  {
    if (globalHook_ != IntPtr.Zero && !UnregisterHotKey(globalHook_, keyCode_))
    {
      int error = GetLastError();
      MessageBox.Show("UnregisterHotKey failed " + error.ToString());
    }

    globalHook_ = IntPtr.Zero;
  }

 private
  bool Hook(int newKeyCode)
  {
    if (!RegisterHotKey(globalHook_, newKeyCode, 0, newKeyCode))
    {
      int error = GetLastError();
      MessageBox.Show("RegisterHotKey failed " + error.ToString());
      return false;
    }

    keyCode_ = newKeyCode;
    return true;
  }

 public
  delegate void KeyboardDownEvent();
 public
  event KeyboardDownEvent HotkeyDownEvent;

  public
  void OnHotKeyEvent(long lParam)
  {
    if (lParam == keyCode_ && (HotkeyDownEvent != null))
    {
      HotkeyDownEvent();
    }
  }
}
}