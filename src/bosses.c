#include "bosses.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "control.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "player.h"
#include "render.h"
#include "scankeys.h"
#include <stdint.h>
#include <string.h>

struct boss *BossG;

void Handlebosses()
{
	BossG->Handler(BossG);

	// collition detection: player-boss
	if (TestCollide162h_R(Player.X, Player.Y, BossG->X, BossG->Y,
			      Player.Height, BossG->Height,
			      Mariosprites +
				      2 * Marioanimtab[SavePlayer.Spritebase]
						      [SavePlayer.Spritenr],
			      BossG->Sprite) ||
	    TestCollide162h_R(Player.X, Player.Y, BossG->X + 16, BossG->Y,
			      Player.Height, BossG->Height,
			      Mariosprites +
				      2 * Marioanimtab[SavePlayer.Spritebase]
						      [SavePlayer.Spritenr],
			      BossG->Sprite + 3 * BossG->Height2)) {
		if (BossG->Active && Player.IsFalling &&
		    (Player.Y <
		     BossG->Y)) { // something more: x match (hit head)
			Player.IsJumping = 0.75 * jumpheight; // enable rejump
			Player.Yoffset = -4;
			Player.Xoffset = 4 * Player.Face;
			Player.Walkspeed = 4;
			if (!BossG->Mode)
				BossG->Die(BossG);
		} else {
			if (!BossG->Mode && BossG->Life)
				Player_die();
		}
	}
}

int16_t Boss_free_down(struct boss *Boss)
{
	return min(Free_down(Boss->X, Boss->Y + Boss->Height),
		   Free_down(Boss->X + 15, Boss->Y + Boss->Height));
}

int16_t Boss_free_up(struct boss *Boss)
{
	return min(Free_up(Boss->X, Boss->Y), Free_up(Boss->X + 15, Boss->Y));
}

void Add_killed_boss(struct boss *Boss, int16_t Type)
{
	Items[0].X = Boss->X + 8;
	Items[0].Y = Boss->Y + 1 - 16;
	Items[0].Height = 16;
	Items[0].Height2 = 16;
	Items[0].Active = Type;
	Items[0].Sprite = (Type == 1 ? boss1_killed_spr :
				       magic_wand_spr); // magic_wand_spr;
	Items[0].Collect = Player_collect_dead_boss;
	Items[0].Handler = Dead_boss_handler;

	Leveldata.Boss = 0;
}

