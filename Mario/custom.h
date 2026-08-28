#pragma once

struct settings {
  /*short Jumpkey;
  short RunFireKey;*/
  short Keys[2];
  // 0: Jumpkey
  // 1: RunFireKey
};

extern struct settings Settings;

void Custom();

void Contrast_adj();

void Key_configure();

void Load_settings();
