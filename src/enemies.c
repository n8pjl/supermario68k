

#include "enemies.h"
#include "bounch.h"
#include "comp.h"
#include "compat/extgraph.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "player.h"
#include "render.h"
#include "shells.h"
#include <stdint.h>
#include <stdlib.h>

struct enemy *Enemies;

struct shot Enemyshots[nr_of_enemyshots];

void Handleenemies()
{
	int16_t Curr_enemy;
	int16_t C;

	for (Curr_enemy = 0; Curr_enemy < Leveldata.Nr_of_enemies;
	     Curr_enemy++) {
		if (Enemies[Curr_enemy].Life) {
			// if( (FgX<Enemies[Curr_enemy].X+64) &&
			// (FgX+272>Enemies[Curr_enemy].X+64) && (FgY<Enemies[Curr_enemy].Y+64) &&
			// (FgY+160>Enemies[Curr_enemy].Y+64) ){		//if inside active area
			// (big_vscreen+64 in all directions)
			if ((Enemies[Curr_enemy].Respawn < 0) ||
			    ((active_x_left < Enemies[Curr_enemy].X) &&
			     (active_x_right > Enemies[Curr_enemy].X) &&
			     (active_y_upper < Enemies[Curr_enemy].Y) &&
			     (active_y_lower > Enemies[Curr_enemy].Y))) {
				Enemies[Curr_enemy].Handler(
					&Enemies[Curr_enemy]); // execute whatever this enemy is doing

				// gravity
				if (!(Enemies[Curr_enemy].Jumping) &&
				    Enemies[Curr_enemy].Attribs &
					    0b00100000) { // not jumping and gravity enabled
					C = Free_down(
						Enemies[Curr_enemy].X,
						Enemies[Curr_enemy].Y +
							Enemies[Curr_enemy]
								.Height);
					Enemies[Curr_enemy].Y +=
						((C >= enemy_fallspeed) ?
							 (enemy_fallspeed) :
							 (C));

					if (!(Enemies[Curr_enemy].Attribs &
					      0b00000100) &&
					    ((Enemies[Curr_enemy].Y +
					      Enemies[Curr_enemy].Height) >
					     (Leveldata.Height * 16)))
						Enemies[Curr_enemy].Life = 0;
				}

				if (TestCollide162h_R(
					    Player.X, Player.Y,
					    Enemies[Curr_enemy].X,
					    Enemies[Curr_enemy].Y,
					    Player.Height,
					    Enemies[Curr_enemy].Height,
					    Mariosprites +
						    2 * Marioanimtab[SavePlayer.Spritebase]
								    [SavePlayer
									     .Spritenr],
					    Enemies[Curr_enemy].Sprite)) {
					if (((SavePlayer.Attribs & 0b10000000) &&
					     Enemies[Curr_enemy].Active >
						     0 /*|| (Player.Attribs & 0b00000010)*/)) { // super mario
						// (got a star)
						if ((Enemies[Curr_enemy].Attribs &
						     0b00001000)) {
							Enemy_die(
								&Enemies[Curr_enemy]);
						} else {
							Enemies[Curr_enemy].Die(
								&Enemies[Curr_enemy]);
						}
						Enemies[Curr_enemy].Active =
							-1; // to prevent killing the same enemy a tonn of times when
						// mario has the star
						continue;
					} else {
						if ((Enemies[Curr_enemy].Attribs &
						     0b00000010) ||
						    ((Enemies[Curr_enemy]
							      .Attribs &
						      0b10000000) &&
						     Player.IsFalling &&
						     (Player.Y <
						      Enemies[Curr_enemy].Y))) {
							Enemies[Curr_enemy].Die(
								&Enemies[Curr_enemy]); // kill enemy;
							Player.IsJumping =
								0.75 *
								jumpheight; // enable rejump
							Player.Yoffset = -2;
							continue;
						} else {
							if ((Enemies[Curr_enemy]
								     .Attribs &
							     0b00010000))
								Player_die();
						}
					}
				}

				for (C = 0; C < nr_of_bounching_tiles; C++) {
					if (Bounching_tiles[C].Active) {
						if (BoundsCollide(
							    Bounching_tiles[C].X,
							    Bounching_tiles[C].Y,
							    16,
							    Enemies[Curr_enemy]
								    .X,
							    Enemies[Curr_enemy]
								    .Y,
							    Enemies[Curr_enemy]
								    .Height)) {
							Enemies[Curr_enemy].Die(
								&Enemies[Curr_enemy]); // kill enemy;
						}
					}
				}

				if (Player.Racoonspin) { // test if smashed by racoon suit tale

					if ((Enemies[Curr_enemy].Attribs &
					     0b00000001) &&
					    BoundsCollide2(
						    Player.X +
							    (Player.Face == 1 ?
								     -9 :
								     15),
						    Player.Y + 17, 6, 9,
						    Enemies[Curr_enemy].X,
						    Enemies[Curr_enemy].Y,
						    Enemies[Curr_enemy].Height,
						    16)) {
						if ((Enemies[Curr_enemy].Attribs &
						     0b00001000)) {
							Enemy_die(
								&Enemies[Curr_enemy]);
						} else {
							Enemies[Curr_enemy].Die(
								&Enemies[Curr_enemy]);
						}
					}
				}

				// test for collition	with shell
				for (C = 0; C < max_nr_of_shells; C++) {
					if (Shells[C].Active) {
						if (BoundsCollide(
							    Shells[C].X,
							    Shells[C].Y, 16,
							    Enemies[Curr_enemy]
								    .X,
							    Enemies[Curr_enemy]
								    .Y,
							    Enemies[Curr_enemy]
								    .Height)) {
							if ((Enemies[Curr_enemy]
								     .Attribs &
							     0b00001000)) {
								Enemy_die(
									&Enemies[Curr_enemy]);
								// DrawStr(10,10,"voff",A_NORMAL);//Test
							} else {
								Enemies[Curr_enemy]
									.Die(&Enemies[Curr_enemy]);
								// DrawStr(10,10,"mjau",A_NORMAL);
							}
						}
					}
				}

				// test for hit by fireball
				for (C = 0; C < nr_of_player_smallshots; C++) {
					if (Player_smallshots[C]
						    .Mode) { // if active
						if (BoundsCollide2(
							    Player_smallshots[C]
									    .X +
								    1,
							    Player_smallshots[C]
									    .Y +
								    1,
							    6, 6,
							    Enemies[Curr_enemy]
								    .X,
							    Enemies[Curr_enemy]
								    .Y,
							    Enemies[Curr_enemy]
								    .Height,
							    16)) { // hit by fireball
							Player_smallshots[C]
								.Mode =
								0; // kill fireball

							if (Enemies[Curr_enemy]
								    .Attribs &
							    0b01000000) { // if enemy is killable by fireball
								if ((Enemies[Curr_enemy]
									     .Attribs &
								     0b00001000)) {
									Enemy_die(
										&Enemies[Curr_enemy]);
								} else {
									Enemies[Curr_enemy]
										.Die(&Enemies[Curr_enemy]);
								}
							} else {
								Add_dustsky(
									Player_smallshots[C]
										.X,
									Player_smallshots[C]
										.Y);
							}
						}
						// draw ?
					} // end of active (mode!=0)
				}

				// draw the current enemy
				// GrayClipSprite16_MASK_R(Curr_enemy->X-FgX,Curr_enemy->Y-FgY,Curr_enemy->Height,Curr_enemy->Sprite,Curr_enemy->Sprite+Curr_enemy->Height,Curr_enemy->Mask,Curr_enemy->Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
				// //USING CLIPPED SPRITE TO AVOID DRAVING OUTSIDE GFX MEM

			} // end of active

		} //! life
		else {
			if (Enemies[Curr_enemy].Respawn &&
			    !((active_x_left <
			       (Enemies[Curr_enemy].RespawnX * 16)) &&
			      (active_x_right >
			       (Enemies[Curr_enemy].RespawnX * 16)) &&
			      (active_y_upper <
			       (Enemies[Curr_enemy].RespawnY * 16)) &&
			      (active_y_lower > (Enemies[Curr_enemy].RespawnY *
						 16)))) { // Respawn

				Enemies[Curr_enemy].X =
					Enemies[Curr_enemy].RespawnX * 16;
				Enemies[Curr_enemy].Y =
					Enemies[Curr_enemy].RespawnY * 16;

				// only "handler_1" enemies supported for now...(more might come..)
				// Generate_handler_1_enemy(short Nr,char Model,char Face,unsigned char
				// D0D1);
				if ((Enemies[Curr_enemy].Respawn > 0) &&
				    (Enemies[Curr_enemy].Respawn <=
				     handler1_users_high)) {
					Generate_handler_1_enemy(
						Curr_enemy,
						Enemies[Curr_enemy].Respawn, -1,
						Enemies[Curr_enemy].Respawn2);
					/*if(Enemies[Curr_enemy].Respawn==-bomb)
                  Spawn_bomb(&Enemies[Curr_enemy]);*/
				} else {
					/*if(Enemies[Curr_enemy].Respawn==bomb)
                  Spawn_bomb2(&Enemies[Curr_enemy]);*/
				}

			} // End of Respawn
		}
	}
}

