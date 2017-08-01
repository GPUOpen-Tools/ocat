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

using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using Wrapper;

public enum CaptureMode
{
    CaptureAll,
    CaptureSingle
}

namespace Frontend
{
	public
    class Configuration : INotifyPropertyChanged
    {
        private
         bool readyToRecord_ = false;
        private
         string targetExecutable;
        private
         string loggingState;

        private CaptureMode captureMode_;
        public CaptureMode ApplicationCaptureMode
        {
            get
            {
                return captureMode_;
            }

            set
            {
                captureMode_ = value;

                switch (captureMode_)
                {
                    case CaptureMode.CaptureAll:
                        {
                            IsReadyToRecord = true;
                            break;
                        }

                    case CaptureMode.CaptureSingle:
                        {
                            IsReadyToRecord = false;
                            break;
                        }
                }

                this.NotifyPropertyChanged("ApplicationCaptureMode");
                this.NotifyPropertyChanged("IsInSingleApplicationMode");
            }
        }

        public
         bool IsReadyToRecord
        {
            get { return readyToRecord_; }
            set
            {
                readyToRecord_ = value;
                this.NotifyPropertyChanged("IsReadyToRecord");
            }
        }

        public
            bool IsInSingleApplicationMode
        {
            get
            {
                return captureMode_ == CaptureMode.CaptureSingle;
            }
        }

        private bool isCapturing_ = false;
        public bool IsCapturing
        {
            get
            {
                return isCapturing_;
            }

            set
            {
                isCapturing_ = value;
                this.NotifyPropertyChanged("IsCapturing");
            }
        }

        public
         String TargetExecutable
        {
            get { return targetExecutable; }
            set
            {
                targetExecutable = value;
                if (!String.IsNullOrEmpty(targetExecutable))
                {
                    IsReadyToRecord = true;
                }
                this.NotifyPropertyChanged("TargetExecutable");
            }
        }

        public
         String LoggingState
        {
            get { return loggingState; }
            set
            {
                loggingState = value;
                this.NotifyPropertyChanged("LoggingState");
            }
        }

        public
         event PropertyChangedEventHandler PropertyChanged;

        public
         void NotifyPropertyChanged(string propName)
        {
            if (this.PropertyChanged != null)
            {
                this.PropertyChanged(this, new PropertyChangedEventArgs(propName));
            }
        }
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        PresentMonWrapper presentMon;
        Configuration config = new Configuration { };
        KeyboardHook globalKeyboardHook = new KeyboardHook();
        bool showOverlay = true;
        KeyboardHook globalKeyboardHookToggleVisibility = new KeyboardHook();

        private RecordingOptions recordingOptions_ = new RecordingOptions();

        const string processName = ("PresentMon64.exe");

        const string startMsgAllApps = ("Capture All Applications");
        const string endMsgAllAps = ("Stop Capturing All Applications");
        const string defaultMsgApp = ("Capture Single Application");
        const string startMsgApp = ("Start Capturing Single Application");

        string loggingStateDefault = "Press F11 to start Benchmark Logging";

        int keyCode_ = 0x7A;
        int toggleVisibilityKeyCode_ = 0x7A;
        private
         bool keyCapturing_ = false;

        private
         SolidColorBrush deactivatedBrush = new SolidColorBrush(Color.FromArgb(255, 187, 187, 187));
        private
         SolidColorBrush activatedBrush = new SolidColorBrush(Color.FromArgb(255, 245, 189, 108));

        HashSet<int> overlayThreads_ = new HashSet<int>();
        HashSet<int> injectedProcesses_ = new HashSet<int>();

