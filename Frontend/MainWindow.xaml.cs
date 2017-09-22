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
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using Wrapper;


namespace Frontend
{
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
        DelayTimer delayTimer;
        string recordingStateDefault = "Press F12 to start Benchmark Logging";
        int toggleRecordingKeyCode = 0x7A;
        bool enableRecordings = false;

        KeyboardHook toggleVisibilityKeyboardHook = new KeyboardHook();
        OverlayTracker overlayTracker;
        int toggleVisibilityKeyCode = 0x7A;

        public MainWindow()
        {
            InitializeComponent();

            versionTextBlock.Text +=
                System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            this.DataContext = userInterfaceState;
            userInterfaceState.RecordingState = recordingStateDefault;

            RegistryUpdater.UpdateInstallDir();
        }
        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);

            foreach (var item in RecordingDetailMethods.AsStringArray())
            {
                recordingDetail.Items.Add(item);
            }

            presentMon = new PresentMonWrapper();
            overlayTracker = new OverlayTracker();

            IntPtr hwnd = GetHWND();
            enableRecordings = presentMon.Init(hwnd);

            bool enableOverlay = overlayTracker.Init(hwnd);
            if (!enableOverlay)
            {
                SetInjectionMode(InjectionMode.Disabled);
                tabControl.Visibility = Visibility.Collapsed;
                overlayStateTextBox.Visibility = Visibility.Visible;
            }

            delayTimer = new DelayTimer(UpdateDelayTimer, StartRecordingDelayed);
            toggleRecordingKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(ToggleRecordingKeyDownEvent);
            toggleVisibilityKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(overlayTracker.ToggleOverlayVisibility);
            LoadConfiguration();

            // set the event listener after loading the configuration to avoid sending the first property change event.
            userInterfaceState.PropertyChanged += OnUserInterfacePropertyChanged;

            HwndSource source = PresentationSource.FromVisual(this) as HwndSource;
            source.AddHook(WndProc);

            if (recordingOptions.injectOnStart)
            {
                StartCapture(InjectionMode.All);
            }
        }

        public RecordingOptions GetRecordingOptions()
        {
            return recordingOptions;
        }
        

        void UpdateDelayTimer(int remainingTimeInMs)
        {
            float remainingDelayInSeconds = remainingTimeInMs / 1000.0f;
            remainingDelay.Text = remainingDelayInSeconds.ToString("0.00");
        }

        void StartRecordingDelayed()
        {
            TogglePresentMonRecording();
            overlayTracker.SendMessageToOverlay(OverlayMessageType.StartRecording);
            UpdateUserInterface();
        }

        void ToggleRecordingKeyDownEvent()
        {
            if(delayTimer.IsRunning())
            {
                delayTimer.Stop();
                UpdateUserInterface();
                return;
            }
            
            if(presentMon.CurrentlyRecording())
            {
                TogglePresentMonRecording();
                overlayTracker.SendMessageToOverlay(OverlayMessageType.StopRecording);
                UpdateUserInterface();
            }
            else
            {
                // kick off a new recording timer
                int delayInMs = ConvertTimeString(recordingDelay.Text) * 1000;
                delayTimer.Start(delayInMs, 100);
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
            
            if (overlayTracker.ProcessFinished())
            {
                userInterfaceState.IsCapturingSingle = false;
                startSingleApplicationButton.Content = "Start application";
            }

            if(!delayTimer.IsRunning())
            {
                remainingDelay.Text = "";
            }
        }

        private void TogglePresentMonRecording()
        {
            presentMon.ToggleRecording((bool)allProcessesRecordingcheckBox.IsChecked, 
                (uint)toggleRecordingKeyCode, (uint)ConvertTimeString(timePeriod.Text), GetRecordingDetail().ToInt());
        }

        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == OverlayMessage.overlayMessage)
            {
                OverlayMessageType messageType = (OverlayMessageType)wParam.ToInt32();
                overlayTracker.ReceivedOverlayMessage(messageType, lParam);
            }
            else if (msg == presentMon.GetPresentMonRecordingStopMessage())
            {
                TogglePresentMonRecording(); // Signal the recording to stop.
            }

            UpdateUserInterface();
            return IntPtr.Zero;
        }

        int ConvertTimeString(string timeString)
        {
            int value = 0;
            if (!int.TryParse(timeString, out value))
            {
                MessageBox.Show("Invalid time string: " + timeString);
            }
            return value;
        }

        private void StoreConfiguration()
        {
            recordingOptions.toggleRecordingHotkey = toggleRecordingKeyCode;
            recordingOptions.recordTime = ConvertTimeString(timePeriod.Text);
            recordingOptions.recordDelay = ConvertTimeString(recordingDelay.Text);
            recordingOptions.recordAll = (bool)allProcessesRecordingcheckBox.IsChecked;
            recordingOptions.toggleOverlayHotkey = toggleVisibilityKeyCode;
            recordingOptions.injectOnStart = (bool)injectionOnStartUp.IsChecked;
            recordingOptions.recordDetail = GetRecordingDetail().ToString();
            recordingOptions.overlayPosition = userInterfaceState.OverlayPositionProperty.ToInt();
            ConfigurationFile.Save(recordingOptions);
        }

        private void LoadConfiguration()
        {
            string path = ConfigurationFile.GetPath();
            recordingOptions.Load(path);
            SetToggleRecordingKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleRecordingHotkey));
            SetToggleVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleOverlayHotkey));
            timePeriod.Text = recordingOptions.recordTime.ToString();
            recordingDelay.Text = recordingOptions.recordDelay.ToString();
            allProcessesRecordingcheckBox.IsChecked = recordingOptions.recordAll;
            injectionOnStartUp.IsChecked = recordingOptions.injectOnStart;
            recordingDetail.Text = RecordingDetailMethods.GetFromString(recordingOptions.recordDetail).ToString();
            userInterfaceState.OverlayPositionProperty = OverlayPositionMethods.GetFromInt(recordingOptions.overlayPosition);
        }

        private void OnUserInterfacePropertyChanged(object sender, PropertyChangedEventArgs eventArgs)
        {
            switch(eventArgs.PropertyName)
            {
                case "OverlayPositionProperty":
                    StoreConfiguration();
                    overlayTracker.SendMessageToOverlay(userInterfaceState.OverlayPositionProperty.GetMessageType());
                    break;
            }
        }

        private RecordingDetail GetRecordingDetail()
        {
            return RecordingDetailMethods.GetFromString(recordingDetail.Text);
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
            overlayTracker.StopCapturing();
            userInterfaceState.IsCapturingSingle = false;
            userInterfaceState.IsCapturingGlobal = false;
            startOverlayGlobalButton.Content = "Start overlay";
            startSingleApplicationButton.Content = "Start application";
        }

        private void SetToggleVisibilityKey(Key key)
        {
            toggleVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleVisibilityTextBlock.Text = "Overlay visibility hotkey";
            toggleVisibilityHotkeyString.Text = key.ToString();
            toggleVisibilityKeyboardHook.ActivateHook(toggleVisibilityKeyCode);
        }

        private void SetToggleRecordingKey(Key key)
        {
            toggleRecordingKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleRecordingHotkeyString.Text = key.ToString();
            toggleRecordingTextBlock.Text = "Recording hotkey";

            if(enableRecordings)
            {
                recordingStateDefault = "Press " + toggleRecordingHotkeyString.Text + " to start Benchmark Logging";
                toggleRecordingKeyboardHook.ActivateHook(toggleRecordingKeyCode);
            }
            else
            {
                recordingStateDefault = "Recording is disabled due to errors during initialization.";
            }
        }


        private void Window_Closing(object sender, CancelEventArgs e)
        {
            StopCapturing();
            StoreConfiguration();
            overlayTracker.FreeInjectedDlls();
        }

        private void StartCapture(InjectionMode mode)
        {
            if(GetInjectionMode() == InjectionMode.Disabled)
            {
                return;
            }

            if (GetInjectionMode() != mode)
            {
                StopCapturing();
            }

            if (!userInterfaceState.IsCapturing())
            {
                StoreConfiguration();
                SetInjectionMode(mode);
                if (mode == InjectionMode.Single)
                {
                    startSingleApplicationButton.Content = "Stop overlay";
                    overlayTracker.StartCaptureExe(targetExePath.Text, commandArgsExePath.Text);
                    userInterfaceState.IsCapturingSingle = true;
                }
                else if (mode == InjectionMode.All)
                {
                    overlayTracker.StartCaptureAll();
                    userInterfaceState.IsCapturingGlobal = true;
                    startOverlayGlobalButton.Content = "Stop overlay";
                }
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

        private InjectionMode GetInjectionMode()
        {
            return userInterfaceState.ApplicationInjectionMode;
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
            toggleVisibilityKeyboardHook.UnHook();
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