// enemy handlers. Handles the behavious of the enemies. see enemies.txt for
// details
void Enemy_handler_1(struct enemy *Enemy)
{ // walk in one dir,if not free walk to the
	// opposite dir. fall down, no jump
	// mode == true: change sprite when change direction
	// data0 == true: limited to walk only Data0 pixels before change dir
	// initialization: Data1 = Data0
	// active == 2: Don't break xlimit even if free down
	// active == 3: Turn when an edge is reached

	// !!!!!!!!! This can be optimized: Use a variable that is 15 right.
	// !!!!!!!!! The rest of the code is almost equal.

	if (Enemy->Active == 3) { //
		// Solid_down
		if (Enemy->Face > 0) {
			// if(Free_down(Enemy->X+15,Enemy->Y+Enemy->Height)<16){
			// if(Free_down(Enemy->X+16,Enemy->Y+Enemy->Height)){
			if (Get_tile(Enemy->X + 15,
				     Enemy->Y + Enemy->Height + 15) <
			    solid_down_low) {
				Enemy->Face = -Enemy->Face;
				if (Enemy->Mode)
					Enemy->Sprite -= Enemy->Height2 * 3;
			}
		} else {
			if (Enemy->Face < 0) {
				// if(Free_down(Enemy->X-15,Enemy->Y+Enemy->Height)<16){
				// if(Free_down(Enemy->X-16,Enemy->Y+Enemy->Height)){
				if (Get_tile(Enemy->X,
					     Enemy->Y + Enemy->Height + 15) <
				    solid_down_low) {
					Enemy->Face = -Enemy->Face;
					if (Enemy->Mode)
						Enemy->Sprite +=
							Enemy->Height2 * 3;
				}
			}
		}
	}

	char temp = Enemy_move_x(Enemy);

	if (Enemy->Mode && temp) {
		Enemy->Sprite += temp * Enemy->Height2 * 3;
	}

	if (Enemy->Data0) {
		if (Enemy->Data1) {
			// Enemy->Data1--;
			Enemy->Data1 -= abs(Enemy->Face);

		} else { // Data1 == 0, change dir
			Enemy->Data1 = Enemy->Data0;
			Enemy->Face = -Enemy->Face;
			if (Enemy->Mode) {
				if (Enemy->Face > 0) {
					// Enemy->Mask += Enemy->Height;
					// Enemy->Sprite += Enemy->Height2*2;
					Enemy->Sprite += Enemy->Height2 * 3;
				} else {
					// Enemy->Mask -= Enemy->Height;
					// Enemy->Sprite -= Enemy->Height*2;
					Enemy->Sprite -= Enemy->Height2 * 3;
				}
			}
		}
		if (Free_down(Enemy->X,
			      Enemy->Y +
				      Enemy->Height) /*Enemy_free_down(Enemy)*/
		    && (Enemy->Active != 2)) {
			Enemy->Data0 = 0;
		}
	}
}

void Enemy_handler_2(struct enemy *Enemy)
{ // flower. rises from pipe (or
	// similar). stays for n frames,
	// goes down, stays for n frames

	uint8_t Mode = Enemy->Mode;
	// test if player is positioned on pipe above
	// if( !(((Player.Y+Player.Height)==Enemy->Y) && ((Player.X+24)>(Enemy->X)) &&
	// ((Player.X)<(Enemy->X+24))) ){
	if (!(((Player.X + 25) > (Enemy->X)) &&
	      ((Player.X) < (Enemy->X + 25)) && (/*Enemy->*/ Mode == 60))) {
		/*Enemy->*/ Mode++;

		// Enemy->Mode ++;
		if (/*Enemy->*/ Mode >= 61) {
			if ((/*Enemy->*/ Mode <= 93) &&
			    (Enemy->Height < 32)) { // rising
				Enemy->Height++;
				if (Enemy->Life > 0) // if not upside down
					Enemy->Y--;
			} else { // Enemy->Mode >=94
				if ((/*Enemy->*/ Mode >= 155)) { // sinking
					Enemy->Height--;
					if (Enemy->Life >
					    0) // if not upside down
						Enemy->Y++;
					if (/*Enemy->*/ Mode == 187)
						/*Enemy->*/ Mode = 0;
				}
			}
		}
	}

	Enemy->Mode = Mode;

	if ((++Enemy->Face) == 4) { // animate
		// Enemy->Mask += Enemy->Height2;
		// Enemy->Sprite += 2*Enemy->Height2;
		Enemy->Sprite += 3 * Enemy->Height2;
	} else {
		if (Enemy->Face == 8) {
			// Enemy->Mask -= Enemy->Height2;
			// Enemy->Sprite -= 2*Enemy->Height2;
			Enemy->Sprite -= 3 * Enemy->Height2;
			Enemy->Face = 0;
		}
	}
	// Enemy->Sprite = flower1_sprite+31-Enemy->Height;
	// Enemy->Mask = flower1_mask+15-Enemy->Height/2;
}

void Enemy_handler_3(struct enemy *Enemy)
{ // flower. rises from pipe. stays
	// for n frames, goes down,
	// stays for n frames. fires upon player
	// maa gjoere noe sprell her...

	uint8_t Mode = Enemy->Mode;
	uint16_t *Sprite;

	if (!(((Player.X + 25) > (Enemy->X)) &&
	      ((Player.X) < (Enemy->X + 25)) && (/*Enemy->*/ Mode == 60))) {
		/*Enemy->*/ Mode++;

		if (/*Enemy->*/ Mode >= 61) {
			if ((/*Enemy->*/ Mode <= 93) &&
			    (Enemy->Height < 32)) { // rising

				Enemy->Height++;
				if (Enemy->Life > 0) // if not upside down
					Enemy->Y--;

			} else { // Enemy->Mode >=94
				if ((/*Enemy->*/ Mode >= 155)) { // sinking
					Enemy->Height--;
					if (Enemy->Life >
					    0) // if not upside down
						Enemy->Y++;
					if (/*Enemy->*/ Mode == 187)
						/*Enemy->*/ Mode = 0;
				}
			}
		}
	}
	// bug
	// bug
	if (Player.X >= Enemy->X) { // right
		if (Player.Y <= Enemy->Y) { // up
			/*Enemy->*/ Sprite =
				(Enemy->Life > 0 ?
					 flower2_sprite_right_up :
					 flower2_sprite_right_down); // flower2_sprite_right_up;
			// Enemy->Mask = flower2_mask_right_up;
			if (/*Enemy->*/ Mode == 124)
				Enemy_add_fireball(
					Enemy->X + 12,
					Enemy->Y + (Enemy->Life > 0 ? 2 : 29),
					enemy_fireball_speed,
					-enemy_fireball_speed); // 2,29

		} else { // down
			//			Enemy->Face = 1;
			/*Enemy->*/ Sprite =
				(Enemy->Life > 0 ?
					 flower2_sprite_right_down :
					 flower2_sprite_right_up); // flower2_sprite_right_down;
			// Enemy->Mask = flower2_mask_right_down;
			if (/*Enemy->*/ Mode == 124)
				Enemy_add_fireball(
					Enemy->X + 12,
					Enemy->Y + (Enemy->Life > 0 ? 5 : 26),
					enemy_fireball_speed,
					enemy_fireball_speed); // 5,26
		}
	} else { // left
		if (Player.Y <= Enemy->Y) { // up
			//			Enemy->Face = -2;
			/*Enemy->*/ Sprite =
				(Enemy->Life > 0 ?
					 flower2_sprite_left_up :
					 flower2_sprite_left_down); // flower2_sprite_left_up;
			// Enemy->Mask = flower2_mask_left_up;
			if (/*Enemy->*/ Mode == 124)
				Enemy_add_fireball(
					Enemy->X - 4,
					Enemy->Y + (Enemy->Life > 0 ? 2 : 29),
					-enemy_fireball_speed,
					-enemy_fireball_speed); // 2,29

		} else { // down
			//			Enemy->Face = -1;
			/*Enemy->*/ Sprite =
				(Enemy->Life > 0 ?
					 flower2_sprite_left_down :
					 flower2_sprite_left_up); // flower2_sprite_left_down;
			// Enemy->Mask = flower2_mask_left_down;
			if (/*Enemy->*/ Mode == 124)
				Enemy_add_fireball(
					Enemy->X - 4,
					Enemy->Y + (Enemy->Life > 0 ? 5 : 26),
					-enemy_fireball_speed,
					enemy_fireball_speed); // 5,26
		}
	}

	Enemy->Sprite = Sprite;
	Enemy->Mode = Mode;
}

void Enemy_handler_4(struct enemy *Enemy)
{ // flying monster 1

	char Jumping = Enemy->Jumping;
	Enemy_move_x(Enemy);

	int16_t X = Enemy->X;
	int16_t Y = Enemy->Y;
	int16_t Height = Enemy->Height;

	switch (Enemy->Mode) { // switch instead of if's saved some bytes, it's
		// probably faster too...

	case 41:
	case 53:
		//	if( (Enemy->Mode == 41) || (Enemy->Mode == 53) ){
		if (!(Free_down(/*Enemy->*/ X,
				/*Enemy->*/ Y + /*Enemy->*/ Height)) &&
		    Free_up(/*Enemy->*/ X, /*Enemy->*/
			    Y) /*!(Enemy_free_down(Enemy)) && Enemy_free_up(Enemy)*/) {
			/*Enemy->*/ Jumping = 4; // initialize short flying up
		} else {
			Enemy->Mode--; // hold the state
		}

		break; //};

	case 45:
	case 57:
		//	if( (Enemy->Mode == 45) || (Enemy->Mode == 57) ){
		if (Free_down(/*Enemy->*/ X,
			      /*Enemy->*/ Y +
				      /*Enemy->*/
				      Height) /*Enemy_free_down(Enemy)*/) {
			/*Enemy->*/ Jumping =
				-4; // initialize short flying down
		} else {
			Enemy->Mode--; // hold the state
		}

		break; //};

	case 65:
		//	if( (Enemy->Mode == 65) ){
		if (!(Free_down(/*Enemy->*/ X,
				/*Enemy->*/ Y + /*Enemy->*/ Height)) &&
		    Free_up(/*Enemy->*/ X, /*Enemy->*/
			    Y) /*!(Enemy_free_down(Enemy)) && Enemy_free_up(Enemy)*/) {
			/*Enemy->*/ Jumping =
				32; // initialize long flying up 20
		} else {
			Enemy->Mode--; // hold the state
		}

		break; //};

	case 97:
		//	if( (Enemy->Mode == 97) ){//85
		if (Free_down(/*Enemy->*/ X,
			      /*Enemy->*/ Y +
				      /*Enemy->*/
				      Height) /*Enemy_free_down(Enemy)*/) {
			/*Enemy->*/ Jumping =
				-32; // initialize long flying down -20
		} else {
			Enemy->Mode--; // hold the state
		}

		break; //};

	} // end switch

	// keep Enemy->Jumping as a local copy???

	if (/*Enemy->*/ Jumping > 0) { // fly up
		if (Free_up(/*Enemy->*/ X,
			    /*Enemy->*/ Y) /*Enemy_free_up(Enemy)*/) {
			/*Enemy->*/ Y -= 1;
		} else {
			/*Enemy->*/ Jumping = 0;
		}
		/*Enemy->*/ Jumping--;
	}

	if (/*Enemy->*/ Jumping < 0) {
		if (Free_down(
			    /*Enemy->*/ X,
			    /*Enemy->*/ Y +
				    /*Enemy->*/
				    Height) /*Enemy_free_down(Enemy)*/) { // fly down
			/*Enemy->*/ Y += 1;
		} else {
			/*Enemy->*/ Jumping = 0;
		}
		/*Enemy->*/ Jumping++;
	}

	if (/*Enemy->*/ Jumping) {
		if (Fg_plane.frame == 0) {
			if (Enemy->Sprite == monster1_wings1_sprite) {
				Enemy->Sprite = monster1_wings2_sprite;
				// Enemy->Mask = monster1_wings2_mask;
				/*Enemy->Height =*//*Enemy->*/ Height /*2*/ =
					19;
				/*Enemy->*/ Y += 3;
			} else {
				Enemy->Sprite = monster1_wings1_sprite;
				// Enemy->Mask = monster1_wings1_mask;
				/*Enemy->Height =*//*Enemy->*/ Height /*2*/ =
					22;
				/*Enemy->*/ Y -= 3;
			}

			/*Enemy->Height = Enemy->Height2;*/
		}
	}

	Enemy->Mode++;
	if (Enemy->Mode >= 129) // 105
		Enemy->Mode = 0;

	Enemy->Jumping = Jumping;
	Enemy->Y = Y;
	Enemy->Height = Enemy->Height2 = Height;
};