void Boss_handler_1(struct boss *Boss)
{
	int16_t C, Temp, Temp2;
	char Face = Boss->Face;
	char Data0 = Boss->Data0;

	if (Boss->Active) {
		if (Boss->Mode) {
			if ((--Boss->Mode) == 0) {
				Boss->Sprite = boss1_run1_spr;
				Boss->Height = Boss->Height2 = 27;
				Boss->Y -= 10;
			}

		} else {
			if (abs((Player.X + 8) - (Boss->X + 16)) > 32) {
				if (Player.X < Boss->X) {
					/*Boss->*/ Face = -2;
				} else {
					/*Boss->*/ Face = 2;
				}
			}

			if ((/*Boss->*/ Face > 0)) { // dir = right

				if (/*Boss->*/ Face <=
				    Free_right(Boss->X + 15, &Boss->Y,
					       Boss->Height, /*Boss->*/ Face)) {
					Boss->X += /*Boss->*/ Face;
				} else {
					/*Boss->*/ Face = -/*Boss->*/ Face;
				}
			} else { // dir = left
				if ((-/*Boss->*/ Face) <=
				    Free_left(Boss->X, &Boss->Y, Boss->Height,
					      /*Boss->*/ Face)) {
					Boss->X += /*Boss->*/ Face;
				} else {
					/*Boss->*/ Face = -/*Boss->*/ Face;
				}
			}

			if (!(Boss->Life % 2)) { // jumping

				if (abs(Boss->X - Player.X) >= 64) {
					/*Boss->*/ Data0 =
						abs(Boss->X - Player.X) / 4;

					Boss->Data1 = -4;
				}

				if (/*Boss->*/ Data0) {
					/*
                          Temp = Free_up(Boss->X,Boss->Y);
                          Temp2 = Free_up(Boss->X+31,Boss->Y);
                          Temp = (Temp>Temp2?Temp2:Temp);//the smallest
          */
					Temp = Boss_free_up(Boss);

					Boss->Y -= (Temp < 4 ?
							    Temp :
							    4); // Boss->Data1;
					/*Boss->*/ Data0 -= 4; // Boss->Data1;

					if (Temp == 0)
						/*Boss->*/ Data0 = 0;
				}
			}

			// run anim

			if (Fg_plane.step % 2) {
				Boss->Sprite = boss1_run1_spr;
			} else {
				Boss->Sprite = boss1_run2_spr;
			}

		} // mode

	} else {
		if (Boss->Life) {
			if ((abs(Boss->X - Player.X) < 64) &&
			    (abs(Boss->Y - Player.Y) < 64) &&
			    !(/*Boss->*/ Data0)) {
				// Boss->Active = 1;//wake up
				/*Boss->*/ Data0 = 60;
			}

			if (/*Boss->*/ Data0) {
				if ((--/*Boss->*/ Data0) == 0) {
					Boss->Active = 1; // wake up
					Boss->Sprite = boss1_run1_spr;
					Boss->Height = Boss->Height2 = 27;
					Boss->Y -= 4;
				}
			}
		} else { //! Life

			if ((--Boss->Mode) == 0) {
				/*
        Items[0].X = Boss->X+8;
        Items[0].Y = Boss->Y+1-16;
        Items[0].Height = 16;
        Items[0].Height2 = 16;
        Items[0].Active = 1;
        Items[0].Sprite = boss1_killed_spr;
        //Items[0].Mask = boss1_killed_msk;
        (item*)Items[0].Collect = Player_collect_dead_boss;
        (item*)Items[0].Handler = Dead_boss_handler;

        Leveldata.Boss = 0;
        */
				Add_killed_boss(Boss, 1);
			}
		}
	}

	if (!/*Boss->*/ Data0) { // fall
		/*
            Temp = Free_down(Boss->X,Boss->Y+Boss->Height);
            Temp2 = Free_down(Boss->X+31,Boss->Y+Boss->Height);
            Temp = min(Temp,Temp2);//(Temp>Temp2?Temp2:Temp);//the smallest
    */
		Temp = Boss_free_down(Boss);

		if (Temp) {
			Boss->Y += min(Temp, 4); //(Temp<4?Temp:4);
		}
	}

	Boss->Data0 = Data0;
	Boss->Face = Face;
}

#define threshold 5 * 16

