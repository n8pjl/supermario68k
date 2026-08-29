#pragma once

#include <stdbool.h>
#include <stdint.h>

extern struct keystate {
  bool enter;
  bool esc;
  bool jump;
  bool up;
  bool down;
  bool left;
  bool right;
  bool run;
} Keystate, Previous_keystate;

void WaitKeyReleased(void);

void WaitKeyPress(void);

void ScanKeys(void);
