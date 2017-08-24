#pragma once

#include "OverlayMessage.h"
#include "OverlayMessage.h"

// Keep in sync with OverlayPosition in Frontend.
enum OverlayPosition
{
  UPPER_LEFT, // = 0
  UPPER_RIGHT,// = 1
  LOWER_LEFT, // = 2
  LOWER_RIGHT // = 3
};

OverlayPosition GetOverlayPositionFromMessageType(OverlayMessageType messageType);
OverlayPosition GetOverlayPositionFromUint(unsigned int overlayPosition);
bool IsLowerOverlayPosition(OverlayPosition overlayPosition);
bool IsLeftOverlayPosition(OverlayPosition overlayPosition);