void Boss_handler_2(struct boss *Boss)
{
	int16_t xDist, C, Temp, Temp2;
	char Face = Boss->Face;
	char Data0 = Boss->Data0;

	if (Boss->Active) {
		if (Boss->Mode) {
			if ((--Boss->Mode) == 0) {
				Boss->Height = Boss->Height2 = 30;
				Boss->Y -= 13;
				/*Boss->*/ Data0 = 16;
			}

		} else {
			if (Boss->Life) {
				xDist = Player.X - (Boss->X + 16);

				if ((++Boss->Data1) == 80) {
					Boss->Data1 = 0;
					Temp = Player.Y - Boss->Y;

					if (Temp >= 4) {
						Temp2 = 1;
					} else {
						if (Temp <= -4) {
							Temp2 = -1;
						} else {
							Temp2 = 0;
						}
					}

					Add_boss_shot(
						Boss->X + (/*Boss->*/ Face > 0 ?
								   24 :
								   7),
						Boss->Y + 7,
						(/*Boss->*/ Face > 0 ? 2 : -2),
						Temp2 /*((Player.Y-Boss->Y)>0?2:-2)*/);
				} else {
				}

				if (xDist > 0) {
					Boss->Sprite = boss2_right_spr;
					/*Boss->*/ Face =
						(/*Boss->*/ Data0 ? 2 : 1);
				} else {
					Boss->Sprite = boss2_left_spr;
					/*Boss->*/ Face =
						(/*Boss->*/ Data0 ? -2 : -1);
				}

				Temp = Boss_free_down(
					Boss); // Free_down(Boss->X+8,Boss->Y+Boss->Height);

				if (abs(xDist) < threshold) {
					// walk against player

				} else {
					// jump against player
					if ((!/*Boss->*/ Data0 &&
					     !Temp /*Free_down(Boss->X+8,Boss->Y+Boss->Height)*/)) {
						/*Boss->*/ Data0 = abs(xDist);
					}
				}

				if (/*Boss->*/ Face > 0) { // right
					if (Free_right(Boss->X + 15, &Boss->Y,
						       Boss->Height,
						       /*Boss->*/ Face)) {
						Boss->X += /*Boss->*/ Face;
					} else {
						if ((Free_right(
							     Boss->X + 15,
							     &Boss->Y,
							     Boss->Height - 16,
							     /*Boss->*/ Face) &&
						     !Temp /*Free_down(Boss->X+8,Boss->Y+Boss->Height)*/)) {
							// jump against player
							/*Boss->*/ Data0 = 16;
						}
					}

				} else {
					if (Free_left(Boss->X, &Boss->Y,
						      Boss->Height,
						      /*Boss->*/ Face)) {
						Boss->X += /*Boss->*/ Face;
					} else {
						if ((Free_left(
							     Boss->X, &Boss->Y,
							     Boss->Height - 16,
							     /*Boss->*/ Face) &&
						     !Temp /*Free_down(Boss->X+8,Boss->Y+Boss->Height)*/)) {
							// jump against player
							/*Boss->*/ Data0 = 16;
						}
					}
				}

				if (/*Boss->*/ Data0) {
					/*
                  Temp = Free_up(Boss->X,Boss->Y);
                  Temp2 = Free_up(Boss->X+31,Boss->Y);
                  Temp = (Temp>Temp2?Temp2:Temp);//the smallest
          */
					Temp = Boss_free_up(Boss);

					Boss->Y -= (Temp < 4 ?
							    Temp :
							    4); // Boss->Data1;

					/*Boss->*/ Data0 -= 4;

					if (/*Boss->*/ Data0 < 0) {
						/*Boss->*/ Data0 = 0;
					}

					if (Temp == 0)
						/*Boss->*/ Data0 = 0;
				}

			} else { //! Life
			}
		}
	} else { //! Active

		if (Boss->Life) {
			if ((abs(Boss->X - Player.X) < 64) &&
			    (abs(Boss->Y - Player.Y) < 64) &&
			    !(/*Boss->*/ Data0)) {
				/*Boss->*/ Data0 = 20;
			}

			if (/*Boss->*/ Data0) {
				if ((--/*Boss->*/ Data0) == 0) {
					Boss->Active = 1; // wake up
				}
			}

		} else { //! life
			if ((--Boss->Mode) == 0) {
				/*
        Items[0].X = Boss->X+8;
        Items[0].Y = Boss->Y+1-16;
        Items[0].Height = 16;
        Items[0].Height2 = 16;
        Items[0].Active = 2;
        Items[0].Sprite = magic_wand_spr;
        //Items[0].Mask = magic_wand_msk;
        (item*)Items[0].Collect = Player_collect_dead_boss;
        (item*)Items[0].Handler = Dead_boss_handler;

        Leveldata.Boss = 0;
        */
				Add_killed_boss(Boss, 2);
			}
		}
	}

	if (!/*Boss->*/ Data0) { // fall
		/*
            Temp = Free_down(Boss->X,Boss->Y+Boss->Height);
            Temp2 = Free_down(Boss->X+31,Boss->Y+Boss->Height);
//		Temp = (Temp>Temp2?Temp2:Temp);//the smallest
            Temp = min(Temp,Temp2);
    */
		Temp = Boss_free_down(Boss);

		if (Temp) {
			Boss->Y += (Temp < 4 ? Temp : 4);
		}
	}

	Boss->Data0 = Data0;
	Boss->Face = Face;
}

#undef threshold

#define threshold 5 * 16

/*void Dummy_boss(boss *Boss){

}*/

void Draw_ending_text(struct object *Object)
{
	FilledRectEraseLight(47 + screen_offset_sg_x, 7,
			     115 + screen_offset_sg_x, 38);
	FilledRectEraseDark(47 + screen_offset_sg_x, 7,
			    115 + screen_offset_sg_x, 38);

	DrawStringLocale(50 + screen_offset_sg_x, 10, "Ending1", A_REPLACE,
			 F_4x6);
	DrawStringLocale(50 + screen_offset_sg_x, 20, "Ending2", A_REPLACE,
			 F_4x6);

	if (Object->Data0) {
		DrawStringLocale(60 + screen_offset_sg_x, 30, "Ending3",
				 A_REPLACE, F_4x6);
	}
}

