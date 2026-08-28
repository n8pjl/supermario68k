#include "scankeys.h"
#include "compat/kbd.h"
#include "custom.h"
#include <stdint.h>

struct keystate Keystate, Previous_keystate;

void WaitKeyReleased() {
  while (_rowread(0))
    browser_yield();
}

void WaitKeyPress() {
  while (_rowread(0))
    browser_yield(); // wait until key released
  while (!_rowread(0))
    browser_yield(); // wait until key pressed
};

#ifdef PRODUCE_TI89_CODE
// #if TI89

void Scankeys() { // ti 89 and ti89T code
  uint16_t Row;

  // New V 1.03: Configureable keys
  uint16_t Masks[4] = {0b00010000, 0b00100000, 0b01000000, 0b10000000};
  uint16_t RunMask = Masks[Settings.Keys[1]];
  uint16_t JumpMask = Masks[Settings.Keys[0]];
  ;
  // End of new

  Row = _rowread(0b1111110);
  Keystate.Up = Row & 0b00000001;
  Keystate.Left = Row & 0b00000010;
  Keystate.Right = Row & 0b00001000;
  Keystate.Down = Row & 0b00000100;
  Keystate.Jump = Row & JumpMask;
  Keystate.Run = Row & RunMask;

  // Keystate.Jump  = Row & 0b00010000;//2nd
  // Keystate.Run   = Row & 0b01000000;//diamond

  Row = _rowread(0b0111111);
  Keystate.Esc = Row & 0b00000001;

  Row = _rowread(0b1111101);
  Keystate.Enter = Row & 0b00000001;
  Keystate.Plus = Row & 0b00000010;
  Keystate.Minus = Row & 0b00000100;

  int16_t MaskLeft = 0;
  if (Keystate.Left && Keystate.Right && Previous_keystate.Right) {

    MaskLeft = 1;
  }
  if (Keystate.Right && Keystate.Left && Previous_keystate.Left) {
    Keystate.Right = 0;
  }
  if (MaskLeft) {
    Keystate.Left = 0;
  }
}

#endif

#ifdef PRODUCE_TI92PLUS_CODE // compiling for the ti92p
// #if TI92PLUS

void Scankeys() { // ti 92+ code
  uint16_t Row;

  // New V 1.03: Configureable keys
  uint16_t Masks[4] = {0b00001000, 0b00010000, 0b00010000, 0b00010000};
  uint16_t RowMasks[4] = {0b1111111110, 0b1110111111, 0b1111101111,
                          0b1111011111};

  uint16_t RunMask = Masks[Settings.Keys[1]];
  uint16_t JumpMask = Masks[Settings.Keys[0]];
  uint16_t RunRowMask = RowMasks[Settings.Keys[1]];
  uint16_t JumpRowMask = RowMasks[Settings.Keys[0]];
  ;

  Keystate.Run = _rowread(RunRowMask) & RunMask;
  Keystate.Jump = _rowread(JumpRowMask) & JumpMask;
  // End of new

  Row = _rowread(0b1111111110); // 0
  Keystate.Up = Row & 0b00100000;
  Keystate.Left = Row & 0b00010000;
  Keystate.Right = Row & 0b01000000;
  Keystate.Down = Row & 0b10000000;

  Row = _rowread(0b1110111111); // 6
  // Keystate.Jump  = Row & 0b00010000;//F1
  Keystate.Enter = Row & 0b01000000; // Enter2

  Row = _rowread(0b1011111111); // 8
  Keystate.Esc = Row & 0b01000000;
  Keystate.Plus = Row & 0b00010000;

  Row = _rowread(0b0111111111); // 9
  Keystate.Enter = Keystate.Enter | (Row & 0b00000010);
  Keystate.Minus = Row & 0b00000001;

  //	Row = _rowread(0b1111101111);//4
  // Keystate.Run   = Row & 0b00010000;//F2

  int16_t MaskLeft = 0;
  if (Keystate.Left && Keystate.Right && Previous_keystate.Right) {

    MaskLeft = 1;
  }
  if (Keystate.Right && Keystate.Left && Previous_keystate.Left) {
    Keystate.Right = 0;
  }
  if (MaskLeft) {
    Keystate.Left = 0;
  }
}

#endif

#ifdef PRODUCE_V200_CODE // compiling for the V200
// #if V200

void Scankeys() { // ti v200 code
  uint16_t Row;

  // New V 1.03: Configureable keys
  uint16_t Masks[4] = {0b00001000, 0b00001000, 0b00010000, 0b00001000};
  uint16_t RowMasks[4] = {0b1111111110, 0b0111111111, 0b1110111111,
                          0b1111111101};

  uint16_t RunMask = Masks[Settings.Keys[1]];
  uint16_t JumpMask = Masks[Settings.Keys[0]];
  uint16_t RunRowMask = RowMasks[Settings.Keys[1]];
  uint16_t JumpRowMask = RowMasks[Settings.Keys[0]];
  ;

  Keystate.Run = _rowread(RunRowMask) & RunMask;
  Keystate.Jump = _rowread(JumpRowMask) & JumpMask;
  // End of new

  Row = _rowread(0b1111111110);
  Keystate.Up = Row & 0b00100000;
  Keystate.Left = Row & 0b00010000;
  Keystate.Right = Row & 0b01000000;
  Keystate.Down = Row & 0b10000000;
  // Keystate.Jump  = Row & 0b00001000;//HAND

  Row = _rowread(0b1011111111);
  Keystate.Esc = Row & 0b01000000;
  Keystate.Plus = Row & 0b00010000;

  Row = _rowread(0b0111111111);
  Keystate.Enter = Row & 0b00000010;
  // Keystate.Run   = Row & 0b00001000;//Q
  Keystate.Minus = Row & 0b00000001;

  int16_t MaskLeft = 0;
  if (Keystate.Left && Keystate.Right && Previous_keystate.Right) {

    MaskLeft = 1;
  }
  if (Keystate.Right && Keystate.Left && Previous_keystate.Left) {
    Keystate.Right = 0;
  }
  if (MaskLeft) {
    Keystate.Left = 0;
  }
}

#endif