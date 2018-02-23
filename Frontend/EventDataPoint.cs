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