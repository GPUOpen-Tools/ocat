#include "OverlayPosition.h"

OverlayPosition GetOverlayPositionFromMessageType(OverlayMessageType messageType)
{
  switch (messageType)
  {
  case OVERLAY_PositionUpperLeft:
    return OverlayPosition::UPPER_LEFT;
  case OVERLAY_PositionUpperRight:
    return OverlayPosition::UPPER_RIGHT;
  case OVERLAY_PositionLowerLeft:
    return OverlayPosition::LOWER_LEFT;
  case OVERLAY_PositionLowerRight:
    return OverlayPosition::LOWER_RIGHT;
  default:
    return OverlayPosition::UPPER_RIGHT;
  }
}

OverlayPosition GetOverlayPositionFromUint(unsigned int overlayPosition)
{
  // Explicitly state the enums, as we do not want to give out an unexpected but valid enum.
  switch (overlayPosition)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    return static_cast<OverlayPosition>(overlayPosition);
  default:
    return OverlayPosition::UPPER_RIGHT;
  }
}

bool IsLowerOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LOWER_LEFT) || (overlayPosition == OverlayPosition::LOWER_RIGHT);
}

bool IsLeftOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LOWER_LEFT) || (overlayPosition == OverlayPosition::UPPER_LEFT);
}


