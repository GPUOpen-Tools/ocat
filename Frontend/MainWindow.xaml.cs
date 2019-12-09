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
using Microsoft.WindowsAPICodePack.Dialogs;
using Microsoft.WindowsAPICodePack.ApplicationServices;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
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
        string recordingStateDefault = "Press F10 to start Capture";
        int toggleRecordingKeyCode = 0x79;
        bool enableRecordings = false;

        KeyboardHook toggleVisibilityKeyboardHook = new KeyboardHook();
        KeyboardHook toggleGraphVisibilityKeyboardHook = new KeyboardHook();
        KeyboardHook toggleBarVisibilityKeyboardHook = new KeyboardHook();
        KeyboardHook toggleLagIndicatorVisibilityKeyboardHook = new KeyboardHook();
        OverlayTracker overlayTracker;
        int toggleVisibilityKeyCode = 0x78;
        int toggleGraphVisibilityKeyCode = 0x76;
        int toggleBarVisibilityKeyCode = 0x77;
        int toggleLagIndicatorVisibilityKeyCode = 0x75;
        int lagIndicatorKeyCode = 0x91;

        public MainWindow()
        {
            System.Globalization.CultureInfo.DefaultThreadCurrentCulture = System.Globalization.CultureInfo.CreateSpecificCulture("en-US");

            InitializeComponent();

            RegisterApplicationRecoveryAndRestart();

            Version ver = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
            versionTextBlock.Text += ver.Major.ToString() + "." + ver.Minor.ToString() + "." + ver.Build.ToString();

            this.DataContext = userInterfaceState;
            userInterfaceState.RecordingState = recordingStateDefault;

            RegistryUpdater.UpdateInstallDir();
        }

        private int PerformRecovery(object parameter)
        {
            try
            {
                ApplicationRestartRecoveryManager.ApplicationRecoveryInProgress();

                RegistryUpdater.DisableImplicitLayer();

                ApplicationRestartRecoveryManager.ApplicationRecoveryFinished(true);
            }
            catch
            {
                ApplicationRestartRecoveryManager.ApplicationRecoveryFinished(false);
            }

            return 0;
        }

        private void RegisterApplicationRecoveryAndRestart()
        {
            RecoverySettings recoverySettings = new RecoverySettings(new RecoveryData(PerformRecovery, null), 5000);
            ApplicationRestartRecoveryManager.RegisterForApplicationRecovery(recoverySettings);
        }

        private void UnregisterApplicationRecoveryAndRestart()
        {
            ApplicationRestartRecoveryManager.UnregisterApplicationRecovery();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);

            presentMon = new PresentMonWrapper();
            overlayTracker = new OverlayTracker();

            IntPtr hwnd = GetHWND();
            enableRecordings = presentMon.Init(hwnd, System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString());

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
            toggleGraphVisibilityKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(overlayTracker.ToggleGraphOverlayVisibility);
            toggleBarVisibilityKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(overlayTracker.ToggleBarOverlayVisibility);
            toggleLagIndicatorVisibilityKeyboardHook.HotkeyDownEvent += new KeyboardHook.KeyboardDownEvent(overlayTracker.ToggleLagIndicatorOverlayVisibility);
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
            if (delayTimer.IsRunning())
            {
                delayTimer.Stop();
                UpdateUserInterface();
                return;
            }

            if (presentMon.CurrentlyRecording())
            {
                TogglePresentMonRecording();
                overlayTracker.SendMessageToOverlay(OverlayMessageType.StopRecording);
                UpdateUserInterface();
            }
            else
            {
                // kick off a new recording timer
                int delayInMs = ConvertTimeString(captureDelay.Text) * 1000;
                delayTimer.Start(delayInMs, 100);
            }
        }

        void UpdateUserInterface()
        {
            var recordingInProgress = presentMon.CurrentlyRecording();
            if (recordingInProgress)
            {
                var process = presentMon.GetRecordedProcess();
                userInterfaceState.RecordingState = "Capture of \"" + process + "\" in progress";
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

            if (!delayTimer.IsRunning())
            {
                remainingDelay.Text = "";
            }
        }

        private void TogglePresentMonRecording()
        {
            // Send user note to present mon interface
            if (!String.IsNullOrEmpty(userInterfaceState.CaptureUserNote))
            {
                presentMon.UpdateUserNote(userInterfaceState.CaptureUserNote);
            }

            if (!String.IsNullOrEmpty(userInterfaceState.CaptureOutputFolder))
            {
                presentMon.UpdateOutputFolder(userInterfaceState.CaptureOutputFolder);
            }

            presentMon.ToggleRecording((bool)allProcessesRecordingcheckBox.IsChecked,
                (uint)ConvertTimeString(userInterfaceState.TimePeriod), (bool)audioCueCheckBox.IsChecked);
        }

        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == OverlayMessage.WM_HOTKEY)
            {
                // we need the high-order word to get the virtual key code.
                // low-order word specifies the keys that were to be pressed in combination with the virtual key code - in our case none
                toggleRecordingKeyboardHook.OnHotKeyEvent(lParam.ToInt32() >> 16);
                toggleVisibilityKeyboardHook.OnHotKeyEvent(lParam.ToInt32() >> 16);
                toggleGraphVisibilityKeyboardHook.OnHotKeyEvent(lParam.ToInt32() >> 16);
                toggleBarVisibilityKeyboardHook.OnHotKeyEvent(lParam.ToInt32() >> 16);
                toggleLagIndicatorVisibilityKeyboardHook.OnHotKeyEvent(lParam.ToInt32() >> 16);
            }
            else if (msg == OverlayMessage.overlayMessage)
            {
                OverlayMessageType messageType = (OverlayMessageType)wParam.ToInt32();
                overlayTracker.ReceivedOverlayMessage(messageType, lParam);
            }
            else if (msg == presentMon.GetPresentMonRecordingStopMessage())
            {
                ToggleRecordingKeyDownEvent(); // Signal the recording to stop.
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

        private void StoreDisableOverlayWhileRecording()
        {
            recordingOptions.disableOverlayWhileRecording = !(bool)disableOverlayWhileRecording.IsChecked;
            ConfigurationFile.Save(recordingOptions);
        }

        private void StoreConfiguration()
        {
            recordingOptions.toggleCaptureHotkey = toggleRecordingKeyCode;
            recordingOptions.captureTime = ConvertTimeString(userInterfaceState.TimePeriod);
            recordingOptions.captureDelay = ConvertTimeString(captureDelay.Text);
            recordingOptions.captureAll = (bool)allProcessesRecordingcheckBox.IsChecked;
            recordingOptions.audioCue = (bool)audioCueCheckBox.IsChecked;
            recordingOptions.toggleOverlayHotkey = toggleVisibilityKeyCode;
            recordingOptions.toggleGraphOverlayHotkey = toggleGraphVisibilityKeyCode;
            recordingOptions.toggleBarOverlayHotkey = toggleBarVisibilityKeyCode;
            recordingOptions.toggleLagIndicatorOverlayHotkey = toggleLagIndicatorVisibilityKeyCode;
            recordingOptions.lagIndicatorHotkey = lagIndicatorKeyCode;
            recordingOptions.injectOnStart = (bool)injectionOnStartUp.IsChecked;
            recordingOptions.altKeyComb = (bool)altCheckBox.IsChecked;
            recordingOptions.disableOverlayWhileRecording = (bool)disableOverlayWhileRecording.IsChecked;
            recordingOptions.overlayPosition = userInterfaceState.OverlayPositionProperty.ToInt();
            recordingOptions.captureOutputFolder = userInterfaceState.CaptureOutputFolder;
            ConfigurationFile.Save(recordingOptions);
        }

        private void LoadConfiguration()
        {
            string path = ConfigurationFile.GetPath();
            recordingOptions.Load(path);
            altCheckBox.IsChecked = recordingOptions.altKeyComb;
            disableOverlayWhileRecording.IsChecked = recordingOptions.disableOverlayWhileRecording;
            SetToggleRecordingKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleCaptureHotkey));
            SetToggleVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleOverlayHotkey));
            SetToggleGraphVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleGraphOverlayHotkey));
            SetToggleBarVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleBarOverlayHotkey));
            SetToggleLagIndicatorVisibilityKey(KeyInterop.KeyFromVirtualKey(recordingOptions.toggleLagIndicatorOverlayHotkey));
            SetLagIndicatorHotKey(recordingOptions.lagIndicatorHotkey);
            userInterfaceState.TimePeriod = recordingOptions.captureTime.ToString();
            captureDelay.Text = recordingOptions.captureDelay.ToString();
            allProcessesRecordingcheckBox.IsChecked = recordingOptions.captureAll;
            audioCueCheckBox.IsChecked = recordingOptions.audioCue;
            injectionOnStartUp.IsChecked = recordingOptions.injectOnStart;
            userInterfaceState.OverlayPositionProperty = OverlayPositionMethods.GetFromInt(recordingOptions.overlayPosition);
            userInterfaceState.CaptureOutputFolder = recordingOptions.captureOutputFolder;
        }

        private void OnUserInterfacePropertyChanged(object sender, PropertyChangedEventArgs eventArgs)
        {
            switch (eventArgs.PropertyName)
            {
                case "OverlayPositionProperty":
                    StoreConfiguration();
                    overlayTracker.SendMessageToOverlay(userInterfaceState.OverlayPositionProperty.GetMessageType());
                    break;
                case "TimePeriod":
                    StoreConfiguration();
                    overlayTracker.SendMessageToOverlay(OverlayMessageType.CaptureTime);
                    break;
                case "LagIndicatorHotkey":
                    StoreConfiguration();
                    overlayTracker.SendMessageToOverlay(OverlayMessageType.LagIndicator);
                    break;
                case "AltCheckBoxIsChecked":
                    // need to invert check box, because change happens after this call
                    toggleVisibilityKeyboardHook.ModifyKeyCombination(!(bool)altCheckBox.IsChecked, GetHWND());
                    toggleBarVisibilityKeyboardHook.ModifyKeyCombination(!(bool)altCheckBox.IsChecked, GetHWND());
                    toggleGraphVisibilityKeyboardHook.ModifyKeyCombination(!(bool)altCheckBox.IsChecked, GetHWND());
                    toggleLagIndicatorVisibilityKeyboardHook.ModifyKeyCombination(!(bool)altCheckBox.IsChecked, GetHWND());
                    if (enableRecordings)
                    {
                        if (toggleRecordingKeyboardHook.ModifyKeyCombination(!(bool)altCheckBox.IsChecked, GetHWND()))
                        {
                            toggleRecordingHotkeyString.Text = KeyInterop.KeyFromVirtualKey(toggleRecordingKeyCode).ToString();
                            if (!(bool)altCheckBox.IsChecked)
                            {
                                recordingStateDefault = "Press ALT + " + toggleRecordingHotkeyString.Text + " to start Capture";
                            }
                            else
                            {
                                recordingStateDefault = "Press " + toggleRecordingHotkeyString.Text + " to start Capture";
                            }
                        }
                        else
                        {
                            toggleRecordingHotkeyString.Text = "";
                            recordingStateDefault = "You must define a valid Capture hotkey.";
                        }
                    }
                    else
                    {
                        recordingStateDefault = "Capture is disabled due to errors during initialization.";
                    }
                    break;
                case "DisableOverlayWhileRecording":
                    StoreDisableOverlayWhileRecording();
                    if (!(bool)disableOverlayWhileRecording.IsChecked)
                    {
                        overlayTracker.SendMessageToOverlay(OverlayMessageType.HideOverlayWhileRecording);
                    } else
                    {
                        overlayTracker.SendMessageToOverlay(OverlayMessageType.ShowOverlayWhileRecording);
                    }
                    break;
            }
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

        private void RecordingOutputFolderButton_Click(object sender, RoutedEventArgs e)
        {
            var folderDialog = new CommonOpenFileDialog();
            folderDialog.IsFolderPicker = true;

            if (folderDialog.ShowDialog() == CommonFileDialogResult.Ok)
            {
                userInterfaceState.CaptureOutputFolder = folderDialog.FileName;
                if (!String.IsNullOrEmpty(folderDialog.FileName))
                {
                    presentMon.UpdateOutputFolder(folderDialog.FileName);
                }
            }
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

        private void TargetCsvButton_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog fileDialog = new OpenFileDialog();
            fileDialog.Filter = "CSV|*.csv";

            const string captureFolderName = ("\\OCAT\\Captures\\");
            string initialDirectory = System.Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments) + captureFolderName;

            if (!String.IsNullOrEmpty(userInterfaceState.CaptureOutputFolder))
            {
                initialDirectory = userInterfaceState.CaptureOutputFolder;
            }
            fileDialog.InitialDirectory = initialDirectory;

            bool? result = fileDialog.ShowDialog();
            if (result.HasValue && (bool)result)
            {
                userInterfaceState.CsvFile = fileDialog.FileName;
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
                    case KeyCaptureMode.GraphVisibilityToggle:
                        SetToggleGraphVisibilityKey(e.Key);
                        break;
                    case KeyCaptureMode.BarVisibilityToggle:
                        SetToggleBarVisibilityKey(e.Key);
                        break;
                    case KeyCaptureMode.LagIndicatorVisibilityToggle:
                        SetToggleLagIndicatorVisibilityKey(e.Key);
                        break;
                    case KeyCaptureMode.LagIndicatorHotkey:
                        SetLagIndicatorHotKey(KeyInterop.VirtualKeyFromKey(e.Key));
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

            RegistryUpdater.DisableImplicitLayer();
        }

        private void SetToggleVisibilityKey(Key key)
        {
            toggleVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleVisibilityTextBlock.Text = "Overlay visibility hotkey";

            if (toggleVisibilityKeyboardHook.ActivateHook(toggleVisibilityKeyCode, GetHWND(), (bool)altCheckBox.IsChecked))
            {
                toggleVisibilityHotkeyString.Text = key.ToString();
            }
            else
            {
                toggleVisibilityHotkeyString.Text = "";
            }
        }

        private void SetToggleGraphVisibilityKey(Key key)
        {
            toggleGraphVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleGraphVisibilityTextBlock.Text = "Frame graph visibility hotkey";

            if (toggleGraphVisibilityKeyboardHook.ActivateHook(toggleGraphVisibilityKeyCode, GetHWND(), (bool)altCheckBox.IsChecked))
            {
                toggleGraphVisibilityHotkeyString.Text = key.ToString();
            }
            else
            {
                toggleGraphVisibilityHotkeyString.Text = "";
            }
        }

        private void SetToggleBarVisibilityKey(Key key)
        {
            toggleBarVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleBarVisibilityTextBlock.Text = "Colored bar visibility hotkey";

            if (toggleBarVisibilityKeyboardHook.ActivateHook(toggleBarVisibilityKeyCode, GetHWND(), (bool)altCheckBox.IsChecked))
            {
                toggleBarVisibilityHotkeyString.Text = key.ToString();
            }
            else
            {
                toggleBarVisibilityHotkeyString.Text = "";
            }
        }

        private void SetToggleLagIndicatorVisibilityKey(Key key)
        {
            toggleLagIndicatorVisibilityKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleLagIndicatorVisibilityTextBlock.Text = "Lag indicator visibility hotkey";

            if (toggleLagIndicatorVisibilityKeyboardHook.ActivateHook(toggleLagIndicatorVisibilityKeyCode, GetHWND(), (bool)altCheckBox.IsChecked))
            {
                toggleLagIndicatorVisibilityHotkeyString.Text = key.ToString();
            }
            else
            {
                toggleLagIndicatorVisibilityHotkeyString.Text = "";
            }
        }

        private void SetLagIndicatorHotKey(int key)
        {
            lagIndicatorKeyCode = key;
            lagIndicatorTextBlock.Text = "Lag indicator hotkey";

            lagIndicatorHotkeyString.Text = KeyInterop.KeyFromVirtualKey(key).ToString();
            userInterfaceState.LagIndicatorHotkey = key;
        }

        private void SetToggleRecordingKey(Key key)
        {
            toggleRecordingKeyCode = KeyInterop.VirtualKeyFromKey(key);
            toggleRecordingTextBlock.Text = "Capture hotkey";

            if (enableRecordings)
            {
                if (toggleRecordingKeyboardHook.ActivateHook(toggleRecordingKeyCode, GetHWND(), (bool)altCheckBox.IsChecked))
                {
                    toggleRecordingHotkeyString.Text = key.ToString();
                    if ((bool)altCheckBox.IsChecked)
                    {
                        recordingStateDefault = "Press ALT + " + toggleRecordingHotkeyString.Text + " to start Capture";
                    }
                    else
                    {
                        recordingStateDefault = "Press " + toggleRecordingHotkeyString.Text + " to start Capture";
                    }
                }
                else
                {
                    toggleRecordingHotkeyString.Text = "";
                    recordingStateDefault = "You must define a valid Capture hotkey.";
                }

            }
            else
            {
                recordingStateDefault = "Capture is disabled due to errors during initialization.";
            }
        }


        private void Window_Closing(object sender, CancelEventArgs e)
        {
            //UnregisterHotKey();
            StopCapturing();
            StoreConfiguration();
            overlayTracker.FreeInjectedDlls();
            UnregisterApplicationRecoveryAndRestart();
        }

        private void StartCapture(InjectionMode mode)
        {
            if (GetInjectionMode() == InjectionMode.Disabled)
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

                    RegistryUpdater.EnableImplicitlayer();
                }
            }
            else
            {
                if (mode == InjectionMode.All)
                {
                    // Check if all processes with GameOverlay.dll are terminated, otherwise
                    // warn the user to finish all processes.
                    if (overlayTracker.GetHookedProcesses().Count() > 0)
                    {
                        var processIds = overlayTracker.GetHookedProcesses();
                        string processes = "";

                        foreach (int id in processIds)
                        {
                            try
                            {
                                Process process = Process.GetProcessById(id);
                                if (process.ProcessName != "ApplicationFrameHost")
                                {
                                    processes += "\n" + process.ProcessName;
                                }
                            }
                            catch
                            {
                                // skip - process does not exist anymore
                            }
                        }

                        if (processes.Length > 0)
                        {
                            var result = MessageBox.Show(
                                "Continue stop capturing? Following processes are still running: " + processes, "Running processes", MessageBoxButton.YesNo);

                            if (result == MessageBoxResult.No)
                            {
                                return;
                            }
                        }
                    }
                }

                StopCapturing();
            }
        }

        private void StartVisualization()
        {
            // open up new window and display simple framegraph
            VisualizationWindow visWin = new VisualizationWindow(userInterfaceState.CsvFile);
            visWin.Show();
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

        private void StartVisCsvButton_Click(object sender, RoutedEventArgs e)
        {
            StartVisualization();
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

        private void ToggleGraphVisibilityHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            toggleGraphVisibilityKeyboardHook.UnHook();
            CaptureKey(KeyCaptureMode.GraphVisibilityToggle, toggleGraphVisibilityTextBlock);
        }

        private void ToggleBarVisibilityHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            toggleBarVisibilityKeyboardHook.UnHook();
            CaptureKey(KeyCaptureMode.BarVisibilityToggle, toggleBarVisibilityTextBlock);
        }

        private void ToggleLagIndicatorVisibilityHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            toggleLagIndicatorVisibilityKeyboardHook.UnHook();
            CaptureKey(KeyCaptureMode.LagIndicatorVisibilityToggle, toggleLagIndicatorVisibilityTextBlock);
        }

        private void LagIndicatorHotkeyButton_Click(object sender, RoutedEventArgs e)
        {
            CaptureKey(KeyCaptureMode.LagIndicatorHotkey, lagIndicatorTextBlock);
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
