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
        public static string ToString(this RecordingDetail recordingDetail)
        {
            return Enum.GetName(typeof(RecordingDetail), recordingDetail);
        }

        public static string[] AsStringArray()
        {
            return Enum.GetNames(typeof(RecordingDetail));
        }

        public static int ToInt(this RecordingDetail recordingDetail)
        {
            return (int)recordingDetail;
        }

        public static RecordingDetail GetFromInt(int detail)
        {
            var length = AsStringArray().Length;
            return (RecordingDetail)Math.Max(0, Math.Min(length, detail));
        }

        /// <summary>
        /// Try to parse recording detail from a string. Defaults to RecordingDetail.Simple.
        /// </summary>
        public static RecordingDetail GetFromString(string detailString)
        {
            RecordingDetail detail;
            if(Enum.TryParse<RecordingDetail>(detailString, true, out detail))
            {
                return detail;
            }
            return RecordingDetail.Simple;
        }
    }
}