void Enemy_handler_5(struct enemy *Enemy)
{
	Enemy->Mode--;

	if (Enemy->Mode == 0) {
		Enemy->Life = 0;
	}
}

void Enemy_handler_6(struct enemy *Enemy)
{ // turtle/beetle shell

	uint8_t Mode = Enemy->Mode;
	/*Enemy->*/ Mode--;

	if (/*Enemy->*/ Mode <= 12) { // shaking before resurection
		if (/*Enemy->*/ Mode == 12) {
			Enemy->X -= 1;
		} else {
			if (/*Enemy->*/ Mode % 2) {
				Enemy->X += 2;
			} else {
				Enemy->X -= 2;
			}
		}
		char Life = Enemy->Life;
		if (/*Enemy->*/ Mode == 0) { // resurection
			if (Enemy->X >
			    Player.X) { // allways walk against player after
				// resurection
				Enemy->Sprite = (/*Enemy->*/ Life == 1 ?
							 turtle_right_sprite :
							 beetle_left_sprite);
				Enemy->Face = -1;
			} else {
				Enemy->Sprite = (/*Enemy->*/ Life == 1 ?
							 turtle_left_sprite :
							 beetle_right_sprite);
				Enemy->Face = 1;
			}
			Enemy->Handler = Enemy_handler_1;
			// New: V 1.01 Fixed bug where resurected turtle could't be killed by
			// racoon tale: Changed 0b11111000:0b10111000 to 0b11111001:0b10111000
			Enemy->Attribs = (/*Enemy->*/ Life == 1 ? 0b11111001 :
								  0b10111000);
			Enemy->Die = Enemy_die_2;
			/*Enemy->*/ Mode = 1;

			if (Life == 1) {
				Enemy->Height = Enemy->Height2 = 26;
				Enemy->Y -= 10;
			}
		}
	}
	Enemy->Mode = Mode;
}

void Enemy_handler_7(struct enemy *Enemy)
{ // crushed turtle skeleton

	uint8_t Mode = Enemy->Mode;
	/*Enemy->*/ Mode--;

	if (/*Enemy->*/ Mode <= 12) { // shaking before resurection
		if (/*Enemy->*/ Mode == 12) {
			Enemy->X -= 1;
		} else {
			if (/*Enemy->*/ Mode % 2) {
				Enemy->X += 2;
			} else {
				Enemy->X -= 2;
			}
		}
		if (/*Enemy->*/ Mode == 0) { // resurection
			if (Enemy->Face > 0) {
				Enemy->Sprite = turtle_skeleton_right_sprite;
				// Enemy->Mask = turtle_skeleton_right_mask;
			} else {
				Enemy->Sprite = turtle_skeleton_left_sprite;
				// Enemy->Mask = turtle_skeleton_left_mask;
			}
			Enemy->Handler = Enemy_handler_1;
			Enemy->Attribs = 0b10110000;
			Enemy->Die = Enemy_die_7;
			/*Enemy->*/ Mode = 1;
			Enemy->Height = Enemy->Height2 = 27;
			Enemy->Y -= 15;
		}
	}
	Enemy->Mode = Mode;
};

void Enemy_handler_8(struct enemy *Enemy)
{ // flying turtle
	// Mode config:	b7: x motion enable
	//							b6: y motion enable
	//							b5: x motion limit
	// enable 							b4: y motion
	// limit enable 							b3:
	// allways go as far down as possible

	uint8_t Mode = Enemy->Mode;

	if ((/*Enemy->*/ Mode & 0b10000000)) { // x motion

		Enemy_move_x(Enemy);
	}

	int16_t Jumping = Enemy->Jumping;

	if ((/*Enemy->*/ Mode & 0b01000000)) { // y motion

		if (/*Enemy->*/ Jumping > 0) { // down
			if (Free_down(
				    Enemy->X,
				    Enemy->Y +
					    Enemy->Height) /*Enemy_free_down(Enemy)*/) { // free down?
				Enemy->Y += /*Enemy->*/ Jumping;
			} else {
				/*Enemy->*/ Jumping =
					-/*Enemy->*/ Jumping; // change direction
			}
		}

		if (/*Enemy->*/ Jumping < 0) { // left
			if (Free_up(Enemy->X,
				    Enemy->Y) /*Enemy_free_up(Enemy)*/) { // free up?
				Enemy->Y += /*Enemy->*/ Jumping;
			} else {
				/*Enemy->*/ Jumping =
					-/*Enemy->*/ Jumping; // change direction
			}
		}
	}

	if ((/*Enemy->*/ Mode &
	     0b00001000)) { // dont turn before bottom reached
		if ((Enemy->Data0 == 0) && (/*Enemy->*/ Jumping > 0)) {
			if (Free_down(
				    Enemy->X,
				    Enemy->Y +
					    Enemy->Height) /*Enemy_free_down(Enemy)*/) {
				Enemy->Data0 = 1;
			}
		}
	}

	if ((/*Enemy->*/ Mode & 0b00100000)) { // x motion limit
		if (Enemy->Data0 == 0) {
			Enemy->Data0 = Enemy->Data1;
			Enemy->Face = -Enemy->Face;
			if ((/*Enemy->*/ Mode & 0b00010000)) // y motion limit
				/*Enemy->*/ Jumping = -/*Enemy->*/ Jumping;
		} else {
			Enemy->Data0--;
		}
	} else {
		if ((/*Enemy->*/ Mode & 0b00010000)) { // y motion limit
			if (Enemy->Data0 == 0) {
				Enemy->Data0 = Enemy->Data1;
				/*Enemy->*/ Jumping = -/*Enemy->*/ Jumping;
			} else {
				Enemy->Data0--;
			}
		}
	}
	Enemy->Jumping = Jumping;

	if (Fg_plane.frame == 0) {
		uint16_t *Sprite;
		if ((Enemy->Sprite == turtle_fly1_left_sprite) ||
		    (Enemy->Sprite == turtle_fly1_right_sprite)) {
			/*Enemy->*/ Sprite = turtle_fly2_left_sprite;
			// Enemy->Mask = turtle_fly2_left_mask;
		} else {
			/*Enemy->*/ Sprite = turtle_fly1_left_sprite;
			// Enemy->Mask = turtle_fly1_left_mask;
		}
		if (Enemy->Face > 0) {
			/*Enemy->*/ Sprite += 27 * 3 * 2;
			// Enemy->Mask += 27*2;
		}
		Enemy->Sprite = Sprite;
	}
};

static void Move_and_animate(struct enemy *Enemy)
{ // common func for all enemies using the
	// boomerang guy sprites. saved some bytes...
	Enemy_move_x(Enemy);

	if (Enemy->X > Player.X) {
		Enemy->Sprite = boomerang_guy_left_sprite;
	} else {
		Enemy->Sprite = boomerang_guy_right_sprite;
	}
}

void Enemy_handler_9(struct enemy *Enemy)
{ // boomerang guy

	//	Enemy_move_x(Enemy);
	Move_and_animate(Enemy);

	if (Enemy->Jumping) {
		if (Free_up(Enemy->X, Enemy->Y)) {
			Enemy->Y += Enemy->Jumping;
		} else {
			Enemy->Jumping = 0;
		}
	}

	switch (Enemy->Mode) { // switch instead of if's saved some bytes...
	case 40:
		//	if(Enemy->Mode == 40){
		Enemy->Face = -1;
		break; //};
	case 60:
		//	if(Enemy->Mode == 60){
		Enemy->Face = 1;
		break; //};
	case 100:
		//	if(Enemy->Mode == 100){
		Enemy->Face = -1;
		break; //};
	case 120:
		//	if(Enemy->Mode == 120){
		Enemy->Jumping = -2;
		Enemy->Face = 0;
		break; //};
	case 130:
		//	if(Enemy->Mode == 130){
		Enemy->Jumping = 0;
		Enemy_add_boomerang(Enemy->X, Enemy->Y,
				    (Enemy->X > Player.X ? -2 : 2));
		break; //};

	case 160:
		//	if(Enemy->Mode == 160 ){
		Enemy->Mode = -1;
		break; //}
		/*else{
            Enemy->Mode++;
    };*/
	} // end switch

	Enemy->Mode++;
};

