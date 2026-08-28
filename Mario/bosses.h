
#ifndef __bosses__
#define __bosses__

#include "enemies.h"

typedef struct {
  short X; // Xpos
  short Y; // Ypos
  char Active;
  char Life;
  char Face; // direction. -1=left	1=right
  short Height;
  short Height2; // the height of the boss sprite, ie distance from Start of
                 // lightdata (unsigned short* Sprite) to darkdata

  char Data0;
  char Data1;
  // char Data2;
  // char Data3;

  unsigned char Mode;
  unsigned char State;

  unsigned short *Sprite;
  unsigned short *Mask;

  void (*Handler)(void *Boss); // func pointer to the func that deals with the
                               // current kind of boss
  void (*Die)(void *Boss); // func pointer to the func that deals with the death
                           // secuence of the current kind of boss.

} boss;

extern boss *BossG;

inline void Handlebosses();

short Boss_free_down(boss *Boss);

short Boss_free_up(boss *Boss);

void Boss_handler_1(boss *Boss);

void Boss_handler_2(boss *Boss);

void Bowser_handler(boss *Boss);

// void Dummy_boss(boss *Boss);

// void Boss_dummy_h(boss *Boss);

void Boss_die_1(boss *Boss);

void Bowser_die(boss *Boss);

// void Boss_die_2(boss *Boss);

void Add_boss_shot(short X, short Y, char DirX, char DirY);

void Add_big_fireball_shot(short X, short Y, char DirX, char DirY);

void Boss_shot_handler(shot *Shot);

#endif