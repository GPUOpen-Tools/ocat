using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Frontend
{
    public enum RecordingDetail
    {
        // These have to match the arguments given to PresentMon in the PresentMonInterface.
        Simple = 0,
        Normal = 1,
        Verbose = 2
    }

    static class RecordingDetailMethods
    {
        public static string[] AsStringArray()
        {
            return Enum.GetNames(typeof(RecordingDetail));
        }

        public static int ToInt(this RecordingDetail recordingDetail)
        {
            return (int)recordingDetail;
        }

        /// <summary>
        /// Try to parse recording detail from a string. Defaults to RecordingDetail.Simple.
        /// </summary>
        public static RecordingDetail GetFromString(string recordingDetailString)
        {
            RecordingDetail recordingDetail;
            if(Enum.TryParse<RecordingDetail>(recordingDetailString, true, out recordingDetail))
            {
                return recordingDetail;
            }
            return RecordingDetail.Simple;
        }
    }
}