void Enemy_handler_10(struct enemy *Enemy)
{ // jumping fireball
	// Data0: Jumptime (nr of frames)
	// Face: Jumpspeed			(jumpheight = Data0 *  face)
	// Data1: Downtime

	uint8_t Mode = Enemy->Mode;
	/*Enemy->*/ Mode++;
	char Jumping = Enemy->Jumping;

	if (/*Enemy->*/ Jumping) { // jumping or falling
		Enemy->Y += /*Enemy->*/ Jumping;
		if ((/*Enemy->*/ Mode) == Enemy->Data0) {
			/*Enemy->*/ Jumping =
				-/*Enemy->*/ Jumping; // change dir
			Enemy->Life = -Enemy->Life;
		} else {
			if (/*Enemy->*/ Mode >= (2 * Enemy->Data0)) {
				/*Enemy->*/ Jumping = 0;
				/*Enemy->*/ Mode = 0;
			}
		}
	} else { // waiting

		if ((/*Enemy->*/ Mode) >= Enemy->Data1) {
			/*Enemy->*/ Jumping = Enemy->Face;
			/*Enemy->*/ Mode = 0;
			Enemy->Life = (/*Enemy->*/ Jumping > 0 ? 1 : -1);
			Enemy->Height = 18;
		} else {
			Enemy->Height = 1;
		}
	}

	Enemy->Mode = Mode;
	Enemy->Jumping = Jumping;

	if ((Fg_plane.step % 2)) {
		Enemy->Sprite = lavaball_step_2_sprite;
	} else {
		Enemy->Sprite = lavaball_step_1_sprite;
	}
};

void Enemy_handler_11(struct enemy *Enemy)
{ // circulating fireball
	// radius = 32
	// static const char
	// Motion[30]={3,4,3,3,3,3,2,3,2,2,1,1,1,1,0,0,-1,-1,-1,-1,-2,-2,-3,-2,-3,-3,-3,-3,-4,-3};
	// radius = 40
	static const char Motion[30] = {
		4, 4,  4,  4,  4,  4,  3,  3,  2,  3,  2,  1,  1,  1,  0,
		0, -1, -1, -1, -2, -3, -2, -3, -3, -4, -4, -4, -4, -4, -4
	};
	// Initialization:
	// Data0 = 15
	// Data1 = 30
	// Face = -1
	// Jumping = -1

	uint8_t Data0 = Enemy->Data0;

	if (/*Enemy->*/ Data0 == 0)
		Enemy->Face = 1;
	if (/*Enemy->*/ Data0 == 29)
		Enemy->Face = -1;
	if (Enemy->Data1 == 0)
		Enemy->Jumping = 1;
	if (Enemy->Data1 == 29)
		Enemy->Jumping = -1;

	Enemy->X += Motion[/*Enemy->*/ Data0];
	Enemy->Y += Motion[Enemy->Data1];

	/*Enemy->*/ Data0 += Enemy->Face;
	Enemy->Data0 = Data0; // doing this in two steps saves a few bytes
	Enemy->Data1 += Enemy->Jumping;
};

void Enemy_handler_12(struct enemy *Enemy)
{ // falling brick

	//	short X = Enemy->X;
	char Jumping = Enemy->Jumping;
	uint8_t Mode = Enemy->Mode;

	if ((/*Enemy->*/ Mode) && ((Player.X + 32) > Enemy->X) &&
	    (Player.X < (Enemy->X + 32))) {
		/*Enemy->*/ Jumping = 1;
		/*Enemy->*/ Mode = 0;
	}
	int16_t Y = Enemy->Y;

	uint8_t Data0 = Enemy->Data0;

	if ((/*Enemy->*/ Jumping > 0)) {
		if (Free_down(Enemy->X, /*Enemy->*/ Y + Enemy->Height) <
		    2 /*(Enemy_free_down(Enemy)<2)*/) {
			/*Enemy->*/ Jumping = 0;
			/*Enemy->*/ Data0 = 24;
			/*Enemy->*/ Y += 2;
			Add_dustsky(Enemy->X - 8, /*Enemy->*/ Y + 10);
			Add_dustsky(Enemy->X + 8, /*Enemy->*/ Y + 10);
		}
		/*Enemy->*/ Y += 2;
	}

	if ((/*Enemy->*/ Jumping < 0)) {
		if (Free_up(Enemy->X, /*Enemy->*/ Y) <
		    2 /*(Enemy_free_up(Enemy)<2)*/) {
			/*Enemy->*/ Jumping = 0;
			/*Enemy->*/ Mode = 1;
		}
		/*Enemy->*/ Y -= 2;
	}
	Enemy->Mode = Mode;
	Enemy->Y = Y;
	if (/*Enemy->*/ Data0) {
		if (!(--/*Enemy->*/ Data0)) {
			/*Enemy->*/ Jumping = -1;
		}
	}
	Enemy->Data0 = Data0;
	Enemy->Jumping = Jumping;
};

void Enemy_handler_13(struct enemy *Enemy)
{ // handles the jumping brick

	int16_t Temp;
	int16_t Height = Enemy->Height;

	if ((++Enemy->Mode) == 130)
		Enemy->Mode = 0;

	if (Enemy->Mode == 50) {
		if (abs(Player.X - Enemy->X) <= 62) {
			/*Enemy->*/ Height = 21;
			Enemy->Y -= 5;
			Enemy->Attribs = 0b10010101;
		} else {
			Enemy->Mode--; // hold the current state (wait until player is close
			// enough to attack)
		}
	}

	int16_t Y = Enemy->Y;

	if (Enemy->Mode == 60) {
		/*Enemy->*/ Height = 16;
		/*Enemy->*/ Y += 5;
	}

	char Jumping = Enemy->Jumping;

	if (Enemy->Mode == 70) {
		/*Enemy->*/ Height = 21;
		/*Enemy->*/ Y -= 5;
		/*Enemy->*/ Jumping = 4;
		Enemy->Face = (Enemy->X > Player.X ? -2 : 2);
	}

	if (Enemy->Mode == 75) {
		/*Enemy->*/ Height = 16;
	}
	//	if(Height)
	Enemy->Height = Height;

	if ((Enemy->Mode == 77) || (Enemy->Mode == 83) || (Enemy->Mode == 88) ||
	    (Enemy->Mode == 91) || (Enemy->Mode == 94) || (Enemy->Mode == 99) ||
	    (Enemy->Mode == 105) || (Enemy->Mode == 112)) {
		/*Enemy->*/ Jumping--;
	}

	if (/*Enemy->*/ Jumping > 0) { // jumping
		if (Free_up(Enemy->X, /*Enemy->*/ Y) >= /*Enemy->*/ Jumping) {
			/*Enemy->*/ Y -= /*Enemy->*/ Jumping;
		} else {
			Enemy->Die(Enemy);
		}
	} else {
		if (/*Enemy->*/ Jumping < 0) { // falling

			Temp = Free_down(Enemy->X,
					 /*Enemy->*/ Y + Enemy->Height);

			if (Temp >= (-/*Enemy->*/ Jumping)) {
				/*Enemy->*/ Y -= /*Enemy->*/ Jumping;
			} else {
				/*Enemy->*/ Y += Temp;
				/*Enemy->*/ Jumping = 0;
			}
		}
	}

	Enemy->Y = Y;
	Enemy->Jumping = Jumping;

	if (/*Enemy->*/ Jumping) {
		Enemy_move_x(Enemy);

	} else {
		Enemy->Attribs = 0b10110101;
	}
};

void Enemy_handler_14(struct enemy *Enemy)
{ // handles horizontal cannon
	// Data0: time between the shots
	// Face: Direction, 0=bidirectional

	if ((++Enemy->Mode) == Enemy->Data0) {
		Enemy->Mode = 0;

		if ((Enemy->Face >= 0) && (Player.X > Enemy->X))
			Enemy_add_cannonball_horiz(Enemy->X, Enemy->Y, 2);
		if ((Enemy->Face <= 0) && (Player.X < Enemy->X))
			Enemy_add_cannonball_horiz(Enemy->X - 16, Enemy->Y, -2);
	}
};

void Enemy_handler_15(struct enemy *Enemy)
{ // handles diagonal cannons
	// Face: X speed & dir
	// Data1: Y speed & dir
	// Data0: Time between the shots

	if ((++Enemy->Mode) == Enemy->Data0) {
		Enemy->Mode = 0;

		Enemy_add_cannonball(Enemy->X, Enemy->Y, Enemy->Face,
				     Enemy->Data1);
	}
};

void Enemy_handler_16(struct enemy *Enemy)
{ // handles ghost
	// turn your back at them, and they'll start chasing you!

	// Life == 4:
	// No motion limit

	// when using a limit the ghost must be placed in the middle of it's area

	// Life b1 set:
	// Data1 == X Limit
	// Data0 used as counter/dist from origin

	// Life b0 set:
	// Data1 == Y Limit
	// Data0 used as counter/dist from origin

	// Life b0 and b1 set:
	// Data1 == X and Y Limit
	// Data0 used as counter/dist from origin

	// Enemy->Mode = 0;
	int16_t /*Enemy->*/ Jumping = 0;
	uint16_t *Sprite;
	uint8_t Data0 = Enemy->Data0;
	int16_t X = Enemy->X;

	if (Player.Face > 0) { // player facing right

		if (Player.X > /*Enemy->*/ X) {
			/*Enemy->*/ Sprite = ghost_chasing_right_spr;
			// Enemy->Mask = ghost_chasing_right_msk;
			if (Enemy->Life & 0b00000010) { // 0b00000001
				if (/*Enemy->*/ Data0 < Enemy->Data1) {
					/*Enemy->*/ X++; // chase x
					/*Enemy->*/ Data0++;
				}
			} else {
				/*Enemy->*/ X++; // chase x
			}
			// Enemy->X++;//chase x
			// Enemy->Mode = 1;//chase y
			/*Enemy->*/ Jumping = 1;
		} else {
			/*Enemy->*/ Sprite = ghost_waiting_left_spr;
			// Enemy->Mask = ghost_waiting_left_msk;
		}

	} else { // player facing left
		if (Player.X < /*Enemy->*/ X) {
			/*Enemy->*/ Sprite = ghost_chasing_left_spr;

			if (Enemy->Life & 0b00000010) { // 0b00000001
				if (/*Enemy->*/ Data0 > 0) {
					/*Enemy->*/ X--; // chase x
					/*Enemy->*/ Data0--;
				}
			} else {
				/*Enemy->*/ X--; // chase x
			}
			// Enemy->X--;//chase x
			// Enemy->Mode = 1;//chase y
			/*Enemy->*/ Jumping = 1;
		} else {
			/*Enemy->*/ Sprite = ghost_waiting_right_spr;
		}
	}

	Enemy->X = X;
	Enemy->Data0 = Data0;
	Enemy->Sprite = Sprite;

	uint8_t Mode = Enemy->Mode;
	int16_t Y = Enemy->Y;

	// if(Enemy->Mode){//chase y
	if (/*Enemy->*/ Jumping) { // chase y
		if (Player.Y > /*Enemy->*/ Y) {
			if (Enemy->Life & 0b00000001) { // 0b00000010
				if (/*Enemy->*/ Mode < Enemy->Data1) {
					/*Enemy->*/ Y++; // chase x
					/*Enemy->*/ Mode++;
				}
			} else {
				/*Enemy->*/ Y++; // chase x
			}
			// Enemy->Y++;
		}

		if (Player.Y < /*Enemy->*/ Y) {
			if (Enemy->Life & 0b00000001) { // 0b00000010
				if (/*Enemy->*/ Mode < Enemy->Data1) {
					/*Enemy->*/ Y--; // chase x
					/*Enemy->*/ Mode--;
				}
			} else {
				/*Enemy->*/ Y--; // chase x
			}
			// Enemy->Y--;
		}
	}

	Enemy->Mode = Mode;
	Enemy->Y = Y;
};

