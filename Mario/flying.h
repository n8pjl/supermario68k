#pragma once

struct flying_platform {
  short X;
  short Y;

  short Width; // char ???

  // char Active;
  short Active;

  short Data0;
  short Data1;
  short Data2;
  short Data3;

  // char PlayerOn;
  short PlayerOn;
  //	short DrawMode;
  unsigned short Sprite;

  void (*Handler)(struct flying_platform *Flying_platform);
};

extern struct flying_platform *Flying_platforms;

void Handle_flying_platforms();

void Platform_handler_1(struct flying_platform *Platform);

void Platform_handler_2(struct flying_platform *Platform);

void Platform_handler_3(struct flying_platform *Platform);
// short Sign(short);
