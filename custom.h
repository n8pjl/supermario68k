#pragma once

#include <stdint.h>
struct settings {
  /*short Jumpkey;
  short RunFireKey;*/
  int16_t Keys[2];
  // 0: Jumpkey
  // 1: RunFireKey
};

extern struct settings Settings;

void Custom();

void Key_configure();

void Load_settings();