void Enemy_handler_17(struct enemy *Enemy)
{ // handles mad jumping/walking flower

	char Jumping = Enemy->Jumping;

	if ((Player.Y < Enemy->Y) && (abs(Enemy->X - Player.X) < 16) &&
	    (!Free_down(Enemy->X, Enemy->Y + Enemy->Height))) {
		/*Enemy->*/ Jumping = 32;
	}

	if (Enemy->Jumping) {
		/*Enemy->*/ Jumping -= 2;
		Enemy->Y -= 2;
	}

	if (Enemy->Mode) {
		if (Fg_plane.step % 2) {
			Enemy_move_x(Enemy);
		}

		if (!/*Enemy->*/ Jumping &&
		    (!Free_down(Enemy->X, Enemy->Y + Enemy->Height))) {
			/*Enemy->*/ Jumping = 6;
		}
	}
	char Face;
	uint16_t *Sprite;

	if (Player.X < Enemy->X) {
		/*Enemy->*/ Sprite = (Fg_plane.step % 2 ?
					      mad_flower_c_left_spr :
					      mad_flower_o_left_spr);
		/*Enemy->*/ Face = -1;
	} else {
		/*Enemy->*/ Sprite = (Fg_plane.step % 2 ?
					      mad_flower_c_right_spr :
					      mad_flower_o_right_spr);
		/*Enemy->*/ Face = 1;
	}

	Enemy->Face = Face;
	Enemy->Sprite = Sprite;
	Enemy->Jumping = Jumping;
};

void Enemy_handler_18(struct enemy *Enemy)
{ // fire monster

	//	Enemy_move_x(Enemy);
	Move_and_animate(Enemy);

	if (Enemy->Jumping) {
		if (Free_up(Enemy->X, Enemy->Y)) {
			Enemy->Y += Enemy->Jumping;
		} else {
			Enemy->Jumping = 0;
		}
	}

	if ((++Enemy->Mode) > 64)
		Enemy->Mode = 0;

	switch (Enemy->Mode) { // switch instead of if's saved some bytes

	case 0:
		//	if(Enemy->Mode==0){
		Enemy->Face = -1;
		break; //};
	case 16:
		//	if(Enemy->Mode==16){
		Enemy->Face = 1;
		break; //};
	case 32:
		//	if(Enemy->Mode==32){
		Enemy->Face = 0;
		Enemy->Jumping = -2;
		break; //};
	case 40:
		//	if(Enemy->Mode==40){
		Enemy->Jumping = 0;
		// fire
		Enemy_add_bounching_fireball(Enemy->X, Enemy->Y,
					     (Player.X < Enemy->X ? -1 : 1), 1);
		break; //};
	case 50:
		//	if(Enemy->Mode==50){
		// fire
		Enemy_add_bounching_fireball(Enemy->X, Enemy->Y,
					     (Player.X < Enemy->X ? -1 : 1), 1);
		break; //};
	} // end switch
};

void Enemy_handler_19(struct enemy *Enemy)
{ // flying fish
	// Data0: XLimit. Fish limited to move x pixels in x dir before turn. Handled
	// by Enemy_handler_1() Data1=Data0 when initialized Face: dir and xspeed
	// Life: Jumpspeed
	// Mode: Jumping startpoint. Start jump after 16 pixels: Mode = Data0-16

	Enemy->X += Enemy->Face;
	Enemy->Data1 -= abs(Enemy->Face);

	if (Enemy->Data1 == 0) {
		Enemy->Face = -Enemy->Face;
		Enemy->Data1 = Enemy->Data0;
		if (Enemy->Face > 0) {
			Enemy->Sprite += Enemy->Height2 * 3;
		} else {
			Enemy->Sprite -= Enemy->Height2 * 3;
		}
	}

	char Jumping = Enemy->Jumping;

	if (Enemy->Data1 == Enemy->Mode) {
		/*Enemy->*/ Jumping = -Enemy->Life;
	} else {
		if (Enemy->Data1 == (Enemy->Data0 - Enemy->Mode)) {
			/*Enemy->*/ Jumping = 0;
		} else {
			if (/*Enemy->Data1==(Enemy->Data0/2)*/ Enemy->Data1 ==
			    (Enemy->Data0 >> 1)) {
				/*Enemy->*/ Jumping = -/*Enemy->*/ Jumping;
			}
		}
	}

	Enemy->Y += /*Enemy->*/ Jumping;
	Enemy->Jumping = Jumping;
};

void Enemy_handler_20(struct enemy *Enemy)
{ // jellyfish

	if (!Enemy->Data0 && (Enemy->Y >= (Player.Y + 8))) {
		// initialize swim
		Enemy->Jumping = 32;
		Enemy->Data0 = 8;
	} else {
		if (Free_down(Enemy->X, Enemy->Y + Enemy->Height)) {
			if (!(Fg_plane.frame % 2)) // Fg_plane.frame==0)//
				Enemy->Y++;
		}
	}

	if (Enemy->Jumping) {
		if (Get_tile(Enemy->X, Enemy->Y) == water) {
			if (Free_up(Enemy->X, Enemy->Y + Enemy->Height))
				Enemy->Y -= 2;
			Enemy->Jumping -= 2;
		} else {
			Enemy->Jumping = 0; // terminate swim to avoid flying!!!
		}

		if (Enemy->X >= Player.X) {
			if (Free_left(Enemy->X, &Enemy->Y, Enemy->Height, 0))
				Enemy->X--;
		} else {
			if (Free_right(Enemy->X, &Enemy->Y, Enemy->Height, 0))
				Enemy->X++;
		}
	} else {
		if (Enemy->Data0) {
			Enemy->Data0--;
		}
	}
};