void Bowser_handler(struct boss *Boss1)
{
	struct boss TempBoss;
	struct boss *Boss = &TempBoss;

	memcpy(&TempBoss, Boss1, sizeof(struct boss));

	if (Boss->Active && (Boss->Mode == 0)) {
		if (!Boss->Data0) { // not in a jump
			if ((++Boss->State) > 182) {
				Boss->State = 1; // restart cycle
			}

			switch (Boss->State) {
			case 11:
			case 92:
				// start chasing
				Boss->Data1 = -1; // chase flag
				Boss->Face = ((Boss->X < Player.X) ? 3 : -3);
				break;

			case 81:
			case 180:
				// stop chasing
				Boss->Face = 0;
				Boss->Data1 = 0; // chase flag
				break;

			case 91:
				// start power jump
				Boss->Face = ((Boss->X < Player.X) ? 2 : -2);
				Boss->Data0 =
					abs(Player.X - Boss->X + 16); /// 2;
				Boss->Data1 = 1; // Power jump flag
				Boss->Sprite = bowser_facing_spr;
				break;

			case 181:
				// fire
				Add_big_fireball_shot(
					Boss->X + ((Player.X > Boss->X) ? 28 :
									  -4),
					Boss->Y + 8,
					((Player.X > Boss->X) ? 4 : -4), 0);
				break;
			}
		}

		if (Boss->Data1 == -1) {
			// chasing

			int16_t Dist = Player.X - Boss->X + 16;

			if (abs(Dist) > threshold) {
				if (Dist > 0) {
					Boss->Face = 3;
				} else {
					Boss->Face = -3;
				}
			}
		}

		int16_t Dist = 16, Dist2;

		if (Boss->Data0 > 0) { // Handle jump and powerjump
			/*
      short Dist=16,Dist2,DX;

      for(DX=0;DX<=16;DX+=16){
              Dist2 = Free_up(Boss->X+DX,Boss->Y);

              if(Dist2<Dist)
                      Dist = Dist2;

              if( Dist>=3 ){
                      Boss->Y -= 3;
                      Boss->Data0 -= 3;
                      if(Boss->Data0==0){
                              Boss->Data0 = -1;
                      }
              }
              else{
                      //hit the roof, start fall
                      Boss->Data0 = -1;
              }
      }
      */

			int16_t Dist = Boss_free_up(Boss);

			if (Dist >= 3) {
				Boss->Y -= 3;
				Boss->Data0 -= 3;
				if (Boss->Data0 == 0) {
					Boss->Data0 = -1;
				}
			} else {
				// hit the roof, start fall
				Boss->Data0 = -1;
			}

			/*if( Free_up(Boss->X,Boss->Y)>0 && Free_up(Boss->X+16,Boss->Y) ){
              Boss->Y -= 3;
              Boss->Data0 -= 3;
      }
      else{
              Boss->Data0 = -2;//abort jump, start to fall

      }*/

		} else { // gravity

			if (Boss->Data0 == 0)
				Boss->Sprite = (Boss->X > Player.X ?
							bowser_run_left_spr :
							bowser_run_right_spr);

			int16_t /*Dist=16,Dist2,*/ DX;

			//			for(DX=0;DX<=16;DX+=16){
			//				Dist2 =
			// Free_down(Boss->X+DX,Boss->Y+Boss->Height);

			//				if(Dist2<Dist)
			//					Dist = Dist2;

			/*if( Dist<=3 ){
              //hit the ground

              if(Boss->Data1 == 1){//powerjump
                      //break bricks if any

                      unsigned char Tile =
      Get_tile(Boss->X+DX,Boss->Y+Boss->Height+5);

                      if( (Tile>=solid_interactive_bounch_low) &&
      (Tile<=solid_interactive_bounch_high) ){
                              //trigger interactive tile (Likely brick)
                              Interactive_tile_handlers[Tile-solid_interactive_low](Boss->X+DX,Boss->Y+Boss->Height+5);
                      }

                      Boss->Data1 = 0;
              }

              Boss->Data0 = 0;//end jump
              //Boss->Face = 0;
              break;
      }*/
			//			}

			Dist = Boss_free_down(Boss);

			if (Dist >= 0) {
				Boss->Y += min(Dist, 3);

				//				while(1);

				if (!Boss_free_down(
					    Boss) /*Free_down(Boss->X+DX,Boss->Y+Boss->Height)*/) { // hit
					// the
					// floor?
					uint8_t Tile = Get_tile(
						Boss->X,
						Boss->Y + Boss->Height + 5);
					// short Bang = 0;
					if (Boss->Data1 == 1) { // powerjump
						// break bricks if any
						for (DX = 0; DX <= 32;
						     DX += 16) {
							uint8_t Tile = Get_tile(
								Boss->X + DX,
								Boss->Y +
									Boss->Height +
									5);

							if ((Tile >=
							     solid_interactive_bounch_low) &&
							    (Tile <=
							     solid_interactive_bounch_high)) {
								// trigger interactive tile (Likely brick)
								Interactive_tile_handlers[Tile -
											  solid_interactive_low](
									Boss->X +
										DX,
									Boss->Y +
										Boss->Height +
										5);
								// Bang = 16;
							}
						}

						Boss->Data1 =
							0; // end power jump
					}
					Boss->Data0 =
						0; // Bang;//end jump or bounche
				}

				if (Boss->Y >= (Leveldata.Height * 16)) {
					Boss->Life = Boss->Active = 0; // die
				}
			}
		}

		// x motion

		// add something here: Jump up if trapped in hole: will happend after first
		// powerjump landing

		if (Boss->Face < 0) { // left

			if (Free_left(Boss->X, &Boss->Y, Boss->Height, -2) >=
			    abs(Boss->Face)) {
				Boss->X += Boss->Face;
			} else {
				// check if bowser can jump up
				int16_t DY, Y;

				if (!Boss_free_down(
					    Boss) /*Free_down(Boss->X,Boss->Y+Boss->Height) && !Free_down(Boss->X+16,Boss->Y+Boss->Height)*/) {
					for (DY = 16; DY <= 48; DY += 16) {
						Y = Boss->Y - DY;
						if (/*(Dist==0) &&*/ (
							Free_left(Boss->X, &Y,
								  Boss->Height,
								  -2) >=
							abs(Boss->Face))) {
							Boss->Data0 =
								DY; // while(1);
							// Boss->Y = Y+DY;
							break;
						}
					}
				}
			}
		}
		if (Boss->Face > 0) { // right

			if (/*(Dist==0) &&*/ (Free_right(Boss->X + 16, &Boss->Y,
							 Boss->Height, 2) >=
					      abs(Boss->Face))) {
				Boss->X += Boss->Face;
			} else {
				// check if bowser can jump up
				int16_t DY, Y;
				if (!Boss_free_down(
					    Boss) /*Free_down(Boss->X,Boss->Y+Boss->Height) && !Free_down(Boss->X+16,Boss->Y+Boss->Height)*/) {
					for (DY = 16; DY <= 48; DY += 16) {
						Y = Boss->Y - DY;
						if (Free_right(Boss->X + 16, &Y,
							       Boss->Height,
							       2) >=
						    abs(Boss->Face)) {
							Boss->Data0 =
								DY; // while(1);
							// Boss->Y = Y+DY;
							break;
						}
					}
				}
			}
		}

	} else { //! Active

		if (Boss->Mode) {
			int16_t DX = min(Boss_free_down(Boss), 3);
			//			DX =
			// min(Free_down(Boss->X,Boss->Y+Boss->Height),3); DX =
			// min(DX,Free_down(Boss->X+16,Boss->Y+Boss->Height));
			Boss->Y += DX;
			Boss->Mode--;
			Boss->Data0 = Boss->Data1 =
				0; // abort jump/powerjump: Avoid breaking bricks when he shouldn't
			Boss->State =
				170; // 90;//might change: maybe it becomes to easy with 90
			// (powerjump immedeately after jumpkill)

			// hurt animation
			if ((Fg_plane.step % 2) && (Boss->Mode != 0)) { //>=2
				Boss->Sprite = bowser_angry;
				Boss->Height = Boss->Height2 = 40;
				Boss->Y -= 2;
			} else {
				Boss->Sprite = bowser_facing_spr;
				Boss->Height = Boss->Height2 = 38;
			}

			/*
                                      Boss->Sprite = bowser_angry;
                                      Boss->Height = Boss->Height2 = 40;
                                      Boss->Y -= 2;*/
		} else {
			if (Boss->Life) {
				// bowser is waiting, has not spotted mario yet...
				if ((abs(Boss->X - Player.X) <
				     80) /*&& (abs(Boss->Y-Player.Y)<80)*/
				    && !(Boss->Data0)) {
					Boss->Data0 = 20;
				}

				if (Boss->Data0) {
					if ((--Boss->Data0) == 0) {
						Boss->Active = 1; // wake up
					}
				}

			} else { //! Life
				// bowser is now defeated

				// jumpkilled x 10, or fallen down?

				// animation:
				// flashing
				// bowser goes flies upwards

				int16_t C;
				for (C = 0; C < 20; C++) {
					uint16_t D;
					if (C % 2) {
						// FastFilledRect_Draw_R(dBufHPD_G,0,0,239,127);
						FilledRectDark(0, 0, 239, 127);

						GrayDBufToggleSync_SetPointers();
					} else {
						Render();
					}
					for (D = 0; D < 65000; D++)
						;
				}

				/*			BossG->Sprite = bowser_angry;
                                BossG->Height = BossG->Height2 = 40;
                                BossG->X = FgX + screen_width/2-16;//put bowser
           in the middle of screen (x-wise) BossG->Y = FgY + screen_height;

                                while((BossG->Y+40)>FgY){
                                        BossG->Y -= 2;
                                        Render();
                                }
        */
				// mushrom house with princess (remove treasure boxes)
				Skip_anim = true;
				Load_level(Commonfilename, 0);

				/*	Put_tile(5*16,7*16,0);
                Put_tile(9*16,7*16,0);
                Put_tile(7*16,7*16,0);
                */
				int16_t D;
				for (D = 5; D <= 9; D += 2) {
					Put_tile(D * 16, 7 * 16, 0);
				}

				Items[0].X = 9 * 16;
				Items[0].Y = 8 * 16 - 30;
				Items[0].Height = 30;
				Items[0].Height2 = 30;
				Items[0].Active = 1;
				Items[0].Sprite = princess_spr;

				Objects[0].Active = 1;
				Objects[0].Data0 = 0;
				//			(object*)Objects[0].Handler = Dummy_func_;
				Objects[0].Draw = Draw_ending_text;

				Player.X = 5 * 16;
				Player.Y = 8 * 16 - 27;
				Player.Face = 1;
				SavePlayer.Maskbase = SavePlayer.Spritebase = 3;
				SavePlayer.Spritenr = 0;
				Player.Height = Player.Height2 = 27;

				Adjust_renderpoint();

				Render();

				//			Exit = 2;
				Exit = 99;

				// text
				//			Draw_ending_text(0);

				// delay

				for (; ((Player.X < (7 * 16 - 8)) &&
					(Items[0].X > (7 * 16 + 8)));
				     Player.X++) { // mario and princess running against each other
					Items[0].X--;
					Render();
					//				Draw_ending_text(1);
					if ((++SavePlayer.Spritenr) >= 4) {
						SavePlayer.Spritenr = 0;
					}
				}

				Objects[0].Data0 = 1;
				Render();

				/*while(!_rowread(0));//wait until key pressed
        while(_rowread(0));//wait until key released*/
				WaitKeyPress();
			}
		}
	}

	memcpy(Boss1, &TempBoss, sizeof(struct boss));
}

