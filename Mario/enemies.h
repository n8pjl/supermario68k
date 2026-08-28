

#ifndef __enemies__
#define __enemies__

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

typedef struct enemy {
  short X; // Xpos
  short Y; // Ypos

  char Life;
  char Face; // direction. -1=left	1=right
  /*
  short Life;
  short Face;
  */
  short Height;
  short Height2; // the height of the enemy sprite, ie distance from Start of
                 // lightdata (unsigned short* Sprite) to darkdata
  unsigned char Attribs; // b7...b0
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
  unsigned char Mode;
  char Active; // if positioned in the active area
  unsigned char Data0;
  unsigned char Data1;

  unsigned short *Sprite;
  //	unsigned short* Mask;
  char Respawn;
  char Respawn2;
  unsigned char RespawnX;
  unsigned char RespawnY;

  void (*Handler)(struct enemy *Enemy); // func pointer to the func that deals
                                        // with the current kind of enemy
  void (*Die)(struct enemy
                  *Enemy); // func pointer to the func that deals with the death
                           // secuence of the current kind of enemy. the reason
                           // for this is that enemies dies different ways (some
                           // must be hit 2 times, some are animated etc.)
} enemy;

typedef struct shot { // experiment shot
  short X;            // Xpos
  short Y;            // Ypos
  short Height;
  /*char Mode;
  char Data0;
  char Data1;*/
  short Mode; // Changing those 3 variables to short saved bytes
  short Data0;
  short Data1;
  short Data2;
  unsigned short *Sprite;
  // unsigned short* Mask;
  void (*Handler)(struct shot *Shot);
} shot;

extern enemy *Enemies;

extern shot Enemyshots[nr_of_enemyshots];

void Handleenemies();

void Enemy_handler_1(enemy *Enemy);
void Enemy_handler_2(enemy *Enemy);
void Enemy_handler_3(enemy *Enemy);
void Enemy_handler_4(enemy *Enemy);
void Enemy_handler_5(enemy *Enemy);
void Enemy_handler_6(enemy *Enemy);
void Enemy_handler_7(enemy *Enemy);
void Enemy_handler_8(enemy *Enemy);
void Enemy_handler_9(enemy *Enemy);
void Enemy_handler_10(enemy *Enemy);
void Enemy_handler_11(enemy *Enemy);
void Enemy_handler_12(enemy *Enemy);
void Enemy_handler_13(enemy *Enemy);
void Enemy_handler_14(enemy *Enemy);
void Enemy_handler_15(enemy *Enemy);
void Enemy_handler_16(enemy *Enemy);
void Enemy_handler_17(enemy *Enemy);
void Enemy_handler_18(enemy *Enemy);
void Enemy_handler_19(enemy *Enemy);
void Enemy_handler_20(enemy *Enemy);
void Enemy_handler_21(enemy *Enemy);
void Enemy_handler_22(enemy *Enemy);
void Enemy_handler_23(enemy *Enemy);

char Enemy_move_x(enemy *Enemy);

void Dummy_func_(void *P);

void Handle_hardkilled_enemies(enemy *Enemy);
void Handle_dead_upside_down_enemies(enemy *Enemy);

void Enemy_die_1(enemy *Enemy);
void Enemy_die_2(enemy *Enemy);
void Enemy_die_3(enemy *Enemy);
void Enemy_die_4(enemy *Enemy);
void Enemy_die_5(enemy *Enemy);
void Enemy_die_6(enemy *Enemy);
void Enemy_die_7(enemy *Enemy);
void Enemy_die_8(enemy *Enemy);
void Enemy_die_9(enemy *Enemy);
void Enemy_die_10(enemy *Enemy);
void Enemy_die_11(enemy *Enemy);

void Enemy_die(enemy *Enemy);

// void Spawn_bomb(short Nr);
void Spawn_bomb(enemy *Enemy);
void Spawn_bomb2(enemy *Enemy);

void Handle_enemyshots();

void Enemy_add_fireball(short X, short Y, /*char*/ short Dir,
                        /*char*/ short Ydir);

void Enemy_add_hammer(short X, short Y, /*char*/ short Xdir);

void Enemy_add_boomerang(short X, short Y, /*char*/ short Dir);

void Enemy_add_cannonball_horiz(short X, short Y, char Dir);

void Enemy_add_cannonball(short X, short Y, char DirX, char DirY);

void Enemy_add_bounching_fireball(short X, short Y, /*char*/ short Xdir,
                                  /*char*/ short Ydir);

void Enemy_add_underwater_shot(short X, short Y, /*char*/ short Dirx);

void Enemy_fireball_handler(shot *Fireball);

void Hammer_handler(shot *Fireball);

void Enemy_cannonball_horiz_handler(shot *Cannonball);

void Enemy_cannonball_handler(shot *Cannonball);

void Underwater_shot_handler(shot *Cannonball);

void Enemy_boomerang_handler(shot *Fireball);
short Find_free_enemyshot(void);

#endif