void Enemy_handler_21(struct enemy *Enemy)
{ // bomb. WARNING: This is a really messy function!
	// Life:
	// 1: walking until jumpkilled, then initializing countdown
	// 2: walking until jumpkilled, then initializing countdown (more might come):
	// Initialize countdown when player in range, not implemented 3: walking until
	// jumpkilled, then initializing countdown (more might come)

	// 4: falling, initializing countdown when hit ground
	// 8: Shot out from cannon
	// 9: Passive, used when in countdown

	// static const char
	// dx[20]={-8,16,-8,0,-12,24,-24,24,-12,0,-16,32,-32,32,-32,32,-24,16,-16,16};
	// static const char
	// dy[20]={0,0,-8,16,-24,24,0,-24,-8,32,16,0,-16,32,0,-32,8,16,0,-16};

	// static const char dx[20]={ 16,-16, 16,-24,   32,-32, 32,-32,   32,-16,
	// 0,-12,   24,-24, 24,-12,    0, -8, 16, -8}; static const char dy[20]={-16,
	// 0, 16,  8,  -32,  0, 32,-16,    0, 16, 32,-8,  -24,  0, 24,-24,   16, -8,
	// 0,  0};

	static const char dx[20] = { 48,  -24, 0,  20,	-40, 0,	 40,
				     -20, -16, 32, -16, -12, 24, -24,
				     24,  -12, 0,  -8,	16,  -8 };
	static const char dy[20] = { 0,	 -24, 48, -44, 40, -40, 40, -36, 16, 0,
				     16, -28, 0,  24,  0,  -20, 16, -8,	 0,  0 };
	//	short Collision = 0;//shot

	if (Enemy->Life < 4) {
		Enemy_handler_1(Enemy);
	} else {
		// if(Enemy->Life==4){
		if ((Enemy->Mode == 1) &&
		    !Free_down(Enemy->X, Enemy->Y + Enemy->Height)) {
			Enemy->Mode =
				120; // hit the ground, initialize countdown
			Enemy->Attribs = 0b10101011;
			if (Enemy->Life != 8) { // shot
				Enemy->Life = 9;
				// Enemy->Attribs = 0b10011001;
			}

			Enemy->Height = 14;
		}
		//};
		if ((Enemy->Life == 4) && (abs(Enemy->X - Player.X) < 16 * 3)) {
			Enemy->Attribs = 0b10111001;
		}

		if ((Enemy->Life == 9) ||
		    (Enemy->Life == 8)) { // Life == 8: shot
			if (Enemy->Jumping) {
				// short Dist;// = Free_down(Enemy->X,Enemy->Y+Enemy->Height);

				if (Enemy->Jumping < 0) { // up
					// Dist = Free_up(Enemy->X,Enemy->Y);

					if (Free_up(Enemy->X, Enemy->Y) >= 2) {
						Enemy->Y -= 2;
					} else {
						Enemy->Jumping = 2;
						// Collision = 1;//shot
					}

					if ((--Enemy->Data0) == 0) {
						Enemy->Jumping = 2;
					}

				} else { // down
					// Dist = Free_down(Enemy->X,Enemy->Y+Enemy->Height);

					if (Free_down(Enemy->X,
						      Enemy->Y +
							      Enemy->Height) >=
					    2) {
						Enemy->Y += 2;
					} else {
						Enemy->Jumping = 0;
						// Collision = 1;//shot
					}
				}

				char Temp = Enemy->Face;

				Enemy_move_x(Enemy);

				if (Temp != Enemy->Face) { // hit something
					Enemy->Face = 0;
					Enemy->Jumping = 0;
					// Collision = 1;//shot
				}

				/*if(Dist>0){//free down?
                Enemy->Y += (Dist>Enemy->Jumping?Enemy->Jumping:Dist);
                Enemy_move_x(Enemy);
                if( (Enemy->Jumping>0) && (--Enemy->Data0)==0 ){
                        Enemy->Jumping = 2;
                }
        }
        else{//not free down
                Enemy->Jumping = 0;//stop falling

        };*/
			}
		}

		//		if(Collision)
		//			Enemy->Active = 2;

		if (Enemy->Life == 8 /*&& (Collision == 0)*/) { // shot
			Enemy->Data1++;

			//			if( (Enemy->Jumping==0) &&
			//(Enemy->Active==1)/*(Enemy->Mode<=2)*/ )//&& (Enemy->Mode==1)  &&
			//(Enemy->Attribs!=0b10101011) Enemy_move_x(Enemy);

			if (Free_down(Enemy->X, Enemy->Y + Enemy->Height) < 2) {
				Enemy->Attribs = 0b10101011;
			}

			switch (Enemy->Data1) {
			case 100:
				Enemy->Height = 14; // Enemy->Height2 = ;
				Enemy->Jumping = -2;
				Enemy->Face = Enemy->Respawn2;
				Enemy->Data0 = 16;
				break;
				/*			case 116:
                                        Enemy->Jumping = 0;
                                break;*/
			case 116:
				Enemy->Attribs = 0b10111001;
				//					Enemy->Jumping = 2;

				break;
			}

		} // end of shot
	}

	if (Enemy->Mode > 2) {
		Enemy->Mode--;
		if ((Enemy->Mode < 50)) {
			if (Enemy->Mode > 23) {
				if (Fg_plane.step % 2) {
					Enemy->Sprite =
						(Enemy->Face > 0 ?
							 bomb_deto_right_spr :
							 bomb_deto_left_spr);
					Enemy->Height2 = 14;
				} else {
					Enemy->Sprite =
						(Enemy->Face > 0 ?
							 bomb_walk_right_spr :
							 bomb_walk_left_spr);
					Enemy->Height2 = 16;
				} // Fg_plane.step%2

			} //>23
			else { // 2<Mode<=23

				if (Enemy->Mode == 23) {
					Enemy->Sprite = bomb_star_spr;
					// Enemy->Mask = bomb_star_msk;
					Enemy->Height2 = Enemy->Height = 8;
					Enemy->Attribs =
						0b00010100; // 0b00011100;//
					Enemy->Die = (void *)Dummy_func_;
					//					Enemy->Jumping = 0;
				} else {
					Enemy->X += dx[Enemy->Mode - 3];
					Enemy->Y += dy[Enemy->Mode - 3];
				}
			}
		} //<50

	} else { // mode<=2
		// moved out of if block, see below. Fixed a bug where some bombs
		// didn't respawn
		/*if(Enemy->Mode==2){
            if(Enemy->Life==8){//restart shot
                    Spawn_bomb(Enemy);
            }
            else{//end of shot
                    if(Enemy->Respawn2==4){
                            Spawn_bomb2(Enemy);
                    }
                    else{
                            Enemy->Life = 0;
                    }

            }
    };		*/
	}

	if (Enemy->Mode == 2) {
		if (Enemy->Life == 8) { // restart shot
			Spawn_bomb(Enemy);
		} else { // end of shot
			if (Enemy->Respawn2 == 4) {
				Spawn_bomb2(Enemy);
			} else {
				Enemy->Life = 0;
			}
		}
	}
};

void Spawn_bomb_common(struct enemy *Enemy)
{
	Enemy->X = Enemy->RespawnX * 16;
	Enemy->Y = Enemy->RespawnY * 16; //-2;
	Enemy->Handler = Enemy_handler_21;
	Enemy->Die = Enemy_die_11;
	//	Enemy->Height = Enemy->Height2 = 16;
	Enemy->Sprite = bomb_walk_left_spr;
	Enemy->Mode = 1;
	Enemy->Active = 1;
	Enemy->Respawn = -bomb;
}

void Spawn_bomb(struct enemy *Enemy)
{ //(Re)spawns bomb that is to be shot out from a
	// diagonal cannon

	Spawn_bomb_common(Enemy);
	//	Enemy->X = Enemy->RespawnX*16;
	//	Enemy->Y = Enemy->RespawnY*16;//-2;
	Enemy->Attribs = 0b10011001;
	Enemy->Data1 = 0;
	//	Enemy->Sprite = bomb_walk_left_spr;
	//	Enemy->Height = Enemy->Height2 = 16;
	//	Enemy->Mode = 1;
	Enemy->Face = 0;
	Enemy->Height = 1;
	Enemy->Height2 = 16;
	Enemy->Life = 8;
	Enemy->Jumping = 0;
	//	Enemy->Active = 1;
	//	(struct enemy *)Enemy->Handler = Enemy_handler_21;
	//	(struct enemy *)Enemy->Die = Enemy_die_11;
}

void Spawn_bomb2(struct enemy *Enemy)
{ //(Re)spawns bomb

	Spawn_bomb_common(Enemy);
	Enemy->Height = Enemy->Height2 = 16;
	//	Enemy->Sprite = bomb_walk_left_spr;
	Enemy->Attribs = 0b10111001;
	//	Enemy->X = Enemy->RespawnX*16;
	//	Enemy->Y = Enemy->RespawnY*16;
	//	Enemy->Mode = 1;
	//	(struct enemy *)Enemy->Handler = Enemy_handler_21;
	//	(struct enemy *)Enemy->Die = Enemy_die_11;
	Enemy->Life = 4;
	//	Enemy->Active = 1;
	Enemy->Respawn2 = 4;
	// 	Enemy->Respawn = -bomb;
}

void Enemy_handler_22(struct enemy *Enemy)
{ // Handles hammerman

	Move_and_animate(Enemy);

	if (Enemy->Data0 == 1) {
		Enemy->Data0 = 0;
		Enemy->Jumping = -2;
		Enemy->Face = 0;
		Enemy->Mode = 27;
		if (Enemy->Data1 == 0) {
			Enemy->Data1 = 1; // dest up
		} else {
			if (Get_tile(Enemy->X, Enemy->Y + Enemy->Height + 18) >=
			    solid_low) { // solid 2x below, cant go down
				Enemy->Data1 = 1; // dest up
			} else {
				Enemy->Data1 = 0; // dest down
			}
			//			Enemy->Data1 = 0;//dest down
		}
	}
	if (!Enemy->Jumping) {
		if ((++Enemy->Mode) == 16 * 4) {
			Enemy->Mode = 0;
			Enemy->Data0++;
		}

		switch (Enemy->Mode) {
		case 0:
			Enemy->Face = -1;
			break;
		case 16:
			Enemy->Face = 1;
			Enemy_add_hammer(Enemy->X, Enemy->Y - 4,
					 (Enemy->X > Player.X ? -2 : 2));
			break;
		case 48:
			Enemy->Face = -1;
			Enemy_add_hammer(Enemy->X, Enemy->Y - 4,
					 (Enemy->X > Player.X ? -2 : 2));
			break;
			/*case 64:
              Enemy->Face = 1;
              break;*/
		}
	} else { // Enemy->Jumping

		if (Enemy->Data1 == 0) {
			if (Enemy->Mode == (27 - 3)) {
				Enemy->Jumping = 2;
			}
		}

		Enemy->Y += Enemy->Jumping;

		if ((--Enemy->Mode) == 0) {
			Enemy->Jumping = 0;
			Enemy->Face = -1;
		}
	}
}

void Enemy_handler_23(struct enemy *Enemy)
{ // bottom flower

	if (!(Fg_plane.frame % 2)) { // slow down anim

		if (Enemy->Mode) { // left part, master
			// if((++Enemy->Data0)==130){
			if ((++Enemy->Data0) == 110) {
				Enemy->Data0 = 0;
			}

			int16_t H = 0;
			int16_t DY = 0;

			DY = 0;

			switch (Enemy->Data0) {
			case 0:
				/*Enemy->Y += 2;
        (Enemy+1)->Y += 2;*/
				DY = 2;
			case 20:
			case 40:
			case 60:
				// case 80:

				Enemy->Sprite = bottom_flower_l1;
				H = 28; // Enemy->Height = Enemy->Height2 = 28;
				DY += 2; // Enemy->Y += 2;
				(Enemy + 1)->Sprite = bottom_flower_r1;
				//(Enemy+1)->Height = (Enemy+1)->Height2 = 28;
				//(Enemy+1)->Y += 2;
				break;
			case 10:
			case 30:
			case 50:
			case 70:
				// case 90:
				Enemy->Sprite = bottom_flower_l2;
				H = 30; // Enemy->Height = Enemy->Height2 = 30;
				DY = -2; // Enemy->Y -= 2;
				(Enemy + 1)->Sprite = bottom_flower_r2;
				//(Enemy+1)->Height = (Enemy+1)->Height2 = 30;
				//(Enemy+1)->Y -= 2;
				break;
			// case 100:
			case 80:
				Enemy->Sprite = bottom_flower_l3;
				H = 32; // Enemy->Height = Enemy->Height2 = 32;
				DY = -2; // Enemy->Y -= 2;
				(Enemy + 1)->Sprite = bottom_flower_r3;
				//(Enemy+1)->Height = (Enemy+1)->Height2 = 32;
				//(Enemy+1)->Y -= 2;
				Enemy_add_underwater_shot(Enemy->X + 4,
							  Enemy->Y + 12, -1);
				Enemy_add_underwater_shot(Enemy->X + 12,
							  Enemy->Y + 12, 1);

				break;
			}

			if (H) {
				Enemy->Height = Enemy->Height2 = H;
				(Enemy + 1)->Height = (Enemy + 1)->Height2 = H;
			}
			if (DY) {
				Enemy->Y += DY;
				(Enemy + 1)->Y += DY;
			}

		} else { // right part, slave
		}
	}
};

