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
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Frontend
{
    public class UserInterfaceState : INotifyPropertyChanged
    {
        private bool readyToRecord = false;
        private bool readyToVisualize = false;
        private bool altCheckBoxIsChecked = false;
        private bool disableOverlayDuringCapture = true;
        private string captureOutputFolder;
        private string captureUserNote;
        private string targetExecutable;
        private string recordingState;
        private string csvFile;
        private string timePeriod;
        private int lagIndicatorHotkey;

        private OverlayPosition overlayPosition;
        public OverlayPosition OverlayPositionProperty
        {
            get => overlayPosition;
            set
            {
                overlayPosition = value;
                this.NotifyPropertyChanged("OverlayPositionProperty");
            }
        }

        private InjectionMode injectionMode = InjectionMode.All;
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

        public bool IsReadyToVisualize
        {
            get { return readyToVisualize; }
            set
            {
                readyToVisualize = value;
                this.NotifyPropertyChanged("IsReadyToVisualize");
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
        public bool IsCapturingGlobal
        {
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

        public bool AltCheckBoxIsChecked
        {
            get => altCheckBoxIsChecked;
            set
            {
                altCheckBoxIsChecked = value;
                this.NotifyPropertyChanged("AltCheckBoxIsChecked");
            }
        }

        public bool DisableOverlayDuringCapture
        {
            get => disableOverlayDuringCapture;
            set
            {
                disableOverlayDuringCapture = value;
                this.NotifyPropertyChanged("DisableOverlayDuringCapture");
            }
        }

        public String CaptureOutputFolder
        {
            get { return captureOutputFolder; }
            set
            {
                captureOutputFolder = value;
                this.NotifyPropertyChanged("CaptureOutputFolder");
            }
        }

        public String CaptureUserNote
        {
            get { return captureUserNote; }
            set
            {
                captureUserNote = value;
                this.NotifyPropertyChanged("CaptureUserNote");
            }
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

        public String CsvFile
        {
            get { return csvFile; }
            set
            {
                csvFile = value;
                if (!String.IsNullOrEmpty(csvFile))
                {
                    IsReadyToVisualize = true;
                }
                this.NotifyPropertyChanged("CsvFile");
            }
        }

        public String TimePeriod
        {
            get { return timePeriod; }
            set
            {
                timePeriod = value;
                if (String.IsNullOrEmpty(timePeriod))
                {
                    timePeriod = "0";
                }
                this.NotifyPropertyChanged("TimePeriod");
            }
        }

        public int LagIndicatorHotkey
        {
            get { return lagIndicatorHotkey; }
            set
            {
                lagIndicatorHotkey = value;
                this.NotifyPropertyChanged("LagIndicatorHotkey");
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
}
