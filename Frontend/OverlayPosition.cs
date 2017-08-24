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
                    return OverlayMessageType.OVERLAY_PositionUpperLeft;
                case OverlayPosition.UpperRight:
                    return OverlayMessageType.OVERLAY_PositionUpperRight;
                case OverlayPosition.LowerLeft:
                    return OverlayMessageType.OVERLAY_PositionLowerLeft;
                case OverlayPosition.LowerRight:
                    return OverlayMessageType.OVERLAY_PositionLowerRight;
                default:
                    return OverlayMessageType.OVERLAY_PositionUpperRight;
            }
        }
    }
}
