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
        private string targetExecutable;
        private string recordingState;

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
}
