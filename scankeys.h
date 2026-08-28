#pragma once

#include <stdint.h>
struct keystate {
  int16_t Enter;
  int16_t Esc;
  int16_t Jump;
  int16_t Down;
  int16_t Left;
  int16_t Right;
  int16_t Up;
  int16_t Run;
  int16_t Plus;
  int16_t Minus;
  //	short Fire;//not in use any more

  //	short Keydetect;//not neccessarry
};

extern struct keystate Keystate, Previous_keystate;

void WaitKeyReleased();

void WaitKeyPress();

void Scankeys();
