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
// The above copyright notice and this permission notice shall be included in
// all
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
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Wrapper;

namespace Frontend
{
    /// <summary>
    ///  Tracks all injected overlays and their respective threads.
    /// </summary>
    class OverlayTracker
    {

        OverlayWrapper overlay;
        bool showOverlay = true;
        bool showGraphOverlay = true;
        bool showBarOverlay = false;
        bool showLagIndicatorOverlay = false;

        HashSet<int> overlayThreads = new HashSet<int>();
        HashSet<int> injectedProcesses = new HashSet<int>();

        string steamAppIdFile;

        List<int> hookedProcesses = new List<int>();

        [DllImport("user32.dll")]
        static extern bool PostThreadMessage(int idThread, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll")]
        static extern int GetLastError();

        public OverlayTracker()
        {
            overlay = new OverlayWrapper();
        }

        public bool Init(IntPtr hwnd)
        {
            return overlay.Init(hwnd);
        }

        /// <summary>
        ///  Send the given message to the overlay and remove every invalid thread.
        /// </summary>
        public void SendMessageToOverlay(OverlayMessageType message)
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

        public void ReceivedOverlayMessage(OverlayMessageType messageType, IntPtr lParam)
        {
            Debug.Print(messageType.ToString() + " " + lParam.ToString());
            int lParamValue = lParam.ToInt32();
            switch (messageType)
            {
                case OverlayMessageType.AttachDll:
                    injectedProcesses.Add(lParamValue);
                    hookedProcesses.Add(lParamValue);
                    break;
                case OverlayMessageType.DetachDll:
                    injectedProcesses.Remove(lParamValue);
                    hookedProcesses.Remove(lParamValue);
                    // reset overlay state
                    showOverlay = true;
                    showGraphOverlay = true;
                    showBarOverlay = false;
                    showLagIndicatorOverlay = false;
                    break;
                case OverlayMessageType.ThreadInitialized:
                    overlayThreads.Add(lParamValue);
                    break;
                case OverlayMessageType.ThreadTerminating:
                    overlayThreads.Remove(lParamValue);
                    break;
                //Remove overlay process from the list of injected processes
                case OverlayMessageType.Initialized:
                    injectedProcesses.Remove(lParamValue);
                    break;
                default:
                    break;
            }
        }

        /// <summary>
        /// Returns a copy of all overlay thread ids as array.
        /// </summary>
        private int[] GetOverlayThreadsAsArray()
        {
            int[] threads = new int[overlayThreads.Count()];
            int index = 0;
            foreach (var threadID in overlayThreads)
            {
                threads[index++] = threadID;
            }
            return threads;
        }

        /// <summary>
        /// Returns a copy of all injected processes ids as array.
        /// It excludes the id of the current process.
        /// </summary>
        private int[] GetInjectedProcessesAsArray()
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
            return processes;
        }

        public List<int> GetHookedProcesses()
        {
            return hookedProcesses;
        }

        public void ToggleOverlayVisibility()
        {
            showOverlay = !showOverlay;
            OverlayMessageType messageType = showOverlay ? OverlayMessageType.ShowOverlay : OverlayMessageType.HideOverlay;
            SendMessageToOverlay(messageType);
        }

        public void ToggleGraphOverlayVisibility()
        {
            showGraphOverlay = !showGraphOverlay;
            OverlayMessageType messageType = showGraphOverlay ? OverlayMessageType.ShowGraphOverlay : OverlayMessageType.HideGraphOverlay;
            SendMessageToOverlay(messageType);
        }

        public void ToggleBarOverlayVisibility()
        {
            showBarOverlay = !showBarOverlay;
            OverlayMessageType messageType = showBarOverlay ? OverlayMessageType.ShowBarOverlay : OverlayMessageType.HideBarOverlay;
            SendMessageToOverlay(messageType);
        }

        public void ToggleLagIndicatorOverlayVisibility()
        {
            showLagIndicatorOverlay = !showLagIndicatorOverlay;
            OverlayMessageType messageType = showLagIndicatorOverlay ? OverlayMessageType.ShowLagIndicatorOverlay : OverlayMessageType.HideLagIndicatorOverlay;
            SendMessageToOverlay(messageType);
        }

        public bool ProcessFinished()
        {
            bool processFinished = overlay.ProcessFinished();
            // if we have created a steam appid file, delete it
            if (processFinished && System.IO.File.Exists(steamAppIdFile))
            {
                System.IO.File.Delete(steamAppIdFile);
            }

            return processFinished;
        }

        public void StopCapturing()
        {
            if (System.IO.File.Exists(steamAppIdFile))
            {
                System.IO.File.Delete(steamAppIdFile);
            }

            var threads = GetOverlayThreadsAsArray();
            overlay.StopCapture(threads);
            overlayThreads.Clear();
        }

        public void FreeInjectedDlls()
        {
            var processes = GetInjectedProcessesAsArray();
            overlay.FreeInjectedDlls(processes);
        }

        public void StartCaptureExe(string exe, string workingDirectory, string cmdArgs)
        {
            string[] args = cmdArgs.Split(' ');
            string steamRunAppId = "steam://run/";
            foreach (var arg in args)
            {
                if (arg.Contains(steamRunAppId))
                {
                    string appId;
                    appId = arg.Substring(steamRunAppId.Length);

                    // remove this from command arguments
                    // we actually don't use the steam://run command to run the app via the steam client
                    // but instead steam_appid.txt is created to prevent app restart and provide the client
                    // with the correct app id to be able to initialize
                    cmdArgs.Remove(cmdArgs.IndexOf(arg), arg.Length);

                    string appIdFilePath = System.IO.Path.GetDirectoryName(exe) + "\\steam_appid.txt";
                    if (!System.IO.File.Exists(appIdFilePath))
                    {
                        steamAppIdFile = appIdFilePath;

                        using (System.IO.FileStream fs = System.IO.File.Create(appIdFilePath))
                        {
                            Byte[] info = new UTF8Encoding(true).GetBytes(appId);
                            fs.Write(info, 0, info.Length);
                        }
                    }
                    break;
                }
            }

            overlay.StartCaptureExe(exe, workingDirectory, cmdArgs);
        }

        public void StartCaptureAll()
        {
            overlay.StartCaptureAll();
        }
    }
}
