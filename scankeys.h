#pragma once

struct keystate {
  short Enter;
  short Esc;
  short Jump;
  short Down;
  short Left;
  short Right;
  short Up;
  short Run;
  short Plus;
  short Minus;
  //	short Fire;//not in use any more

  //	short Keydetect;//not neccessarry
};

extern struct keystate Keystate, Previous_keystate;

void WaitKeyReleased();

void WaitKeyPress();

void Scankeys();
