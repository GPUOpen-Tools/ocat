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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Frontend
{
    public class RecordingOptions
    {
        public int toggleCaptureHotkey;
        public int toggleOverlayHotkey;
        public int toggleGraphOverlayHotkey;
        public int toggleBarOverlayHotkey;
        public int toggleLagIndicatorOverlayHotkey;
        public int lagIndicatorHotkey;
        public int captureTime;
        public int captureDelay;
        public bool captureAll;
        public bool audioCue;
        public bool injectOnStart;
        public bool altKeyComb;
        public bool disableOverlayWhileRecording;
        public int overlayPosition;
        public string captureOutputFolder;

        private const string section = "Recording";

        public RecordingOptions()
        {
            toggleCaptureHotkey = 0x79; // F10
            toggleOverlayHotkey = 0x78; // F9
            toggleGraphOverlayHotkey = 0x76; // F7
            toggleBarOverlayHotkey = 0x77; // F8
            toggleLagIndicatorOverlayHotkey = 0x75; // F6
            lagIndicatorHotkey = 0x91; // SCROLL_LOCK
            captureTime = 60;
            captureDelay = 0;
            captureAll = true;
            audioCue = true;
            altKeyComb = false;
            disableOverlayWhileRecording = false;
            injectOnStart = true;
            overlayPosition = OverlayPosition.UpperRight.ToInt();
            const string outputFolderPath = ("\\OCAT\\Captures");
            captureOutputFolder = System.Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + outputFolderPath;
        }

        public void Save(string path)
        {
            using (var iniFile = new StreamWriter(path, false, Encoding.Unicode))
            {
                iniFile.WriteLine("[" + section + "]");
                iniFile.WriteLine("toggleCaptureHotkey=" + toggleCaptureHotkey);
                iniFile.WriteLine("toggleOverlayHotkey=" + toggleOverlayHotkey);
                iniFile.WriteLine("toggleFramegraphOverlayHotkey=" + toggleGraphOverlayHotkey);
                iniFile.WriteLine("toggleColoredBarOverlayHotkey=" + toggleBarOverlayHotkey);
                iniFile.WriteLine("toggleLagIndicatorOverlayHotkey=" + toggleLagIndicatorOverlayHotkey);
                iniFile.WriteLine("lagIndicatorHotkey=" + lagIndicatorHotkey);
                iniFile.WriteLine("overlayPosition=" + overlayPosition);
                iniFile.WriteLine("captureTime=" + captureTime);
                iniFile.WriteLine("captureDelay=" + captureDelay);
                iniFile.WriteLine("captureAllProcesses=" + Convert.ToInt32(captureAll));
                iniFile.WriteLine("audioCue=" + Convert.ToInt32(audioCue));
                iniFile.WriteLine("altKeyComb=" + Convert.ToInt32(altKeyComb));
                iniFile.WriteLine("disableOverlayWhileRecording=" + Convert.ToInt32(disableOverlayWhileRecording));
                iniFile.WriteLine("injectOnStart=" + Convert.ToInt32(injectOnStart));
                iniFile.WriteLine("captureOutputFolder=" + captureOutputFolder);
            }
        }

        public void Load(string path)
        {
            if (ConfigurationFile.Exists())
            {
                toggleCaptureHotkey = ConfigurationFile.ReadInt(section, "toggleCaptureHotkey", toggleCaptureHotkey, path);
                toggleOverlayHotkey = ConfigurationFile.ReadInt(section, "toggleOverlayHotkey", toggleOverlayHotkey, path);
                toggleGraphOverlayHotkey = ConfigurationFile.ReadInt(section, "toggleFramegraphOverlayHotkey", toggleGraphOverlayHotkey, path);
                toggleBarOverlayHotkey = ConfigurationFile.ReadInt(section, "toggleColoredBarOverlayHotkey", toggleBarOverlayHotkey, path);
                toggleLagIndicatorOverlayHotkey = ConfigurationFile.ReadInt(section, "toggleLagIndicatorOverlayHotkey", toggleLagIndicatorOverlayHotkey, path);
                lagIndicatorHotkey = ConfigurationFile.ReadInt(section, "lagIndicatorHotkey", lagIndicatorHotkey, path);
                overlayPosition = ConfigurationFile.ReadInt(section, "overlayPosition", overlayPosition, path);
                captureTime = ConfigurationFile.ReadInt(section, "captureTime", captureTime, path);
                captureDelay = ConfigurationFile.ReadInt(section, "captureDelay", captureDelay, path);
                captureAll = ConfigurationFile.ReadBool(section, "captureAllProcesses", path);
                audioCue = ConfigurationFile.ReadBool(section, "audioCue", path);
                altKeyComb = ConfigurationFile.ReadBool(section, "altKeyComb", path);
                disableOverlayWhileRecording = ConfigurationFile.ReadBool(section, "disableOverlayWhileRecording", path);
                injectOnStart = ConfigurationFile.ReadBool(section, "injectOnStart", path);
                captureOutputFolder = ConfigurationFile.ReadString(section, "captureOutputFolder", path);
            }
        }
    }

    static class ConfigurationFile
    {
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        static extern int GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault, StringBuilder lpReturnedString, int nSize, string lpFileName);
        [DllImport("kernel32", CharSet = CharSet.Ansi)]
        static extern int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);
        [DllImport("kernel32")]
        static extern int GetLastError();

        public static string GetPath()
        {
            const string iniFileName = ("\\OCAT\\Config\\settings.ini");
            return System.Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + iniFileName;
        }

        public static void Save(RecordingOptions recordingOptions)
        {
            string path = GetPath();
            recordingOptions.Save(path);
        }

        public static bool Exists()
        {
            return File.Exists(GetPath());
        }

        public static string ReadString(string section, string key, string path)
        {
            int bufferSize = 256;
            int stringSize = 0;
            var returnValue = new StringBuilder(bufferSize);
            do
            {
                returnValue.Capacity = bufferSize;
                stringSize = GetPrivateProfileString(section, key, "", returnValue, bufferSize, path);
                bufferSize *= 2;
            }
            //loop until capacity is higher than string size including null terminator
            while (returnValue.Capacity - 1 == stringSize);

            return returnValue.ToString();
        }

        public static int ReadInt(string section, string key, int defaultValue, string path)
        {
            return GetPrivateProfileInt(section, key, defaultValue, path);
        }

        public static bool ReadBool(string section, string key, string path)
        {
            int value = GetPrivateProfileInt(section, key, 0, path);
            switch (value)
            {
                case 0:
                    return false;
                case 1:
                    return true;
                default:
                    System.Diagnostics.Debug.Print("Retrieved invalid bool from config, seting to true");
                    return true;
            }
        }
    }
}
