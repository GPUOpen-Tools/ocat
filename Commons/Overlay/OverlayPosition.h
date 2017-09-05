#pragma once

#include "OverlayMessage.h"
#include "OverlayMessage.h"

// Keep in sync with OverlayPosition in Frontend.
enum class OverlayPosition
{
  UpperLeft, // = 0
  UpperRight,// = 1
  LowerLeft, // = 2
  LowerRight // = 3
};

OverlayPosition GetOverlayPositionFromMessageType(OverlayMessageType messageType);
OverlayPosition GetOverlayPositionFromUint(unsigned int overlayPosition);
bool IsLowerOverlayPosition(OverlayPosition overlayPosition);
bool IsLeftOverlayPosition(OverlayPosition overlayPosition);
