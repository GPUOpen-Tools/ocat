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
