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

/// Used to identify the current key to assign
public enum KeyCaptureMode
{
    None,
    RecordingToggle,
    VisibilityToggle
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
        Configuration config = new Configuration { };
        KeyCaptureMode keyCaptureMode;

        PresentMonWrapper presentMon;
        KeyboardHook toggleRecordingKeyboardHook = new KeyboardHook();
        RecordingOptions recordingOptions = new RecordingOptions();
        string recordingStateDefault = "Press F12 to start Benchmark Logging";
        int toggleRecordingKeyCode = 0x7A;

        OverlayWrapper overlay;
        KeyboardHook toggleVisibilityKeyboardHook = new KeyboardHook();
        bool showOverlay = true;
        int toggleVisibilityKeyCode = 0x7A;

        SolidColorBrush deactivatedBrush = new SolidColorBrush(Color.FromArgb(255, 187, 187, 187));
        SolidColorBrush activatedBrush = new SolidColorBrush(Color.FromArgb(255, 245, 189, 108));

        HashSet<int> overlayThreads = new HashSet<int>();
        HashSet<int> injectedProcesses = new HashSet<int>();

        [DllImport("user32.dll")]
        static extern bool PostThreadMessage(int idThread, uint Msg,
                                                                       IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll")]
        static extern int GetLastError();

        public MainWindow()
        {
            InitializeComponent();

            versionTextBlock.Text +=
                System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            this.DataContext = config;
            config.LoggingState = recordingStateDefault;
        }

        /// Send the given message to the overlay and remove every invalid thread.
        void SendMessageToOverlay(OverlayMessageType message)
        {
            List<int> invalidThreads = new List<int>();
            foreach (var threadID in overlayThreads)
            {
                if (!PostThreadMessage(threadID, OverlayMessage.overlayMessage, (IntPtr)message, IntPtr.Zero))
                {
                    Debug.Print("Send message to overlay failed " + GetLastError().ToString() + " removing thread " + threadID.ToString());
                    invalidThreads.Add(threadID);
                }
            }

            foreach (var threadID in invalidThreads)
            {
                overlayThreads.Remove(threadID);
            }
        }

        void ToggleRecordingKeyDownEvent()
        {
            TogglePresentMonRecording();
            OverlayMessageType messageType = presentMon.CurrentlyRecording() ? OverlayMessageType.OVERLAY_StartRecording : OverlayMessageType.OVERLAY_StopRecording;
            SendMessageToOverlay(messageType);
            UpdateUserInterface();
        }
        void ToggleVisibilityKeyDownEvent()
        {
            showOverlay = !showOverlay;
            OverlayMessageType messageType = showOverlay ? OverlayMessageType.OVERLAY_ShowOverlay : OverlayMessageType.OVERLAY_HideOverlay;
            SendMessageToOverlay(messageType);
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);

            IntPtr hwnd = GetHWND();
            presentMon = new PresentMonWrapper(hwnd);
            overlay = new OverlayWrapper(hwnd);
            
            toggleRecordingKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(ToggleRecordingKeyDownEvent);
            toggleVisibilityKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(ToggleVisibilityKeyDownEvent);
            LoadConfiguration();

            // Set the default capture mode
            SetCaptureMode(CaptureMode.CaptureAll);

            HwndSource source = PresentationSource.FromVisual(this) as HwndSource;
            source.AddHook(WndProc);
        }

        void UpdateUserInterface()
        {
            var recordingInProgress = presentMon.CurrentlyRecording();
            if (recordingInProgress)
            {
                var process = presentMon.GetRecordedProcess();
                config.LoggingState = "Benchmark Logging of \"" + process + "\" in progress";
            }
            else
            {
                config.LoggingState = recordingStateDefault;
            }

            if (overlay.ProcessFinished())
            {
                startButton.Content = "Start";
                config.IsCapturing = false;
            }
        }

