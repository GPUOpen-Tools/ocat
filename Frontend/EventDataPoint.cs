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
using OxyPlot;

namespace Frontend
{
    public struct EventDataPoint : ICodeGenerating, IEquatable<EventDataPoint>
    {
        public static readonly EventDataPoint Undefined = new EventDataPoint(double.NaN, double.NaN, "");

        internal readonly double x;
        internal readonly double y;
        internal readonly string eventTitle;

        public EventDataPoint(double x, double y, string eventTitle)
        {
            this.x = x;
            this.y = y;
            this.eventTitle = eventTitle;
        }

        public double X
        {
            get
            {
                return this.x;
            }
        }
        public double Y
        {
            get
            {
                return this.y;
            }
        }
        public string EventTitle
        {
            get
            {
                return eventTitle;
            }
        }

        public bool Equals(EventDataPoint other)
        {
            return (X == other.X && Y == other.Y && EventTitle == other.EventTitle);
        }
        public bool IsDefined()
        {
            return (double.IsNaN(x) && double.IsNaN(y));
        }
        public string ToCode()
        {
            return CodeGenerator.FormatConstructor(this.GetType(), "{0},{1}", this.x, this.y);
        }
        public override string ToString()
        {
            return this.eventTitle + ": " + this.x + " " + this.y;
        }
    }
}