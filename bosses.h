#pragma once

#include <stdint.h>
#include "enemies.h"

struct boss {
  int16_t X; // Xpos
  int16_t Y; // Ypos
  char Active;
  char Life;
  char Face; // direction. -1=left	1=right
  int16_t Height;
  int16_t Height2; // the height of the boss sprite, ie distance from Start of
                 // lightdata (unsigned short* Sprite) to darkdata

  char Data0;
  char Data1;
  // char Data2;
  // char Data3;

  uint8_t Mode;
  uint8_t State;

  uint16_t *Sprite;
  uint16_t *Mask;

  void (*Handler)(void *Boss); // func pointer to the func that deals with the
                               // current kind of boss
  void (*Die)(void *Boss); // func pointer to the func that deals with the death
                           // secuence of the current kind of boss.
};

extern struct boss *BossG;

inline void Handlebosses();

int16_t Boss_free_down(struct boss *Boss);

int16_t Boss_free_up(struct boss *Boss);

void Boss_handler_1(struct boss *Boss);

void Boss_handler_2(struct boss *Boss);

void Bowser_handler(struct boss *Boss);

// void Dummy_boss(boss *Boss);

// void Boss_dummy_h(boss *Boss);

void Boss_die_1(struct boss *Boss);

void Bowser_die(struct boss *Boss);

// void Boss_die_2(boss *Boss);

void Add_boss_shot(int16_t X, int16_t Y, char DirX, char DirY);

void Add_big_fireball_shot(int16_t X, int16_t Y, char DirX, char DirY);

void Boss_shot_handler(struct shot *Shot);
