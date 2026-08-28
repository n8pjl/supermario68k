

#ifndef __custom__
#define __custom__

typedef struct {
  /*short Jumpkey;
  short RunFireKey;*/
  short Keys[2];
  // 0: Jumpkey
  // 1: RunFireKey

} settings;

extern settings Settings;

void Custom();

void Contrast_adj();

void Key_configure();

void Load_settings();

#endif