#undef threshold

void Boss_die_1(struct boss *Boss)
{ // Boss_die_1 and Boss_die_2 merged
	// together for size optimization.

	if (!Boss->Mode) {
		if (Boss->Height == 27) {
			Boss->Sprite = boss1_hurt_spr;
			Boss->Y += 10;
		} else { // Height == 30
			Boss->Sprite = boss2_hurt_spr;
			Boss->Y += 13;
		}

		Boss->Height = Boss->Height2 = 17;

		Boss->Mode = 60;
		Boss->Data0 = 0; // abort jump
		// enter passive hurt mode for some time, temporarly unkillable

		if ((--Boss->Life) == 0) {
			// Die
			Boss->Active = 0;
		}
	}
}

void Bowser_die(struct boss *Boss)
{
	if (!Boss->Mode) {
		if ((--Boss->Life) == 0) {
			Boss->Active = 0; // die
		}

		//		Boss->Sprite =

		Boss->Data1 = 0;
		Boss->Mode = 60;
	}
}

/*
void Boss_die_1(boss *Boss){

        if(!Boss->Mode){
                Boss->Sprite = boss1_hurt_spr;
                Boss->Height = Boss->Height2 = 17;
                Boss->Y += 10;
                Boss->Mode = 60;
                Boss->Data0 = 0;//abort jump
                //enter passive hurt mode for some time, temporarly unkillable

                if((--Boss->Life)==0){
                        //Die
                        Boss->Active = 0;

                };
        };

};

void Boss_die_2(boss *Boss){

        if(!Boss->Mode){
                Boss->Sprite = boss2_hurt_spr;
                Boss->Height = Boss->Height2 = 17;
                Boss->Y += 13;
                Boss->Mode = 60;
                Boss->Data0 = 0;//abort jump
                //enter passive hurt mode for some time, temporarly unkillable

                if((--Boss->Life)==0){
                        //Die
                        Boss->Active = 0;

                };
        };
};
*/

