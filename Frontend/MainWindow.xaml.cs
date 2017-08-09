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

public enum InjectionMode
{
    All,
    Single
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
    public class UserInterfaceState : INotifyPropertyChanged
    {
        private bool readyToRecord = false;
        private string targetExecutable;
        private string recordingState;

        private InjectionMode injectionMode;
        public InjectionMode ApplicationInjectionMode
        {
            get
            {
                return injectionMode;
            }
            set
            {
                injectionMode = value;
                this.NotifyPropertyChanged("ApplicationInjectionMode");
            }
        }
        
        public bool IsReadyToRecord
        {
            get { return readyToRecord; }
            set
            {
                readyToRecord = value;
                this.NotifyPropertyChanged("IsReadyToRecord");
            }
        }


        public bool IsInSingleApplicationMode
        {
            get
            {
                return injectionMode == InjectionMode.Single;
            }
        }

        private bool isCapturingGlobal = false;
        public bool IsCapturingGlobal {
            get => isCapturingGlobal;
            set
            {
                isCapturingGlobal = value;
                this.NotifyPropertyChanged("IsCapturingGlobal");
            }
        }

        private bool isCapturingSingle = false;
        public bool IsCapturingSingle
        {
            get => isCapturingSingle;
            set
            {
                isCapturingSingle = value;
                this.NotifyPropertyChanged("IsCapturingSingle");
            }
        }

        public bool IsCapturing()
        {
            return isCapturingGlobal || isCapturingSingle;
        }
                
        public String TargetExecutable
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
        
        public String RecordingState
        {
            get { return recordingState; }
            set
            {
                recordingState = value;
                this.NotifyPropertyChanged("RecordingState");
            }
        }
        
        public event PropertyChangedEventHandler PropertyChanged;
        
        public void NotifyPropertyChanged(string propName)
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
        UserInterfaceState userInterfaceState = new UserInterfaceState { };
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

        HashSet<int> overlayThreads = new HashSet<int>();
        HashSet<int> injectedProcesses = new HashSet<int>();

        [DllImport("user32.dll")]
        static extern bool PostThreadMessage(int idThread, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll")]
        static extern int GetLastError();

        public MainWindow()
        {
            InitializeComponent();

            versionTextBlock.Text +=
                System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            this.DataContext = userInterfaceState;
            userInterfaceState.RecordingState = recordingStateDefault;
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

        public RecordingOptions GetRecordingOptions()
        {
            return recordingOptions;
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
            
            HwndSource source = PresentationSource.FromVisual(this) as HwndSource;
            source.AddHook(WndProc);

            SetInjectionMode(InjectionMode.All);
            if(recordingOptions.injectOnStart)
            {
                StartCapture(InjectionMode.All);
            }
        }

        void UpdateUserInterface()
        {
            var recordingInProgress = presentMon.CurrentlyRecording();
            if (recordingInProgress)
            {
                var process = presentMon.GetRecordedProcess();
                userInterfaceState.RecordingState = "Benchmark Logging of \"" + process + "\" in progress";
            }
            else
            {
                userInterfaceState.RecordingState = recordingStateDefault;
            }
            
            if (overlay.ProcessFinished())
            {
                toggleVisibilityKeyboardHook.UnHook();
                userInterfaceState.IsCapturingSingle = false;
                startSingleApplicationButton.Content = "Start Application";
            }
        }

        private void TogglePresentMonRecording()
        {
            presentMon.ToggleRecording((bool)allProcessesRecordingcheckBox.IsChecked, 
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
            recordingOptions.injectOnStart = (bool)injectionOnStartUp.IsChecked;
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
            injectionOnStartUp.IsChecked = recordingOptions.injectOnStart;
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
                userInterfaceState.TargetExecutable = fileDialog.FileName;
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
                userInterfaceState.RecordingState = recordingStateDefault;
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
            
            toggleVisibilityKeyboardHook.UnHook();
            userInterfaceState.IsCapturingSingle = false;
            userInterfaceState.IsCapturingGlobal = false;

            startSingleApplicationButton.Content = "Start Application";
        }

        private void SetToggleVisibilityKey(Key key)
        {
            toggleVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleVisibilityTextBlock.Text = "Overlay hotkey";
            toggleVisibilityHotkeyString.Text = key.ToString();
        }

        private void SetToggleRecordingKey(Key key)
        {
            toggleRecordingKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleRecordingHotkeyString.Text = key.ToString();
            toggleRecordingTextBlock.Text = "Recording hotkey";
            recordingStateDefault = "Press " + toggleRecordingHotkeyString.Text + " to start Benchmark Logging";
            toggleRecordingKeyboardHook.ActivateHook(toggleRecordingKeyCode);
        }


        private void Window_Closing(object sender, CancelEventArgs e)
        {
            StopCapturing();
            StoreConfiguration();
            
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
        }

        private void StartCapture(InjectionMode mode)
        {
            if (userInterfaceState.ApplicationInjectionMode != mode)
            {
                StopCapturing();
            }

            if (!userInterfaceState.IsCapturing())
            {
                StoreConfiguration();
                SetInjectionMode(mode);
                if (mode == InjectionMode.Single)
                {
                    startSingleApplicationButton.Content = "Disable Overlay";
                    overlay.StartCaptureExe(targetExePath.Text, commandArgsExePath.Text);
                    userInterfaceState.IsCapturingSingle = true;
                }
                else if (mode == InjectionMode.All)
                {
                    overlay.StartCaptureAll();
                    userInterfaceState.IsCapturingGlobal = true;
                }

                toggleVisibilityKeyboardHook.ActivateHook(toggleVisibilityKeyCode);
            }
            else
            {
                StopCapturing();
            }
        }

        private void SetInjectionMode(InjectionMode captureMode)
        {
            userInterfaceState.ApplicationInjectionMode = captureMode;
        }

        private void StartOverlayGlobalButton_Click(object sender, RoutedEventArgs e)
        {
            StartCapture(InjectionMode.All);
        }
        
        private void StartSingleApplicationButton_Click(object sender, RoutedEventArgs e)
        {
            StartCapture(InjectionMode.Single);
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
            textBlock.Text = "Press new hotkey";
            keyCaptureMode = mode;
        }
    }
}