        [DllImport("user32.dll")]
        static extern bool PostThreadMessage(int idThread, uint Msg,
                                                                       IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll")]
        static extern int GetLastError();

        public
         MainWindow()
        {
            InitializeComponent();

            versionTextBlock.Text +=
                System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            this.DataContext = config;
            config.LoggingState = loggingStateDefault;
            presentMon = new PresentMonWrapper();

            globalKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(globalKeyDownEvent);
            globalKeyboardHookToggleVisibility.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(globalKeyDownEventToggleVisibility);
            LoadConfiguration();

            // Set the default capture mode
            SetCaptureMode(CaptureMode.CaptureAll);
        }

        /// Send the given message to the overlay and remove every invalid thread.
        void SendMessage(OverlayMessageType message)
        {
            List<int> invalidThreads = new List<int>();
            foreach (var threadID in overlayThreads_)
            {
                if (!PostThreadMessage(threadID, OverlayMessage.overlayMessage, (IntPtr)message, IntPtr.Zero))
                {
                    Debug.Print("PostThreadMessage failed " + GetLastError().ToString() + " removing thread " + threadID.ToString());
                    invalidThreads.Add(threadID);
                }
            }

            foreach (var threadID in invalidThreads)
            {
                overlayThreads_.Remove(threadID);
            }
        }

        void globalKeyDownEvent()
        {
            Debug.Print("globalKeyDownEvent");
            presentMon.KeyEvent();
            OverlayMessageType messageType = presentMon.CurrentlyRecording() ? OverlayMessageType.OVERLAY_StartRecording : OverlayMessageType.OVERLAY_StopRecording;
            SendMessage(messageType);
            UpdateCaptureStatus();
        }

        void globalKeyDownEventToggleVisibility()
        {
            Debug.Print("globalKeyDownEventToggleVisibility");
            showOverlay = !showOverlay;
            OverlayMessageType messageType = showOverlay ? OverlayMessageType.OVERLAY_ShowOverlay : OverlayMessageType.OVERLAY_HideOverlay;
            SendMessage(messageType);
        }

        protected
         override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            HwndSource source = PresentationSource.FromVisual(this) as HwndSource;
            source.AddHook(WndProc);
        }

        void UpdateCaptureStatus()
        {
            var recordingInProgress = presentMon.CurrentlyRecording();
            if (recordingInProgress)
            {
                var process = presentMon.GetRecordedProcess();
                config.LoggingState = "Benchmark Logging of \"" + process + "\" in progress";
            }
            else
            {
                config.LoggingState = loggingStateDefault;
            }

            if (presentMon.ProcessFinished())
            {
                startButton.Content = "Start";
                config.IsCapturing = false;
            }
        }

        private
      IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
          if (msg == OverlayMessage.overlayMessage)
          {
            OverlayMessageType messageType = (OverlayMessageType)wParam.ToInt32();
            Debug.Print(messageType.ToString() + " " + lParam.ToString());
            int lParamValue = lParam.ToInt32();
            switch (messageType)
            {
              case OverlayMessageType.OVERLAY_AttachDll:
                injectedProcesses_.Add(lParamValue);
                break;
              case OverlayMessageType.OVERLAY_DetachDll:
                injectedProcesses_.Remove(lParamValue);
                break;
              case OverlayMessageType.OVERLAY_ThreadInitialized:
                overlayThreads_.Add(lParamValue);
                break;
              case OverlayMessageType.OVERLAY_ThreadTerminating:
                overlayThreads_.Remove(lParamValue);
                break;
              //Remove overlay processes from the list of injected processes
              case OverlayMessageType.OVERLAY_Initialized:
                injectedProcesses_.Remove(lParamValue);
                break;
              default:
                break;
            }
          }
          else if (msg == OverlayMessage.recordingTimeMessage)
          {
            presentMon.KeyEvent();
            UpdateCaptureStatus();
          }

          UpdateCaptureStatus();

          return IntPtr.Zero;
        }

        int ConvertRecordTime()
        {
            int value = 0;
            if (!int.TryParse(timePeriod.Text, out value))
            {
                MessageBox.Show("Invalid time period: " + timePeriod.Text);
            }
            return value;
        }

        private
         void StoreConfiguration()
        {
            recordingOptions_.hotkey = keyCode_;
            recordingOptions_.recordTime = ConvertRecordTime();
            recordingOptions_.simple = (bool)simpleRecordingcheckBox.IsChecked;
            recordingOptions_.detailed = (bool)detailedRecordingcheckBox.IsChecked;
            recordingOptions_.recordAll = (bool)allProcessesRecordingcheckBox.IsChecked;

            ConfigurationFile.Save(recordingOptions_);
        }

        private
        void LoadConfiguration()
        {
            string path = ConfigurationFile.GetPath();

            recordingOptions_.Load(path);
            SetKey(KeyInterop.KeyFromVirtualKey(recordingOptions_.hotkey));
            toggleVisibilityKeyCode_ = recordingOptions_.toggleOverlayHotkey;
            timePeriod.Text = recordingOptions_.recordTime.ToString();
            simpleRecordingcheckBox.IsChecked = recordingOptions_.simple;
            detailedRecordingcheckBox.IsChecked = recordingOptions_.detailed;
            allProcessesRecordingcheckBox.IsChecked = recordingOptions_.recordAll;
        }