void Add_boss_shot(int16_t X, int16_t Y, char DirX, char DirY)
{
	int16_t C;

	// */Shot->
	// shot* Shot = &Enemyshots[C];

	/*	for(C=0;C<nr_of_enemyshots;C++){
                  if(!(Enemyshots[C].Mode)){

                          Enemyshots[C].Data0 = DirX;
                          Enemyshots[C].Data1 = DirY;
                          Enemyshots[C].Height = 12;
                          Enemyshots[C].Sprite = boss_shot_spr;
                          Enemyshots[C].X = X+DirX;
                          Enemyshots[C].Y = Y+DirY;
                          Enemyshots[C].Mode = 1;
                          (shot*)Enemyshots[C].Handler = Boss_shot_handler;
                          C = nr_of_enemyshots;
                  };
          };*/

	if ((C = Find_free_enemyshot()) >= 0) {
		struct shot *Shot = &Enemyshots[C];
		Shot->Data0 = DirX;
		Shot->Data1 = DirY;
		Shot->Height = 12;
		Shot->Sprite = boss_shot_spr;
		Shot->X = X + DirX;
		Shot->Y = Y + DirY;
		Shot->Mode = 1;
		Shot->Handler = Boss_shot_handler;
	}
}

void Boss_shot_handler(struct shot *Shot)
{
	Shot->X += Shot->Data0;
	Shot->Y += Shot->Data1;
}

void Add_big_fireball_shot(int16_t X, int16_t Y, char DirX, char DirY)
{
	int16_t C;
	/*
  for(C=0;C<nr_of_enemyshots;C++){
          if(!(Enemyshots[C].Mode)){
                  Enemyshots[C].X = X;
                  Enemyshots[C].Y = Y;
                  Enemyshots[C].Data0 = DirX;
                  Enemyshots[C].Data1 = DirY;
                  (shot*)Enemyshots[C].Handler = Boss_shot_handler;
                  Enemyshots[C].Height = 10;//temp
                  Enemyshots[C].Sprite = ( DirX>0 ?
  big_fireball_right_spr:big_fireball_left_spr );//temp Enemyshots[C].Mode = 1;
                  break;
          }
  };*/

	if ((C = Find_free_enemyshot()) >= 0) {
		struct shot *Shot = &Enemyshots[C];
		Shot->X = X;
		Shot->Y = Y;
		Shot->Data0 = DirX;
		Shot->Data1 = DirY;
		Shot->Handler = Boss_shot_handler;
		Shot->Height = 10; // temp
		Shot->Sprite = (DirX > 0 ? big_fireball_right_spr :
					   big_fireball_left_spr); // temp
		Shot->Mode = 1;
	}
}