char Enemy_move_x(struct enemy *Enemy)
{
	char Face = Enemy->Face;

	if (/*Enemy->*/ Face > 0) { // right
		if (Free_right(Enemy->X, &Enemy->Y, Enemy->Height,
			       /*Enemy->*/ Face) >
		    /*Enemy->*/ Face) { // free right?
			Enemy->X += /*Enemy->*/ Face;
		} else {
			Enemy->Face = -/*Enemy->*/ Face; // change direction
			return -1;
		}

	} else {
		if (/*Enemy->*/ Face < 0) { // left
			if (Free_left(Enemy->X, &Enemy->Y, Enemy->Height,
				      /*Enemy->*/ Face) >
			    (-/*Enemy->*/ Face)) { // free left?
				Enemy->X += /*Enemy->*/ Face;
			} else {
				Enemy->Face =
					-/*Enemy->*/ Face; // change direction
				return 1;
			}
		}
	}

	return 0;
};

void Dummy_func_(void *P) {
	// nothing
};

void Handle_hardkilled_enemies(struct enemy *Enemy)
{
	Enemy->Mode++;

	if (Enemy->Mode < 3) {
		Enemy->Y -= 4;
	} else {
		if (Enemy->Mode > 6)
			Enemy->Y += 4;
	}

	Enemy->X += Enemy->Face;

	// if( Enemy->Mode > 28 )//with speed 4 in y dir this nr ensures that none is
	// removed before they have left the screen
	if ((FgY + screen_height) <= (Enemy->Y)) { // active_y_lower
		Enemy->Life = 0; // Enemy_remove(Enemy);

		/*switch(Enemy->Respawn){
            case (-bomb):
                    Spawn_bomb(Enemy);
                    if(Enemy->Respawn2==4){
                            Spawn_bomb2(Enemy);
                    }
                    else{
                            Spawn_bomb(Enemy);
                    }

            break;

            //case (bomb):
            //	Spawn_bomb2(Enemy);
            //break;
    }*/

		if (Enemy->Respawn == -bomb) {
			if (Enemy->Respawn2 == 4) {
				Spawn_bomb2(Enemy);
			} else {
				Spawn_bomb(Enemy);
			}
		}
	}
};

void Handle_dead_upside_down_enemies(struct enemy *Enemy)
{
	if ((Enemy->Y += 2) >= 16 * Leveldata.Height) {
		Enemy->Active = 0;
		// Enemy->Life = 0;
	}
};

// enemy die funcs. Handler the death sequence of the enemies. see enemies.txt
// for details
void Enemy_die_1(struct enemy *Enemy)
{
	Enemy->Attribs =
		0b00100000; // Enemy->Attribs & 0b00100111;//0b00100000;//
	Enemy->Height = Enemy->Height2 = 5;
	Enemy->Sprite = monster1_dead_sprite;
	(Enemy->Handler) = Enemy_handler_5;
	Enemy->Mode = 60; // timer: nr of frames to show sprite
	(Enemy->Die) = (void *)Dummy_func_; // E_dummy_handler;
	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
}
void Enemy_die_2(struct enemy *Enemy)
{ // the death of the turtle and beetle.
	// leaves behind a shell

	Enemy->Attribs = 0b11100010;
	uint16_t *Sprite;
	if (Enemy->Life == 2) {
		/*Enemy->*/ Sprite = beetle_shell_sprite;
	} else {
		/*Enemy->*/ Sprite = shell_0_sprite;
		Enemy->Height = Enemy->Height2 = 16;
	}
	Enemy->Sprite = Sprite;

	(Enemy->Handler) = Enemy_handler_6;
	Enemy->Mode = 60; // timer. Nr of frames to resurection
	(Enemy->Die) = Enemy_die_5;
	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
}
void Enemy_die_3(struct enemy *Enemy)
{ // flying goomba
	(Enemy->Handler) = Enemy_handler_1;
	Enemy->Sprite = monster1_sprite;
	Enemy->Height = Enemy->Height2 = 15;
	(Enemy->Die) = Enemy_die_1;
	Score_anim(Enemy->X, Enemy->Y - 8, score_200);
	Enemy->Mode = 0;
	Enemy->Attribs = 0b11111001; //(Enemy->Attribs | 00100000);//gravity on
	Enemy->Jumping = 0;
	Player.Yoffset = -4;
	Player.Jumpspeed = 4;
}
void Enemy_die_4(struct enemy *Enemy)
{ // flying turtle dies and becomes normal turtle

	// see if using Generate_handler1_enemy can save bytes

	//	Generate_handler_1_enemy(short Nr,turtle_limited,Enemy->Face,0);

	Enemy->Sprite = turtle_right_sprite;
	if (Enemy->Face > 0) {
		Enemy->Sprite += 16 * 3;
	}

	Enemy->Height = Enemy->Height2 = 26;
	Enemy->Jumping = 0;
	Enemy->Handler = Enemy_handler_1;
	Enemy->Attribs = 0b11111000;
	Enemy->Die = Enemy_die_2;
	Enemy->Mode = 1;
	Enemy->Data0 = 0;
	Enemy->Active = 3;
	Score_anim(Enemy->X, Enemy->Y - 8, score_200);
}

void Enemy_die_5(struct enemy *Enemy)
{
	Player.Xoffset = (Player.X > Enemy->X ? 2 : -2);

	Add_shell(/*Enemy->X,Enemy->Y,*/ (
			  (Player.X >= Enemy->X) ? -shell_speed : shell_speed),
		  Enemy->Life, Enemy);
	Score_anim(Enemy->X, Enemy->Y - 8, score_200);
	Enemy->Mode = Enemy->Life;
	Enemy->Life = 0;
}

void Enemy_die_6(struct enemy *Enemy)
{
	Enemy->Life = -1;
	Enemy->Attribs = 0;
	Enemy->Y += Enemy->Height;
	Score_anim(Enemy->X, Enemy->Y - 8, score_200);
	(Enemy->Handler) =
		Handle_dead_upside_down_enemies; // maybe Handle_hardkilled_enemies  ?
}

void Enemy_die_7(struct enemy *Enemy)
{
	Enemy->Attribs = 0; // b00100000;
	Enemy->Sprite = crushed_turtle_skeleton_sprite;
	Enemy->Height = Enemy->Height2 = 12;
	Enemy->Y += 15;
	(Enemy->Handler) = Enemy_handler_7;
	Enemy->Mode = 120; // timer. Nr of frames to resurection
	(Enemy->Die) = (void *)Dummy_func_; // E_dummy_handler;
	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
};

void Enemy_die_8(struct enemy *Enemy)
{
	Enemy->Life = 0;
	Add_dustsky(Enemy->X, Enemy->Y + Enemy->Height - 8);
	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
};

void Enemy_die_9(struct enemy *Enemy)
{ // The death of the jumping brick
	int16_t C;

	Enemy->Life = 0;

	Add_break_brick_anim(Enemy->X, Enemy->Y);

	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
};

#define scanwidth 7 * 16

void Enemy_die_10(struct enemy *Enemy)
{ // handles the teath of boomerang guy,
	// fireball guy and hammer man
	// when brothers and/or treasure
	int16_t C, Y = Enemy->Y; //<=important!

	if (Enemy->Active == 4) { // if alone

		//		for(C=0;Free_down(Enemy->X-4*16,Y+C+1);C++);//search for the
		// floor

		//		Put_tile(Enemy->X-4*16,Y+C,Enemy->Life);//Treasure

		// find start X
		//			short ScanX = Enemy->X + (Enemy->X>Player.X ?
		// scanwidth/2:-scanwidth/2);

		int16_t ScanX =
			Player.X +
			(Enemy->X > Player.X ? -scanwidth / 2 : scanwidth / 2);

		if (ScanX >
		    min(Enemy->X + scanwidth,
			Leveldata.Border_right)) { // rightmost scan column reached
			ScanX = min(
				Enemy->X + scanwidth,
				Leveldata.Border_right); // go back to leftmost scan column
		}

		if (ScanX <
		    max(Enemy->X - scanwidth,
			Leveldata.Border_left)) { // rightmost scan column reached
			ScanX = max(
				Enemy->X - scanwidth,
				Leveldata.Border_left); // go back to leftmost scan column
		}

		int16_t StartX = ScanX; // -16;
		int16_t ScanY;

		do { // scan X-wise

			// ScanY = Enemy->Y - 2*16;
			ScanY = Leveldata.Height * 16 - 16;

			/*while( (Get_tile(ScanX,ScanY)<solid_low) &&
      (ScanY<(Leveldata.Height*16-16)) ){//scan downwards ScanY += 16;
      }*/
			int16_t SolidFound = 0;
			while (((Get_tile(ScanX, ScanY) >= solid_low) ||
				(!SolidFound)) &&
			       (ScanY > 0)) { // scan upwards
				if (Get_tile(ScanX, ScanY) >= solid_low)
					SolidFound = 1;
				ScanY -= 16;
			}

			if ((Get_tile(ScanX, ScanY /*-16*/) == 0) &&
			    (ScanY < (Leveldata.Height * 16))) {
				Put_tile(ScanX, ScanY /*-16*/,
					 Enemy->Life); // Treasure
				ScanX = StartX; // abort scan
				break;
			}

			ScanX += 16;
			if (ScanX >
			    min(StartX + scanwidth,
				Leveldata.Border_right)) { // rightmost scan column reached
				ScanX = max(
					StartX - scanwidth,
					Leveldata.Border_left); // go back to leftmost scan column
			}

		} while (ScanX != StartX);

	} else { // not alone

		(Enemy + Enemy->Active - 2)->Active = 4; // set brother as alone
	}

	Enemy->Die =
		(void *)Dummy_func_; // prevent this func to be executed twice,
	// destroying treasure
	Enemy_die_6(Enemy);
};

