#pragma once

#include <stdint.h>
#define enemy_fallspeed 3

// nr of enemy fireballs simultaniously
#define nr_of_enemy_smallshots 5
#define nr_of_enemyshots 5
#define enemy_fireball_speed 2

// active screen:

#define active_x_left (FgX - 16 * 4)
#define active_x_right (FgX + 240 + 16 * 4)
#define active_y_lower (FgY + 160 + 16 * 4)
#define active_y_upper (FgY - 16 * 4)

struct enemy {
  int16_t X; // Xpos
  int16_t Y; // Ypos

  char Life;
  char Face; // direction. -1=left	1=right
  /*
  short Life;
  short Face;
  */
  int16_t Height;
  int16_t Height2; // the height of the enemy sprite, ie distance from Start of
                 // lightdata (unsigned short* Sprite) to darkdata
  uint8_t Attribs; // b7...b0
                         // b7 set: dies when jumped on
                         // b6 set: dies when hit by fireball
                         // b5 set: gravity enabled
                         // b4 set: player dies when contact
                         // b3 set: use Enemy_die() instead of Individual.Die
                         // when hit by shell,fireball,racoontale etc b2 set:
                         // allowed to (temporary) go outside level b1 set:
                         // enemy dies when contact b0 set: dies when hit by
                         // racoontale

  char Jumping;
  uint8_t Mode;
  char Active; // if positioned in the active area
  uint8_t Data0;
  uint8_t Data1;

  uint16_t *Sprite;
  //	unsigned short* Mask;
  char Respawn;
  char Respawn2;
  uint8_t RespawnX;
  uint8_t RespawnY;

  void (*Handler)(struct enemy *Enemy); // func pointer to the func that deals
                                        // with the current kind of enemy
  void (*Die)(struct enemy
                  *Enemy); // func pointer to the func that deals with the death
                           // secuence of the current kind of enemy. the reason
                           // for this is that enemies dies different ways (some
                           // must be hit 2 times, some are animated etc.)
};

struct shot { // experiment shot
  int16_t X;    // Xpos
  int16_t Y;    // Ypos
  int16_t Height;
  /*char Mode;
  char Data0;
  char Data1;*/
  int16_t Mode; // Changing those 3 variables to short saved bytes
  int16_t Data0;
  int16_t Data1;
  int16_t Data2;
  uint16_t *Sprite;
  // unsigned short* Mask;
  void (*Handler)(struct shot *Shot);
};

extern struct enemy *Enemies;

extern struct shot Enemyshots[nr_of_enemyshots];

void Handleenemies();

void Enemy_handler_1(struct enemy *Enemy);
void Enemy_handler_2(struct enemy *Enemy);
void Enemy_handler_3(struct enemy *Enemy);
void Enemy_handler_4(struct enemy *Enemy);
void Enemy_handler_5(struct enemy *Enemy);
void Enemy_handler_6(struct enemy *Enemy);
void Enemy_handler_7(struct enemy *Enemy);
void Enemy_handler_8(struct enemy *Enemy);
void Enemy_handler_9(struct enemy *Enemy);
void Enemy_handler_10(struct enemy *Enemy);
void Enemy_handler_11(struct enemy *Enemy);
void Enemy_handler_12(struct enemy *Enemy);
void Enemy_handler_13(struct enemy *Enemy);
void Enemy_handler_14(struct enemy *Enemy);
void Enemy_handler_15(struct enemy *Enemy);
void Enemy_handler_16(struct enemy *Enemy);
void Enemy_handler_17(struct enemy *Enemy);
void Enemy_handler_18(struct enemy *Enemy);
void Enemy_handler_19(struct enemy *Enemy);
void Enemy_handler_20(struct enemy *Enemy);
void Enemy_handler_21(struct enemy *Enemy);
void Enemy_handler_22(struct enemy *Enemy);
void Enemy_handler_23(struct enemy *Enemy);

char Enemy_move_x(struct enemy *Enemy);

void Dummy_func_(void *P);

void Handle_hardkilled_enemies(struct enemy *Enemy);
void Handle_dead_upside_down_enemies(struct enemy *Enemy);

void Enemy_die_1(struct enemy *Enemy);
void Enemy_die_2(struct enemy *Enemy);
void Enemy_die_3(struct enemy *Enemy);
void Enemy_die_4(struct enemy *Enemy);
void Enemy_die_5(struct enemy *Enemy);
void Enemy_die_6(struct enemy *Enemy);
void Enemy_die_7(struct enemy *Enemy);
void Enemy_die_8(struct enemy *Enemy);
void Enemy_die_9(struct enemy *Enemy);
void Enemy_die_10(struct enemy *Enemy);
void Enemy_die_11(struct enemy *Enemy);

void Enemy_die(struct enemy *Enemy);

void Spawn_bomb(struct enemy *Enemy);
void Spawn_bomb2(struct enemy *Enemy);

void Handle_enemyshots();

void Enemy_add_fireball(int16_t X, int16_t Y, /*char*/ int16_t Dir,
                        /*char*/ int16_t Ydir);

void Enemy_add_hammer(int16_t X, int16_t Y, /*char*/ int16_t Xdir);

void Enemy_add_boomerang(int16_t X, int16_t Y, /*char*/ int16_t Dir);

void Enemy_add_cannonball_horiz(int16_t X, int16_t Y, char Dir);

void Enemy_add_cannonball(int16_t X, int16_t Y, char DirX, char DirY);

void Enemy_add_bounching_fireball(int16_t X, int16_t Y, /*char*/ int16_t Xdir,
                                  /*char*/ int16_t Ydir);

void Enemy_add_underwater_shot(int16_t X, int16_t Y, /*char*/ int16_t Dirx);

void Enemy_fireball_handler(struct shot *Fireball);

void Hammer_handler(struct shot *Fireball);

void Enemy_cannonball_horiz_handler(struct shot *Cannonball);

void Enemy_cannonball_handler(struct shot *Cannonball);

void Underwater_shot_handler(struct shot *Cannonball);

void Enemy_boomerang_handler(struct shot *Fireball);
int16_t Find_free_enemyshot(void);
