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
 public
  struct tagKBDLLHOOKSTRUCT
  {
   public
    int vkCode;
   public
    int scanCode;
   public
    int flags;
   public
    int time;
   public
    int dwExtraInfo;
  };

 public
  delegate int LowLevelKeyboardProc(int nCode, int wParam, ref tagKBDLLHOOKSTRUCT lParam);

  // C++ functions
  [DllImport("kernel32.dll")] public static extern IntPtr LoadLibrary(string lpFileName);
  [DllImport("user32.dll")]  static extern IntPtr SetWindowsHookEx(int idHook,
                                                                  LowLevelKeyboardProc lpfn,
                                                                  IntPtr hMod, uint dwThreadId);
  [DllImport("user32.dll")]  static extern int CallNextHookEx(IntPtr hhk, int nCode, int wParam,
                                                              tagKBDLLHOOKSTRUCT lParam);
  [DllImport("user32.dll")] static extern bool UnhookWindowsHookEx(IntPtr hhk);
  [DllImport("kernel32.dll")] static extern int GetLastError();
  [DllImport("user32.dll")] static extern bool RegisterHotKey(IntPtr hWnd, int id, int fsModifiers, int vlc);
  [DllImport("user32.dll")] static extern bool UnregisterHotKey(IntPtr hWnd, int id);

 const int WH_KEYBOARD_LL = 13;
 const int HC_ACTION = 0;
 const int WM_KEYDOWN = 0x100;
 const int WM_SYSKEYDOWN = 0x104;

 private
  int keyCode_;
  IntPtr globalHook_ = IntPtr.Zero;
  IntPtr windowHandler_ = IntPtr.Zero;
  bool handled_ = false;
 private
  LowLevelKeyboardProc keyBoardHookProcDelegate_;

 public
  KeyboardHook() { keyBoardHookProcDelegate_ = KeyboardProc; }
  ~KeyboardHook() { UnHook(); }
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
      windowHandler_ = handler;
    }

    return Hook(newKeyCode);
  }

 public
  void UnHook()
  {
    // first unregister hotkey and then unhook windows hook
    if (windowHandler_ != IntPtr.Zero && !UnregisterHotKey(windowHandler_, keyCode_))
    {
      // window will be invalid if OCAT gets closed
      // int error = GetLastError();
      // MessageBox.Show("UnregisterHotKey failed " + error.ToString());
    }
    windowHandler_ = IntPtr.Zero;

    if (globalHook_ != IntPtr.Zero && !UnhookWindowsHookEx(globalHook_))
    {
      int error = GetLastError();
      MessageBox.Show("UnhookWindowsHookEx failed " + error.ToString());
    }

    globalHook_ = IntPtr.Zero;
  }

 private
  bool Hook(int newKeyCode)
  {
    IntPtr hMod = LoadLibrary("kernel32.dll");
    if (hMod == IntPtr.Zero)
    {
      MessageBox.Show("Load Library failed");
      return false;
    }

    globalHook_ = SetWindowsHookEx(WH_KEYBOARD_LL, keyBoardHookProcDelegate_, hMod, 0);
    if (globalHook_ == IntPtr.Zero)
    {
      int error = GetLastError();
      MessageBox.Show("SetWindowsHookEx failed " + error.ToString());
      return false;
    }

    if (!RegisterHotKey(windowHandler_, newKeyCode, 0, newKeyCode))
    {
      int error = GetLastError();
      if (newKeyCode == 0x7B)
      {
        MessageBox.Show(
          "RegisterHotKey failed, F12 is a reserved key and cannot be used as hotkey " + error.ToString());
      }
      else
      {
        MessageBox.Show("RegisterHotKey failed " + error.ToString());
      }
      windowHandler_ = IntPtr.Zero;
      // TODO: we only want both hooks registered successfully ?
      UnhookWindowsHookEx(globalHook_);
      globalHook_ = IntPtr.Zero;
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
  int KeyboardProc(int nCode, int wParam, ref tagKBDLLHOOKSTRUCT lParam)
  {
    if (nCode == HC_ACTION)
    {
      if (lParam.vkCode == keyCode_ && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
        (HotkeyDownEvent != null))
      {
        HotkeyDownEvent();
        handled_ = true;
      }
    }
    return CallNextHookEx(globalHook_, nCode, wParam, lParam);
  }

 public
  void OnHotKeyEvent(long lParam)
  {
    if (lParam == keyCode_ && (HotkeyDownEvent != null))
    {
      if (!handled_)
        HotkeyDownEvent();
      handled_ = false;
    }
  }
}
}