        private
         IntPtr GetHWND()
        {
            Window window = Window.GetWindow(this);
            var wih = new WindowInteropHelper(window);
            return wih.Handle;
        }

        private
         String GetWindowClassName()
        {
            Window window = Window.GetWindow(this);
            return window.GetType().Name;
        }

        private
         void targetExeButton_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog fileDialog = new OpenFileDialog();
            fileDialog.Filter = "Executable|*.exe";

            bool? result = fileDialog.ShowDialog();
            if (result.HasValue && (bool)result)
            {
                config.TargetExecutable = fileDialog.FileName;
                config.IsCapturing = false;
            }
        }

        private
         void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (keyCapturing_)
            {
                SetKey(e.Key);
                config.LoggingState = loggingStateDefault;
                hotkeyTextBlock.Text = "Recording Hotkey";
                keyCapturing_ = false;
            }
        }

        private void StopCapturing()
        {
          int[] threads = new int[overlayThreads_.Count()];
          int index = 0;
          foreach (var threadID in overlayThreads_)
          {
            threads[index++] = threadID;
          }

          presentMon.StopCapture(threads);
          overlayThreads_.Clear();
        }

        private void SetKey(Key key)
        {
          keyCode_ = KeyInterop.VirtualKeyFromKey(key);
          hotkeyString.Text = key.ToString();
          loggingStateDefault = "Press " + hotkeyString.Text + " to start Benchmark Logging";
        }

        private
         void Window_Closing(object sender, CancelEventArgs e)
        { StopCapturing(); }
        private
         void hotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            keyCapturing_ = true;
            hotkeyTextBlock.Text = "Press New Hotkey";
        }

        private
         void SetCaptureMode(CaptureMode captureMode)
        {
            config.ApplicationCaptureMode = captureMode;

            switch (captureMode)
            {
                case CaptureMode.CaptureSingle:
                    {
                        captureAllButton.Background = deactivatedBrush;
                        captureAppButton.Background = activatedBrush;
                        break;
                    }
                case CaptureMode.CaptureAll:
                    {
                        captureAllButton.Background = activatedBrush;
                        captureAppButton.Background = deactivatedBrush;
                        break;
                    }
            }

            StopCapturing();
            startButton.Content = "Start";
            config.IsCapturing = false;
        }

        private
         void captureAllButton_Click(object sender, RoutedEventArgs e)
        {
            SetCaptureMode(CaptureMode.CaptureAll);
        }

        private
         void captureAppButton_Click(object sender, RoutedEventArgs e)
        {
            SetCaptureMode(CaptureMode.CaptureSingle);
        }

        private
         void startButton_Click(object sender, RoutedEventArgs e)
        {
            if (!config.IsCapturing)
            {
                StoreConfiguration();
                presentMon.StartRecording(GetHWND());
                if (config.ApplicationCaptureMode == CaptureMode.CaptureSingle)
                {
                  presentMon.StartCaptureExe(targetExePath.Text, commandArgsExePath.Text);
                }
                else if (config.ApplicationCaptureMode == CaptureMode.CaptureAll)
                {
                  presentMon.StartCaptureAll();
                }

                startButton.Content = "Stop";
                globalKeyboardHook.ActivateHook(keyCode_);
                globalKeyboardHookToggleVisibility.ActivateHook(toggleVisibilityKeyCode_);
                config.IsCapturing = true;
            }
            else
            {
                StopCapturing();
                startButton.Content = "Start";
                globalKeyboardHook.UnHook();
                globalKeyboardHookToggleVisibility.UnHook();
                config.IsCapturing = false;
            }
        }

        private
         void Border_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        { DragMove(); }
        private
         void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            this.WindowState = System.Windows.WindowState.Minimized;
        }

        private
         void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            globalKeyboardHook.UnHook();

            int[] processes = new int[injectedProcesses_.Count()];
            int index = 0;
            int ocatProcessID = Process.GetCurrentProcess().Id;
            foreach (var processID in injectedProcesses_)
            {
              if (processID != ocatProcessID)
              {
                processes[index++] = processID;
              }
            }
            presentMon.FreeInjectedDlls(processes);
            this.Close();
        }
    }
}