void Enemy_die_11(struct enemy *Enemy)
{ // death of the bomb. Initializes countdown

	if (Enemy->Mode < 2) {
		// initialize countdown
		Enemy->Mode = 120;
		Enemy->Attribs = 0b10101011;
		Enemy->Life = 9;
		Enemy->Height = 14;

	} else {
		if (!Enemy->Jumping) { // if not already in a bounch
			// bounch
			Enemy->Jumping = -3; //-2
			Enemy->Data0 = 16;

			if (Enemy->X > Player.X) {
				Enemy->Face = 4; // 3
				Enemy->X = Player.X + 16;

			} else {
				Enemy->Face = -4; // 3
				Enemy->X = Player.X - 16;
			}
		}

		//		Player.Walkspeed = Player.Walkspeed2 = 0;

		//		Keystate.Left = Keystate.Right = 0;
	}
}

void Enemy_die(struct enemy *Enemy)
{ // kills enemy and animates. Used when hit
	// by shells and fireballs
	Enemy->Life = -1;
	Enemy->Attribs = 0;
	(Enemy->Handler) = Handle_hardkilled_enemies;
	(Enemy->Die) = (void *)Dummy_func_; // this might prevent strange bugs
	Enemy->Mode = 0;
	Enemy->Face = (Player.X > Enemy->X) ? -4 : 4;
	Score_anim(Enemy->X, Enemy->Y - 8, score_100);
}

void Handle_enemyshots()
{
	int16_t C, D;

	for (C = 0; C < nr_of_enemyshots; C++) {
		if (Enemyshots[C].Mode) { // if existing
			Enemyshots[C].Handler(&Enemyshots[C]);

			if (/*(!Player.Immortal) &&*/ TestCollide162h_R(
				Player.X, Player.Y, Enemyshots[C].X,
				Enemyshots[C].Y, Player.Height,
				Enemyshots[C].Height,
				Mariosprites +
					2 * Marioanimtab[SavePlayer.Spritebase]
							[SavePlayer.Spritenr],
				Enemyshots[C].Sprite)) {
				if (((Enemyshots[C].Mode < 0) &&
				     ((Player.Y + Player.Height - 8) <=
				      Enemyshots[C].Y)) ||
				    ((SavePlayer.Attribs & 0b10000000) &&
				     (Enemyshots[C].Mode < 0))) {
					// This constant adjust the difficulty of killing shots ^
					Player.IsJumping =
						0.75 *
						jumpheight; // enable rejump
					Player.Yoffset = -2;
					// add object

					if ((D = Find_free_object()) != -1) {
						Objects[D].Active = 1;

						if (Enemyshots[C].Mode == -1) {
							Objects[D].Data0 =
								14 * 3 *
								2; // sprite offset
							Objects[D].Data1 = 12;
						} else {
							Objects[D].Data1 = 14;
							if (Enemyshots[C].Data0 <
							    0) {
								Objects[D]
									.Data0 =
									0; // sprite offset
							} else {
								Objects[D]
									.Data0 =
									14 *
									3; // sprite offset
							}
						}
						Enemyshots[C].Mode =
							0; // kill shot
						// Objects[C].Data1 = (Enemyshots[C].Mode==-1?12:14);//height
						Objects[D].X = Enemyshots[C].X;
						Objects[D].Y = Enemyshots[C].Y;
						//(object*)Objects[D].Handler = Dummy_func_;//Handle_drop_down;
						Objects[D].Draw =
							Draw_killed_cannonballs;
					}

				} else {
					Player_die();
				}
			}

			if ((FgX > Enemyshots[C].X) ||
			    ((FgX + screen_width) < Enemyshots[C].X) ||
			    ((FgY - 16 * 2) > Enemyshots[C].Y) ||
			    ((FgY + screen_height) < Enemyshots[C].Y))
				Enemyshots[C].Mode =
					0; // kill shot if outside screen
		}
	}
};

void Enemy_fireball_handler(struct shot *Fireball)
{
	Fireball->X += Fireball->Data1; //*enemy_fireball_speed;
	Fireball->Y += Fireball->Data0; //*enemy_fireball_speed;
	Fireball->Sprite = fireball_anim_16_spr + 24 * Fg_plane.step;
};

void Hammer_handler(struct shot *Hammer)
{
	Hammer->Mode++;

	if (Hammer->Mode == 8) {
		Hammer->Data1 = 0;
	} else {
		if (Hammer->Mode == 16) {
			Hammer->Data1 = 3;
		} else {
			if (Hammer->Mode == 24) {
				Hammer->Data0 = 0;
			}
		}
	}

	Hammer->X += Hammer->Data0;
	Hammer->Y += Hammer->Data1;

	Hammer->Sprite = hammer_anim_spr + 16 * 3 * Fg_plane.step;
};

int16_t Find_free_enemyshot()
{
	int16_t C;

	for (C = 0; C < nr_of_enemyshots; C++) {
		if (!(Enemyshots[C].Mode)) {
			return C;
		}
	}
	return -1;
}

void Enemy_add_fireball(int16_t X, int16_t Y, /*char*/ int16_t Xdir,
			/*char*/ int16_t Ydir)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		//	if(!(Enemyshots[C].Mode)){
		struct shot *Shot = &Enemyshots[C];
		Shot->Mode = 2; // find free shot (Mode==0)
		Shot->Data1 = Xdir;
		Shot->Data0 = Ydir;
		Shot->Height = 8;
		Shot->Sprite = fireball_anim_16_spr;
		Shot->X = X;
		Shot->Y = Y;
		Shot->Handler = Enemy_fireball_handler; // experiment
		// shot
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Enemy_add_hammer(int16_t X, int16_t Y, /*char*/ int16_t Xdir)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)
		struct shot *Shot = &Enemyshots[C];
		Shot->Mode = 1;
		Shot->Data0 = Xdir;
		Shot->Data1 = -2;
		Shot->Height = 16;
		Shot->Sprite = hammer_anim_spr;
		Shot->X = X;
		Shot->Y = Y;

		Shot->Handler = Hammer_handler;
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Enemy_add_bounching_fireball(int16_t X, int16_t Y, /*char*/ int16_t Xdir,
				  /*char*/ int16_t Ydir)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)
		struct shot *Shot = &Enemyshots[C];
		Shot->Mode = 3;
		// Enemyshots[C].Data1 = Xdir;
		Shot->Data2 = Xdir;
		Shot->Data0 = Ydir;
		//			Enemyshots[C].Data1 = 0;
		Shot->Height = 8;
		Shot->Sprite = fireball_anim_16_spr;
		Shot->X = X;
		Shot->Y = Y;
		Shot->Handler = Player_fireball_handler; // experiment shot
		// break;//C = nr_of_enemyshots;
		//};
	}
}

void Enemy_add_boomerang(int16_t X, int16_t Y, /*char*/ int16_t Dir)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)
		struct shot *Shot = &Enemyshots[C];
		Shot->Mode = 4;
		Shot->Data0 = Dir;
		Shot->Data1 = Dir;
		Shot->Height = 16;
		Shot->Sprite = boomerang_anim_16_spr;
		Shot->X = X + (Dir > 0 ? 8 : -8);
		Shot->Y = Y;
		Shot->Handler = Enemy_boomerang_handler; // experiment shot
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Enemy_add_cannonball_horiz(int16_t X, int16_t Y, char Dir)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)
		struct shot *Shot = &Enemyshots[C];
		Shot->Data0 = Dir;
		Shot->Data1 = 0;
		Shot->Height = 14;
		Shot->Sprite = (Dir > 0 ? cannonball_horiz_right_spr :
					  cannonball_horiz_left_spr);
		Shot->X = X; // + (Dir>0?8:-8);
		Shot->Y = Y;
		Shot->Mode = -2;
		Shot->Handler = Enemy_cannonball_horiz_handler;
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Enemy_add_cannonball(int16_t X, int16_t Y, char DirX, char DirY)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)
		struct shot *Shot = &Enemyshots[C];
		Shot->Data0 = DirX;
		Shot->Data1 = DirY;
		Shot->Height = 12;
		Shot->Sprite = cannonball_spr;
		Shot->X = X + DirX;
		Shot->Y = Y + DirY;
		Shot->Mode = -1;
		Shot->Handler = Enemy_cannonball_handler; // experiment shot
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Enemy_add_underwater_shot(int16_t X, int16_t Y, /*char*/ int16_t DirX)
{
	int16_t C;

	if ((C = Find_free_enemyshot()) >= 0) {
		// for(C=0;C<nr_of_enemyshots;C++){
		// if(!(Enemyshots[C].Mode)){//find free shot (Mode==0)

		struct shot *Shot = &Enemyshots[C];
		Shot->Data0 = DirX;
		Shot->Data1 = 48;
		Shot->Height = 8;
		Shot->Sprite = underwater_shot;
		Shot->X = X;
		Shot->Y = Y;
		Shot->Mode = 5;
		Shot->Handler = Underwater_shot_handler;
		// break;//C = nr_of_enemyshots;
		//};
	}
};

void Underwater_shot_handler(struct shot *Shot)
{
	if (!(Fg_plane.frame % 2)) { // slow down

		if (Shot->Data1) { // up
			Shot->Data1--;
			Shot->Y--;
		} else { // down
			Shot->Y++;
		}

		if (Fg_plane.frame == 2) {
			Shot->X += Shot->Data0;
		}
	}
};

void Enemy_cannonball_horiz_handler(struct shot *Cannonball)
{
	Cannonball->X += Cannonball->Data0;
};

void Enemy_cannonball_handler(struct shot *Cannonball)
{
	Cannonball->X += Cannonball->Data0;
	Cannonball->Y += Cannonball->Data1;
};

void Enemy_boomerang_handler(struct shot *Boomerang)
{
	// restricted to move 2 pixels in x dir each frame, for optimization.
	// to use variable speed: use commented out code

	Boomerang->Data1 += Boomerang->Data0;

	Boomerang->X += Boomerang->Data0;

	if (abs(Boomerang->Data1) >= 16 * 2 * 2) // 16*2*abs(Boomerang->Data0)
		Boomerang->Data0 = -Boomerang->Data0;

	if ((16 * 2 * 2 - abs(Boomerang->Data1)) <
	    8 * 2) //(16*2*abs(Boomerang->Data0)-abs(Boomerang->Mode))
		//< 8*abs(Boomerang->Data0)
		Boomerang->Y += 2; // abs(Boomerang->Data0);

	// animation
	Boomerang->Sprite = boomerang_anim_16_spr + 48 * Fg_plane.step;
};
