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
        private string recordingOutputFolder;
        private string targetExecutable;
        private string recordingState;
        private string csvFile;

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

        public String RecordingOutputFolder
        {
            get { return recordingOutputFolder; }
            set
            {
                recordingOutputFolder = value;
                this.NotifyPropertyChanged("RecordingOutputFolder");
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
