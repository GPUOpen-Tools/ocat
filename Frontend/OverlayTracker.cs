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

        HashSet<int> overlayThreads = new HashSet<int>();
        HashSet<int> injectedProcesses = new HashSet<int>();

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

        public void ToggleOverlayVisibility()
        {
            showOverlay = !showOverlay;
            OverlayMessageType messageType = showOverlay ? OverlayMessageType.OVERLAY_ShowOverlay : OverlayMessageType.OVERLAY_HideOverlay;
            SendMessageToOverlay(messageType);
        }

        public bool ProcessFinished()
        {
            return overlay.ProcessFinished();
        }

        public void StopCapturing()
        {
            var threads = GetOverlayThreadsAsArray();
            overlay.StopCapture(threads);
            overlayThreads.Clear();
        }

        public void FreeInjectedDlls()
        {
            var processes = GetInjectedProcessesAsArray();
            overlay.FreeInjectedDlls(processes);
        }

        public void StartCaptureExe(string exe, string cmdArgs)
        {
            overlay.StartCaptureExe(exe, cmdArgs);
        }

        public void StartCaptureAll()
        {
            overlay.StartCaptureAll();
        }
    }
}
