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
    // Keep in sync with OverlayPosition in Backend.
    public enum OverlayPosition
    {
        UpperLeft = 0,
        UpperRight = 1,
        LowerLeft = 2,
        LowerRight = 3
    }

    static class OverlayPositionMethods
    {
        public static int ToInt(this OverlayPosition overlayPosition)
        {
            return (int)overlayPosition;
        }

        /// <summary>
        /// Try to retrieve the enum from an int. Defaults to OverlayPosition.UpperRight.
        /// </summary>
        public static OverlayPosition GetFromInt(int overlayPosition)
        {
            if(Enum.IsDefined(typeof(OverlayPosition), overlayPosition))
            {
                return (OverlayPosition)overlayPosition;
            }
            return OverlayPosition.UpperRight;
        }

        public static OverlayMessageType GetMessageType(this OverlayPosition overlayPosition)
        {
            switch(overlayPosition)
            {
                case OverlayPosition.UpperLeft:
                    return OverlayMessageType.UpperLeft;
                case OverlayPosition.UpperRight:
                    return OverlayMessageType.UpperRight;
                case OverlayPosition.LowerLeft:
                    return OverlayMessageType.LowerLeft;
                case OverlayPosition.LowerRight:
                    return OverlayMessageType.LowerRight;
                default:
                    return OverlayMessageType.UpperRight;
            }
        }
    }
}