        private void TogglePresentMonRecording()
        {
            presentMon.KeyEvent((bool)allProcessesRecordingcheckBox.IsChecked, 
                (uint)toggleRecordingKeyCode, (uint)recordingOptions.recordTime);
        }

        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == OverlayMessage.overlayMessage)
            {
                OverlayMessageType messageType = (OverlayMessageType)wParam.ToInt32();
                Debug.Print(messageType.ToString() + " " + lParam.ToString());
                int lParamValue = lParam.ToInt32();
                switch (messageType)
                {
                    case OverlayMessageType.OVERLAY_AttachDll:
                        injectedProcesses.Add(lParamValue);
                        break;
                    case OverlayMessageType.OVERLAY_DetachDll:
                        injectedProcesses.Remove(lParamValue);
                        break;
                    case OverlayMessageType.OVERLAY_ThreadInitialized:
                        overlayThreads.Add(lParamValue);
                        break;
                    case OverlayMessageType.OVERLAY_ThreadTerminating:
                        overlayThreads.Remove(lParamValue);
                        break;
                    //Remove overlay process from the list of injected processes
                    case OverlayMessageType.OVERLAY_Initialized:
                        injectedProcesses.Remove(lParamValue);
                        break;
                    default:
                        break;
                }
            }
            else if (msg == presentMon.GetPresentMonRecordingStopMessage())
            {
                TogglePresentMonRecording(); // Signal the recording to stop.
            }

            UpdateUserInterface();
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

        private void StoreConfiguration()
        {
            recordingOptions.toggleRecordingHotkey = toggleRecordingKeyCode;
            recordingOptions.recordTime = ConvertRecordTime();
            recordingOptions.recordAll = (bool)allProcessesRecordingcheckBox.IsChecked;
            recordingOptions.toggleOverlayHotkey = toggleVisibilityKeyCode;
            ConfigurationFile.Save(recordingOptions);
        }

        private void LoadConfiguration()
        {
            string path = ConfigurationFile.GetPath();
            recordingOptions.Load(path);
            SetToggleRecordingKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleRecordingHotkey));
            SetToggleVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleOverlayHotkey));
            timePeriod.Text = recordingOptions.recordTime.ToString();
            allProcessesRecordingcheckBox.IsChecked = recordingOptions.recordAll;
        }

        private IntPtr GetHWND()
        {
            Window window = Window.GetWindow(this);
            return new WindowInteropHelper(window).EnsureHandle();
        }


        private String GetWindowClassName()
        {
            Window window = Window.GetWindow(this);
            return window.GetType().Name;
        }

        private void TargetExeButton_Click(object sender, RoutedEventArgs e)
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

        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (keyCaptureMode != KeyCaptureMode.None)
            {
                switch (keyCaptureMode)
                {
                    case KeyCaptureMode.RecordingToggle:
                        SetToggleRecordingKey(e.Key);
                        break;
                    case KeyCaptureMode.VisibilityToggle:
                        SetToggleVisibilityKey(e.Key);
                        break;
                }
                config.LoggingState = recordingStateDefault;
                keyCaptureMode = KeyCaptureMode.None;
                StoreConfiguration();
            }
        }

        private void StopCapturing()
        {
            int[] threads = new int[overlayThreads.Count()];
            int index = 0;
            foreach (var threadID in overlayThreads)
            {
                threads[index++] = threadID;
            }

            overlay.StopCapture(threads);
            overlayThreads.Clear();
        }

        private void SetToggleVisibilityKey(Key key)
        {
            toggleVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleVisibilityTextBlock.Text = "Toggle Visibility Hotkey";
            toggleVisibilityHotkeyString.Text = key.ToString();
        }

        private void SetToggleRecordingKey(Key key)
        {
            toggleRecordingKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleRecordingHotkeyString.Text = key.ToString();
            toggleRecordingTextBlock.Text = "Recording Hotkey";
            recordingStateDefault = "Press " + toggleRecordingHotkeyString.Text + " to start Benchmark Logging";
            toggleRecordingKeyboardHook.ActivateHook(toggleRecordingKeyCode);
        }


        private void Window_Closing(object sender, CancelEventArgs e)
        {
            StopCapturing();
            toggleRecordingKeyboardHook.UnHook();
        }

        private void SetCaptureMode(CaptureMode captureMode)
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


        private void CaptureAllButton_Click(object sender, RoutedEventArgs e)
        {
            SetCaptureMode(CaptureMode.CaptureAll);
        }
        
        private void CaptureAppButton_Click(object sender, RoutedEventArgs e)
        {
            SetCaptureMode(CaptureMode.CaptureSingle);
        }
        
        private void StartButton_Click(object sender, RoutedEventArgs e)
        {
            if (!config.IsCapturing)
            {
                StoreConfiguration();
                if (config.ApplicationCaptureMode == CaptureMode.CaptureSingle)
                {
                    overlay.StartCaptureExe(targetExePath.Text, commandArgsExePath.Text);
                }
                else if (config.ApplicationCaptureMode == CaptureMode.CaptureAll)
                {
                    overlay.StartCaptureAll();
                }

                startButton.Content = "Stop";
                toggleVisibilityKeyboardHook.ActivateHook(toggleVisibilityKeyCode);
                config.IsCapturing = true;
            }
            else
            {
                StopCapturing();
                startButton.Content = "Start";
                toggleVisibilityKeyboardHook.UnHook();
                config.IsCapturing = false;
            }
        }
        
        private void Border_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            DragMove();
        }
        
        private void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            this.WindowState = System.Windows.WindowState.Minimized;
        }
        
        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            int[] processes = new int[injectedProcesses.Count()];
            int index = 0;
            int ocatProcessID = Process.GetCurrentProcess().Id;
            foreach (var processID in injectedProcesses)
            {
                if (processID != ocatProcessID)
                {
                    processes[index++] = processID;
                }
            }
            overlay.FreeInjectedDlls(processes);
            this.Close();
        }

        private void ToggleVisibilityHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            CaptureKey(KeyCaptureMode.VisibilityToggle, toggleVisibilityTextBlock);
        }
        
        private void ToggleRecordingHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            toggleRecordingKeyboardHook.UnHook();
            CaptureKey(KeyCaptureMode.RecordingToggle, toggleRecordingTextBlock);
        }

        private void CaptureKey(KeyCaptureMode mode, System.Windows.Controls.TextBlock textBlock)
        {
            textBlock.Text = "Press New Hotkey";
            keyCaptureMode = mode;
        }
    }
}
