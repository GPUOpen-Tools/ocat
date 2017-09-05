#include "OverlayPosition.h"

OverlayPosition GetOverlayPositionFromMessageType(OverlayMessageType messageType)
{
  switch (messageType)
  {
  case OverlayMessageType::UpperLeft:
    return OverlayPosition::UpperLeft;
  case OverlayMessageType::UpperRight:
    return OverlayPosition::UpperRight;
  case OverlayMessageType::LowerLeft:
    return OverlayPosition::LowerLeft;
  case OverlayMessageType::LowerRight:
    return OverlayPosition::LowerRight;
  default:
    return OverlayPosition::UpperRight;
  }
}

OverlayPosition GetOverlayPositionFromUint(unsigned int overlayPosition)
{
  // Explicitly state the enums, as we do not want to give out an unexpected but valid enum.
  switch (overlayPosition)
  {
  case 0:
    return OverlayPosition::UpperLeft;
  case 1:
    return OverlayPosition::UpperRight;
  case 2:
    return OverlayPosition::LowerLeft;
  case 3:
    return OverlayPosition::LowerRight;
  default:
    return OverlayPosition::UpperRight;
  }
}

bool IsLowerOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LowerLeft) || (overlayPosition == OverlayPosition::LowerRight);
}

bool IsLeftOverlayPosition(OverlayPosition overlayPosition)
{
  return (overlayPosition == OverlayPosition::LowerLeft) || (overlayPosition == OverlayPosition::UpperLeft);
}


