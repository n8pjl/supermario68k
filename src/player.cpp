#include "player.h"
#include "bounch.h"
#include "compat/utils.h"
#include "control.h"
#include "flying.h"
#include "items.h"
#include "level.h"
#include "render.h"
#include "scankeys.h"
#include "smallgames.h"
#include <stdint.h>
#include <string.h>

struct player Player;
struct saveplayer SavePlayer;

// smallshot Player_smallshots[nr_of_player_smallshots];
struct shot Player_smallshots[nr_of_player_smallshots];
// shot* Player_smallshots;
/*
const short Marioanimtab[8][11]={
//  0   1 	2		3		4		5 6 7 8 9
10 {  0, 16,  0, 16,  0,  0,  0, 32, 16,312,  0},//small left { 48, 64, 48, 64,
0,  0,  0, 80, 64,312,  0},//small right {
96,150,123,150,000,000,000,177,123,328,355},//large left
        {204,258,231,258,000,000,000,285,231,328,373},//large right
//	{128,155,209,182,209,155,236,263,182,468,495},//large left
//	{290,317,371,344,371,317,398,425,344,468,513},//large right
        {391,445,418,445,000,000,000,472,418,328,607},//large fireball left
        {499,553,526,553,000,000,000,580,526,328,625},//large fireball right
//	{495,522,576,549,576,522,603,630,549,468,0},//large fireball left
//	{657,684,738,711,738,684,765,792,711,468,0}//large fireball right
        {  0,  1,  2,  1,  0,  0,  0,  2,  2,  8,  3},//racoon tale and cap
index left {  4,  5,  6,  5,  0,  0,  0,  6,  6,  8,  7}//racoon tale and cap
index
};*/

/*
short Marioanimtab[6][11]={

        {  0, 16,  0, 16,  0, 16, 32, 48, 16,452,0},//small left
        { 64, 80, 64, 80, 64, 80, 96,112, 80,452,0},//small right
        {128,209,182,209,000,000,236,263,182,468,495},//large left
        {290,371,344,371,000,000,398,425,344,468,513},//large right
//	{128,155,209,182,209,155,236,263,182,468,495},//large left
//	{290,317,371,344,371,317,398,425,344,468,513},//large right
        {531,558,612,585,612,558,639,666,585,468,855},//large fireball left
        {693,720,774,747,774,720,801,828,747,468,873}//large fireball right
//	{495,522,576,549,576,522,603,630,549,468,0},//large fireball left
//	{657,684,738,711,738,684,765,792,711,468,0}//large fireball right
};*/

/*
void (*Interactive_tile_handlers[70])( short X, short Y)={//will be fewer when
finished...
        Dummy_func,Pipe_left_handler,Dummy_func,Dummy_func,Dummy_func,//91-95
        Dummy_func,Dummy_func,Dummy_func,Dummy_func,Dummy_func,//96-100
        Wood_block_pu2_handler,Dummy_func,Dummy_func,Dummy_func,Dummy_func,//101-105
        Pipe_right_handler,Dummy_func,Dummy_func,Dummy_func,Dummy_func,//106-110
        Pipe_up_left_handler,Pipe_up_right_handler,Dummy_func,Dummy_func,Dummy_func,//111-115
        Bonus_box_climbing_flower,Dummy_func,Dummy_func,Brick_hidden_coins,Bonus_box_coin,//116-120
        Break_brick,Dummy_func,Dummy_func,Bonus_box_powerup_1,Bonus_box_powerup_2,//121-125
        Bonus_box_star,Bonus_box_pow,Bonus_box_1up,Bonus_box_coin,Bonus_box_coins,//126-130
        Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,//131-135
        Bonus_box_coins,Bonus_box_powerup_2,Bonus_box_pow,Bonus_box_powerup_1,Bonus_box_1up,///136-140
        Note_handler,Dark_note_handler,Pu_2_note_handler,Dummy_func,Dummy_func,///141-145
        Dummy_func,Dummy_func,Dummy_func,Dummy_func,Dummy_func,///146-150
        Falling_block_handler,Pipe_down_left_handler,Pipe_down_right_handler,Dummy_func,Trigger_pow_item,//151-155
        Dummy_func,Dummy_func,Dummy_func,Dummy_func,Dummy_func//156-160
};*/
/*
void (*Interactive_tile_handlers[57])( short X, short Y)={//will be fewer when
finished... Pipe_left_handler,Dummy_func,//104-105
        Dummy_func,Wood_block_pu2_handler,Dummy_func,Dummy_func,Pipe_right_handler,//106-110
        Dummy_func,Dummy_func,Pipe_up_left_handler,Pipe_up_right_handler,Dummy_func,//111-115
        Bonus_box_climbing_flower,Dummy_func,Dummy_func,Brick_hidden_coins,Bonus_box_coin,//116-120
        Break_brick,Dummy_func,Dummy_func,Bonus_box_powerup_1,Bonus_box_powerup_2,//121-125
        Bonus_box_star,Bonus_box_pow,Bonus_box_1up,Bonus_box_coin,Bonus_box_coins,//126-130
        Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,Bonus_box_coins,//131-135
        Bonus_box_coins,Bonus_box_powerup_2,Bonus_box_pow,Bonus_box_powerup_1,Bonus_box_1up,///136-140
        Note_handler,Dark_note_handler,Pu_2_note_handler,Dummy_func,Dummy_func,///141-145
        Dummy_func,Dummy_func,Dummy_func,Dummy_func,Dummy_func,///146-150
        Falling_block_handler,Pipe_down_left_handler,Pipe_down_right_handler,Dummy_func,Trigger_pow_item,//151-155
        Dummy_func,Dummy_func,Dummy_func,Dummy_func,Dummy_func//156-160
};
*/
void (*Interactive_tile_handlers[47])(int16_t X, int16_t Y) = {
	// will be fewer when finished...
	Pipe_left_handler, // 113
	Wood_block_pu2_handler, Dummy_func, Dummy_func,
	Pipe_right_handler, // 114-117
	Pipe_up_left_handler, Pipe_up_right_handler, Dummy_func, // 118-
	Bonus_box_climbing_flower, Dummy_func, Dummy_func, Brick_hidden_coins,
	Bonus_box_coin, //
	Break_brick, Dummy_func, Dummy_func, Bonus_box_powerup_1,
	Bonus_box_powerup_2, //
	Bonus_box_star, Bonus_box_pow, Bonus_box_1up, Bonus_box_coin,
	Bonus_box_coins, //
	Bonus_box_coins, Bonus_box_coins, Bonus_box_coins, Bonus_box_coins,
	Bonus_box_coins, //
	Bonus_box_coins, Bonus_box_powerup_2, Bonus_box_pow,
	Bonus_box_powerup_1,
	Bonus_box_1up, //
	Note_handler, Dark_note_handler, Pu_2_note_handler, //
	//
	Falling_block_handler, Pipe_down_left_handler, Pipe_down_right_handler,
	Dummy_func, Trigger_pow_item, //
	Dummy_func, Dummy_func, Dummy_func, Dummy_func, Dummy_func,
	Dummy_func //
};

void (*Interactive_non_solid_tile_handlers[21])(int16_t X, int16_t Y) = {
	Quicksand_handler,
	Quicksand_handler,
	Ladder_handler,
	Water_surface_handler,
	Water_handler,
	Lava_handler,
	Hidden_bonusbox_pu_2_handler,
	Hidden_bonusbox_pow_handler,
	Hidden_note_handler,
	Hidden_dark_note_handler,
	Door_handler,
	Hidden_bonusbox_1_coin_handler,
	Hidden_bonusbox_8_coins_handler,
	Hidden_bonusbox_1_up_handler,
	Hidden_bonusbox_pu_1_handler,
	Dark_gray_magic_mode_handler,
	Dummy_func,
	Dummy_func,
	Dummy_func,
	Dummy_func,
	Dummy_func
};
/*
void (*Bonushandlers[15])( short X, short Y)={
        Player_collect_coin,Player_collect_flower,Handle_treasure_rand,Handle_treasure_star,Handle_treasure_whistle,
        Handle_treasure_hammer,Handle_treasure_pwing,Player_collect_flower,Player_collect_coin_water,Levelend_handler,
        Handle_treasure_cloud,Handle_treasure_anchor,Dummy_func,Dummy_func,Dummy_func
};
*/

void (*Bonushandlers[20])(int16_t X, int16_t Y) = { Player_collect_coin,
						    Player_collect_flower,
						    Handle_treasure_all,
						    Handle_treasure_all,
						    Handle_treasure_all,
						    Handle_treasure_all,
						    Handle_treasure_all,
						    Player_collect_flower,
						    Player_collect_coin_water,
						    Levelend_handler,
						    Handle_treasure_all,
						    Handle_treasure_all,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func,
						    Dummy_func };

// unsigned short Mariosprites[];		//will be fetched from graphics
// file in Loadgfx() unsigned short Mariomasks[];			//will
// be fetched from graphics file in Loadgfx()

void Playerinit()
{
	memset(&Player, 0, sizeof(struct player));
	memset(&SavePlayer, 0, sizeof(struct saveplayer));
	SavePlayer.Life = 1; // 2;
	SavePlayer.Lives = 5;

	Player.Face = 1;

	Player.Offset =
		1; // This fixes the bug where player sometimes moves unintentionally in
	// map when starting a new game 2nd+ time

	// Player.IsJumping = 0;
	// Player.IsInWater = 0;
	// Player.IsFalling = 0;
	// Player.Coins = 0;

	// Player.SlopeAbove = 0;

	Player.Height2 = Player.Height = 16; // 27;
	/*	Player.X = 16;//60;	//test
          Player.Y = 5;	//test
  */
	// Player.Score=0;

	// Player.Attribs = 0b00000000;

	SavePlayer.Maskbase = SavePlayer.Spritebase = 1; // 3;
	Player.Blit = 0xffff;
	// Player.Spritenr = 0;
	// Player.SpriteOffset = 0;
	// Player.PrevCompX = 0;
	// Player.PrevCompY = 0;
	// Player.Offset = 0;
	// Player.Sprite =
	// large_right_sprites;//small_right_sprites;//stand_left_large_sprite;//stand_left_small_sprite;
	// //test Player.Mask =
	// large_right_masks;//small_right_masks;//stand_left_large_mask;//stand_left_small_mask;
	// //test Player.Sprite = Mariosprites; Player.Sprite = Mariomasks;
	// Player.Sprite.Height = 27;

	//	Player.Rightmost = 15;	//test
	//	Player.Leftmost = 0;	//test

	/*	Player.Fallspeed = 2;
          Player.Jumpspeed = 2;
          Player.Walkspeed2 = PLAYER_WALKSPEED;
          Player.Max_jumpheight = jumpheight;
          */

//	SavePlayer.Score = 32000;

// Player.Passified=0;
// Player.MapX=1*16;
// Player.MapY=1*16;
/*
for(Player.CurrCard=0;Player.CurrCard<20;Player.CurrCard++)
        Player.Itemlist[(short)Player.CurrCard]=0;//1
*/
/*short C;
for(C=0;C<20;C++)
        Player.Itemlist[C]=0;//1

Player.Itemlist[0]=6;
*/
/*
Player.Itemlist[1]=5;
Player.Itemlist[2]=7;
Player.Itemlist[3]=4;
Player.Itemlist[4]=8;
Player.Itemlist[19]=3;
*/
#ifdef debug
	SavePlayer.Itemlist[0] = 1; // debug mode
	SavePlayer.Itemlist[1] = 2;
	SavePlayer.Itemlist[2] = 3;
	SavePlayer.Itemlist[3] = 4;
	SavePlayer.Itemlist[4] = 5;
	SavePlayer.Itemlist[5] = 6;
	SavePlayer.Itemlist[6] = 7;
	SavePlayer.Itemlist[7] = 8;
	SavePlayer.Itemlist[8] = 9;
#endif

	//	Player.Itemlist[9]=8;
	// Player.Curr_item=0;

	// Player.Itemlist[]={0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0};

	// Player.Cards[0]=Player.Cards[1]=Player.Cards[2]=0;
	// Player.CurrCard = 0;
	// for testing

	if (Exit == 99) {
		// memset(&SavePlayer.Itemlist,7,itemlist_length);//fill with p-wings if
		// bowser defeated
		int16_t C;
		for (C = 0; C < itemlist_length; C++) {
			SavePlayer.Itemlist[C] = 7;
		}
	}

	//	memset(CardGame,0,18);//reset cardgame: this is neccessarry when
	// starting a new game a second time without exiting the program
	int16_t C;
	for (C = 0; C < 18; C++) {
		*(CardGame + C) = 0;
	}
}

// the following functions can be declared as inline for minor speed increase,
// but size increases
void Set_player_attribs_right(uint8_t *Attribs)
{ // sets the player position attributes for the
	// current direction (right)
	uint8_t Val = 16, Temp;
	int16_t C;
	int16_t Temp2;

	Player.SlopeDownUnder = 0;

	// Attribs[0] = 15-(Player.X+15+15)%16;		//distance to next tile
	// right
	Attribs[0] = 15 - ((Player.X + 15 + 15) &
			   0x000f); // same as above, but optimized
	Attribs[1] =
		Get_tile(Player.X + 15 + 16,
			 Player.Y + Player.Height - 1); // next tile right low
	Attribs[2] = Get_tile(Player.X + 15 + 16,
			      Player.Y + 15); // next tile right middle
	Attribs[3] =
		Get_tile(Player.X + 15 + 16, Player.Y); // next tile right high
	Attribs[4] = Get_tile(Player.X + 15,
			      Player.Y + Player.Height - 1); // 15   ,  -1
	Temp = Get_tile(Player.X + 18,
			Player.Y + Player.Height - 1); // 18   ,  -1

	/*if((Temp>=slope_under_left_low) && (Temp<=slope_under_left_high))
          Attribs[4] = Temp;*/

	if (((Temp >= slope_under_left_low) &&
	     (Temp <= slope_under_left_high)) ||
	    ((Attribs[4] >= slope_under_left_low) &&
	     (Attribs[4] <= slope_under_left_high))) {
		Player.Walkspeed = PLAYER_WALKSPEED;
		Attribs[4] = slope_under_left;
	}

	Temp = Get_tile(Player.X, Player.Y + Player.Height - 1);

	if ((Temp >= slope_under_right_low) &&
	    (Temp <= slope_under_right_high)) {
		Player.SlopeDownUnder = 1;
	} else {
		Temp = Get_tile(Player.X + 8, Player.Y + Player.Height - 1 + 8);
		if ((Temp >= slope_under_right_low) &&
		    (Temp <= slope_under_right_high)) {
			Player.SlopeDownUnder = 1;
		}
	}

	// Player.Test = 1;//test

	// if((Val>Attribs[0]) && !((Attribs[1]>=slope_under_left_low) &&
	// (Attribs[1]<=slope_under_left_high) ) &&
	// !((Attribs[4]>=slope_under_left_low) && (Attribs[4]<=slope_under_left_high)
	// )){

	/*for(C=1;C<=3;C++){//optimize
          if( (Attribs[C]>=solid_low) && (Val>Attribs[0]) &&
  !((Attribs[1]>=slope_under_left_low) && (Attribs[1]<=slope_under_left_high) )
  && !((Attribs[4]>=slope_under_left_low) && (Attribs[4]<=slope_under_left_high)
  )  )// && (Temp<=slope_under_left_low) && (Temp>=slope_under_left_high))
                  Val=Attribs[0];//is Val>Attribs[0] really necessarry?
  };*/

	for (C = 1; C <= 3; C++) { // optimize
		if ((Attribs[C] >= solid_low) && (Attribs[C] <= solid_high) &&
		    (Val > Attribs[0]))
			Val = Attribs[0]; // is Val>Attribs[0] really necessarry?
	}

	Temp = Get_tile(Player.X + 16 + 1, Player.Y);

	if ((Temp >= slope_over_left_low) && (Temp <= slope_over_left_high)) {
		// Val = ( ((Player.Y-1)%16) - ((Player.X+15)%16) );
		Val = (((Player.Y - 1) & 0x000f) - ((Player.X + 15) & 0x000f));

		if (Player.Y % 16 == 0)
			Val = 0;
	}

	// test if blocked by flying platforms
	for (C = 0; C < Leveldata.Nr_of_flying_platforms; C++) {
		if (Flying_platforms[C].Active &&
		    ((Player.Y + Player.Height - 1) >= Flying_platforms[C].Y) &&
		    (Player.Y <= (Flying_platforms[C].Y + 16))) {
			Temp2 = ((Flying_platforms[C].X) -
				 (Player.X + 15)); //-1

			if ((Temp2 >= 0) && (Temp2 < Val)) {
				Val = Temp2;
			}
		}
	}

	Attribs[5] = Val;

	/*
  //this version is working if slope_under_blah_blah is defined in the non_solid
  area Attribs[0] = 15-(Player.X+15+15)%16;		//distance to next tile
  right Attribs[1] = Get_tile(Player.X+15+16,Player.Y+Player.Height-2);	//next
  tile right low Attribs[2] = Get_tile(Player.X+15+16,Player.Y+15);
  //next tile right middle Attribs[3] = Get_tile(Player.X+15+16,Player.Y);
  //next tile right high Attribs[4] =
  Get_tile(Player.X+15,Player.Y+Player.Height-1); Temp =
  Get_tile(Player.X+18,Player.Y+Player.Height-1);

          if((Temp>=slope_under_left_low) && (Temp<=slope_under_left_high))
                  Attribs[4] = Temp;

          for(C=1;C<=3;C++){//optimize
                  if( (Attribs[C]>=solid_low) && (Val>Attribs[0]) )
                          Val=Attribs[0];
          };

          Attribs[5] = Val;

          */
	//----------------------------------------------------------------

	/*
   */
}

void Set_player_attribs_left(uint8_t *Attribs)
{
	uint8_t Val = 16, Temp; //,Temp2,Temp3,FL,FM,FH
	int16_t C;
	int16_t Temp2;

	Player.SlopeDownUnder = 0;

	// Attribs[0] = (Player.X)%16;				//distance to
	// next tile left
	Attribs[0] = (Player.X) & 0x000f; // same as above, but optimized
	Attribs[1] =
		Get_tile(Player.X - 16,
			 Player.Y + Player.Height - 1); // next tile left low
	Attribs[2] =
		Get_tile(Player.X - 16, Player.Y + 15); // next tile left middle
	Attribs[3] = Get_tile(Player.X - 16, Player.Y); // next tile left high
	Attribs[4] = Get_tile(Player.X, Player.Y + Player.Height - 1);
	Temp = Get_tile(Player.X - 3, Player.Y + Player.Height - 1);

	if ((Temp >= slope_under_right_low) && (Temp <= slope_under_right_high))
		Attribs[4] = Temp;

	if (((Temp >= slope_under_right_low) &&
	     (Temp <= slope_under_right_high)) ||
	    ((Attribs[4] >= slope_under_right_low) &&
	     (Attribs[4] <= slope_under_right_high))) {
		Player.Walkspeed = PLAYER_WALKSPEED;
		Attribs[4] = slope_under_right;
	}

	Temp = Get_tile(Player.X + 15, Player.Y + Player.Height - 1);

	if ((Temp >= slope_under_left_low) && (Temp <= slope_under_left_high)) {
		Player.SlopeDownUnder = 1;
	} else {
		Temp = Get_tile(Player.X + 8, Player.Y + Player.Height - 1 + 8);
		if ((Temp >= slope_under_left_low) &&
		    (Temp <= slope_under_left_high)) {
			Player.SlopeDownUnder = 1;
		}
	}

	/*for(C=1;C<=3;C++){//optimize
          if( (Attribs[C]>=solid_low) && (Val>Attribs[0]) &&
  !((Attribs[1]>=slope_under_right_low) && (Attribs[1]<=slope_under_right_high)
  ) && !( (Attribs[4]>=slope_under_left_low) &&
  (Attribs[4]<=slope_under_left_high) ) )//&& (Temp<=slope_under_left_low) &&
  (Temp>=slope_under_left_high)) Val=Attribs[0];//is Val>Attribs[0] really
  necessarry?
  };*/

	for (C = 1; C <= 3; C++) { // optimize
		if ((Attribs[C] >= solid_low) && (Attribs[C] <= solid_high) &&
		    (Val > Attribs[0]))
			Val = Attribs[0]; // is Val>Attribs[0] really necessarry?
	}

	Temp = Get_tile(Player.X - 2, Player.Y);

	//	unsigned char Temp2 = Get_tile(Player.X-2,Player.Y-2);

	if (((Temp >= slope_over_right_low) &&
	     (Temp <= slope_over_right_high))) {
		// Val = ( 16-((Player.Y-1)%16) - ((Player.X)%16) );//Player.SlopeAbove = 1;
		Val = (16 - (((Player.Y - 1) & 0x000f)) -
		       ((Player.X) & 0x000f));
		//		Val = ( 16-(((Player.Y)&0x000f)) - ((Player.X-2)&0x000f) );
		/*	if(Val>1)
                    Val--;*/
		/*	if(Val<0)
                    Val = 0;*/

		if (Player.Y % 16 == 0)
			Val = 0;

	} else {
		/*
            Temp = Get_tile(Player.X-2,Player.Y);
            if( (Temp >= slope_over_right_low) && (Temp <=
       slope_over_right_high) ){//(Temp>=solid_low)
                    //Val = ( 16-((Player.Y-1)%16) - ((Player.X)%16)
       );//Player.SlopeAbove = 1; Val = 0;//( 16-(((Player.Y-1)&0x000f)) -
       ((Player.X)&0x000f) );
            }

            */
	}

	/*	Temp = Get_tile(Player.X-4,Player.Y);

          if( (Temp >= slope_over_right_low) && (Temp <= slope_over_right_high)
     && (Player.Y%16==0) ) Val = 0;*/

	// test if blocked by flying platforms
	for (C = 0; C < Leveldata.Nr_of_flying_platforms; C++) {
		if (Flying_platforms[C].Active &&
		    ((Player.Y + Player.Height - 1) >= Flying_platforms[C].Y) &&
		    (Player.Y <= (Flying_platforms[C].Y + 16))) {
			Temp2 = ((Player.X) -
				 (Flying_platforms[C].X +
				  Flying_platforms[C].Width * 16)); //-1

			if ((Temp2 >= 0) && (Temp2 < Val)) {
				Val = Temp2;
			}
		}
	}

	//	Player.Test = Val;

	Attribs[5] = Val;
}

void Set_player_attribs_up(uint8_t *Attribs)
{
	uint8_t Val = 16;
	int16_t C;
	int16_t Temp;

	// Attribs[0] = (Player.Y)%16;
	// //distance to next tile up
	Attribs[0] = (Player.Y) & 0x000f; // same as above, but optimized
	Attribs[1] = Get_tile(Player.X, Player.Y - 15); // 16
	// //next tile up left
	Attribs[2] = Get_tile(Player.X + 15, Player.Y - 15); // 16
	// //next tile up right
	Attribs[3] = Get_tile(Player.X, Player.Y - 2);
	Attribs[4] = Get_tile(Player.X + 15, Player.Y - 2);

	for (C = 1; C <= 2; C++) {
		if (Attribs[C] >= solid_low &&
		    Val > Attribs[0]) // is Val>Attribs[0] really necessarry?
			Val = Attribs[0];
	}

	Player.SlopeAbove = 0;

	if ((Attribs[4] >= slope_over_left_low) &&
	    (Attribs[4] <= slope_over_left_high)) {
		// Val = ( ((Player.Y-1)%16) - ((Player.X+15)%16) );
		Val = (((Player.Y - 1) & 0x000f) - ((Player.X + 15) & 0x000f));
		if (Player.X % 16 == 0)
			Val = 0;
		Player.SlopeAbove = 1;
	}

	//	unsigned char Tile = Get_tile(Player.X,Player.Y-1);

	if ((Attribs[3] >= slope_over_right_low) &&
	    (Attribs[3] <= slope_over_right_high)) {
		// Val = ( ((Player.Y-1)%16) - (15-((Player.X)%16)) );
		Val = (((Player.Y - 1) & 0x000f) -
		       (15 - ((Player.X) & 0x000f)));
		if (Player.X % 16 == 0)
			Val = 0;
		Player.SlopeAbove = 1;
	}

	/*unsigned char Tile = Get_tile(Player.X+2,Player.Y-2);
  if( (Tile >= slope_over_right_low) && (Tile <= slope_over_right_high) ){
          Val = 0;
  }*/

	// test if blocked by flying platforms
	for (C = 0; C < Leveldata.Nr_of_flying_platforms; C++) {
		if (Flying_platforms[C].Active &&
		    ((Player.X + 15) >= Flying_platforms[C].X) &&
		    (Player.X <= (Flying_platforms[C].X +
				  Flying_platforms[C].Width * 16))) {
			Temp = ((Player.Y) -
				(Flying_platforms[C].Y +
				 16 /*+Flying_platforms[C].Data3*/)); //-1

			if (((Temp) >= 0) && (Temp <= (int16_t)Val)) {
				Val = (uint8_t)Temp;

				if (((Temp - Flying_platforms[C].Data3) <=
				     Player.Jumpspeed) /*&& (Flying_platforms[C].Data3>0)*/) {
					Flying_platforms[C].PlayerOn = 1;
				}
			}
		}
	}

	Attribs[5] = Val;
}

void Set_player_attribs_down(uint8_t *Attribs)
{
	uint8_t Val = 16;
	int16_t C;
	int16_t Temp;

	Attribs[0] = 15 - (Player.Y + Player.Height - 1) %
				  16; // distance to next tile down
	Attribs[1] = Get_tile(Player.X, Player.Y + Player.Height +
						15); // next tile down left
	Attribs[2] =
		Get_tile(Player.X + 15,
			 Player.Y + Player.Height + 15); // next tile down right
	Attribs[3] = Get_tile(Player.X, Player.Y + Player.Height);
	Attribs[4] = Get_tile(Player.X + 15, Player.Y + Player.Height);

	// Attribs[0] = (15-((Player.Y+Player.Height-1)&0x000f));//same as above, but
	// optimized. INTRODUCES BUG!!!

	if (Player.Y < (-Player.Height +
			1)) { // adjust distance to next tile down when player
		// goes outside level upwards
		Attribs[0] -= 16;
	}

	/*
  Attribs[1] = Get_tile(Player.X,Player.Y+Player.Height+15);		//next
tile down left Attribs[2] = Get_tile(Player.X+15,Player.Y+Player.Height+15);
//next tile down right Attribs[3] = Get_tile(Player.X,Player.Y+Player.Height);
Attribs[4] = Get_tile(Player.X+15,Player.Y+Player.Height);
*/
	if (Player.IsInQsand || Player.IsClimbing || Player.IsInWater) {
		Val = 0;
	} else {
		for (C = 1; C <= 2; C++) {
			if ((Attribs[C] >= solid_down_low) &&
			    (Val >
			     Attribs[0])) { // is Val>Attribs[0] really necessarry?
				Val = Attribs[0];
			}
		}

		if ((Attribs[4] >= slope_under_left_low) &&
		    (Attribs[4] <= slope_under_left_high)) {
			// Val = ( (15-((Player.X+15)%16))-((Player.Y+Player.Height)%16) );
			Val = ((15 - ((Player.X + 15) & 0x000f)) -
			       ((Player.Y + Player.Height) & 0x000f));
		}

		if ((Attribs[3] >= slope_under_right_low) &&
		    (Attribs[3] <= slope_under_right_high)) {
			// Val = ( ((Player.X)%16)-((Player.Y+Player.Height)%16) );
			Val = (((Player.X) & 0x000f) -
			       ((Player.Y + Player.Height) & 0x000f));
		}

		// test if blocked by flying platforms
		for (C = 0; C < Leveldata.Nr_of_flying_platforms; C++) {
			if (Flying_platforms[C].Active &&
			    ((Player.X + 15) >= Flying_platforms[C].X) &&
			    (Player.X <= (Flying_platforms[C].X +
					  Flying_platforms[C].Width * 16))) {
				Temp = ((Flying_platforms[C].Y) -
					(Player.Y + Player.Height - 1)); //-1

				if ((Temp >= 0) && (Temp <= (int16_t)Val)) {
					Val = (uint8_t)Temp;

					if (((Temp +
					      Flying_platforms[C].Data3) <=
					     Player.Fallspeed)) {
						Flying_platforms[C].PlayerOn =
							3;
					}
				}
			}
		}
	}

	Attribs[5] = Val;
}

void Handleplayer()
{
	uint8_t Temp, C, D, Runflag; // runflag is fly code
	static char Anim_step;

	uint8_t Attribs[6], Attribs2[6]; // common temporary storage

	if ((Player.Behind_fg >
	     0) /*&& (!(SavePlayer.Attribs & 0b00010000))*/) {
		Player.Behind_fg--;
	}

	Runflag = 0; // fly code

	if (SavePlayer.Spritenr == 10) {
		Player.Height = Player.Height2 = 27;
		SavePlayer.Spritenr = 1;
		Player.Y -= 9;
	}

	if (Player.Racoonspin) {
		Player.Racoonspin--;
		if (Player.Racoonspin == 5) {
			Player.Face = -Player.Face;
			SavePlayer.Maskbase = SavePlayer.Spritebase =
				((Player.Face == 1) ? 3 : 2);
		}

		// test if hitting reactive tiles
		for (D = 0; D <= 9; D += 9) {
			for (C = 0; C <= 6; C += 6) {
				Temp = Get_tile(
					Player.X +
						(Player.Face == 1 ? -9 : 15) +
						D,
					Player.Y + 17 + C);

				if ((Temp >= solid_interactive_bounch_low) &&
				    (Temp <= solid_interactive_bounch_high))
					Interactive_tile_handlers
						[Temp - solid_interactive_low](
							Player.X +
								(Player.Face == 1 ?
									 -9 :
									 15) +
								D,
							Player.Y + 17 + C);
			}
		}
	}

	if (Player.Immortal) {
		// flashing mario
		if (!(--Player.Immortal)) {
			SavePlayer.Attribs =
				SavePlayer.Attribs &
				0b01111111; // terminate super mode/star mode
		}
	}

	if (Keystate.right || (Player.Xoffset > 0)) {
		// animate walksprite
		if (SavePlayer.Life == 1) {
			SavePlayer.Maskbase = SavePlayer.Spritebase = 1;
		} else {
			if (SavePlayer.Life == 2) {
				SavePlayer.Maskbase = SavePlayer.Spritebase = 3;
			} else {
				if ((SavePlayer.Attribs & 0b01000000)) {
					SavePlayer.Maskbase = 3;
					SavePlayer.Spritebase = 5; // fire mario
				} else {
					SavePlayer.Maskbase =
						SavePlayer.Spritebase =
							3; // racoon suit
				}
			}
		}

		if ((Anim_step++) == anim_lenght) {
			if ((++SavePlayer.Spritenr) >= 4) { // 6
				SavePlayer.Spritenr = 0;
			}

			Anim_step = 0;
		}

		Player.Face = 1;

		if (Player.X < (Leveldata.Border_right - 16)) { // not at end
			Set_player_attribs_right(Attribs);

			if (Attribs[5] >=
			    Player.Walkspeed) { // enough free space to move with
				// primary speed (for example run)
				Player.X = Player.X + Player.Walkspeed;

				// fly code
				if (Player.Walkspeed == PLAYER_RUNSPEED) {
					Runflag++;
				}

				if (Attribs[4] == slope_under_left) {
					Player.Y -= Player.Walkspeed;
				} else {
					if (Player.SlopeDownUnder) {
						// New: V 1.04 Fixed bug where player died on contact with flying
						// platforms when he was positioned on slope tiles
						Set_player_attribs_down(
							Attribs2);
						Player.Y += min(
							Player.Walkspeed,
							Attribs2[5] /*Free_down(Player.X,Player.Y+Player.Height)*/);
					}
				}

			} else {
				if (Attribs[5] >= Player.Walkspeed2) {
					Player.X =
						Player.X +
						Player.Walkspeed2; // enough free space to move with
					// secondary speed (for example walk)

					if (Attribs[4] == slope_under_left)
						Player.Y -= Player.Walkspeed2;

				} else { // not free right
					// interactive right ?
					Temp = Get_tile(Player.X + 16,
							Player.Y);
					if ((Temp >=
					     solid_interactive_left_right_low) &&
					    (Temp <=
					     solid_interactive_right_high))
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X + 16,
								Player.Y);

					Temp = Get_tile(
						Player.X + 16,
						Player.Y + Player.Height - 1);
					if ((Temp >=
					     solid_interactive_left_right_low) &&
					    (Temp <=
					     solid_interactive_right_high))
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X + 16,
								Player.Y +
									Player.Height -
									1);
				}
			}
		}

	} // end of right
	//		else{//qbqbqbqbqb
	if (Keystate.left || (Player.Xoffset < 0)) {
		// animate walksprite

		if (SavePlayer.Life == 1) {
			SavePlayer.Maskbase = SavePlayer.Spritebase = 0;
		} else {
			if (SavePlayer.Life == 2) {
				SavePlayer.Maskbase = SavePlayer.Spritebase = 2;
			} else {
				if ((SavePlayer.Attribs & 0b01000000)) {
					SavePlayer.Maskbase = 2;
					SavePlayer.Spritebase = 4; // fire mario
				} else {
					SavePlayer.Maskbase =
						SavePlayer.Spritebase =
							2; // racoon suit
				}
			}
		}

		if ((Anim_step++) == anim_lenght) {
			if ((++SavePlayer.Spritenr) >= 4) { // 6
				SavePlayer.Spritenr = 0;
			}

			Anim_step = 0;
		}

		Player.Face = -1;
		if (Player.X >= (Leveldata.Border_left +
				 (uint8_t)Player.Walkspeed)) { // bug?
			Set_player_attribs_left(Attribs);

			if (Attribs[5] >=
			    Player.Walkspeed) { // enough free space
				Player.X = Player.X - Player.Walkspeed;

				if (Attribs[4] >= slope_under_right) {
					Player.Y -= Player.Walkspeed;
				} else {
					if (Player.SlopeDownUnder) {
						// New: V 1.04 Fixed bug where player died on contact with flying
						// platforms when he was positioned on slope tiles
						Set_player_attribs_down(
							Attribs2);
						Player.Y += min(
							Player.Walkspeed,
							Attribs2[5] /*Free_down(Player.X,Player.Y+Player.Height)*/);
					}
				}
				/*else{//b111 bugfix attempt, didn't work...
                if( (Attribs[4]>=slope_under_left_low) ){
                        Player.Y += Player.Walkspeed;
                };
        }//b111*/

				// fly code
				if (Player.Walkspeed == PLAYER_RUNSPEED) {
					Runflag++;
				}

			} else {
				if (Attribs[5] >= Player.Walkspeed2) {
					Player.X = Player.X - Player.Walkspeed2;
					// if( ((Attribs[4] >= slope_under_right_low) && (Attribs[4] <=
					// slope_under_right_high)) )
					if (Attribs[4] >= slope_under_right)
						Player.Y -= Player.Walkspeed2;
				} else {
					// interactive left ?

					Temp = Get_tile(Player.X - 1, Player.Y);
					if ((Temp >=
					     solid_interactive_left_low) &&
					    (Temp <=
					     solid_interactive_left_right_high))
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X - 1,
								Player.Y);

					Temp = Get_tile(
						Player.X - 1,
						Player.Y + Player.Height - 1);
					if ((Temp >=
					     solid_interactive_left_low) &&
					    (Temp <=
					     solid_interactive_right_high))
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X - 1,
								Player.Y +
									Player.Height -
									1);
				}
			}
		}

	} // end of left
	/*else{


          Anim_step=0;//-1;
          Player.Offset = 0;

          if( !Player.Racoonspin )
                  Player.Spritenr=0;

  };*/

	if ((!Keystate.left) && (!Keystate.right) && (Player.Xoffset == 0)) {
		Anim_step = 0; //-1;
		Player.Offset = 0;

		if (!Player.Racoonspin)
			SavePlayer.Spritenr = 0;
	}

	//		};

	// Player.Xoffset = 0;

	// Debug stuff
	/*if(Free_down(Player.X,Player.Y+Player.Height)>0){
          Player.Test = 1;
  }
  else{
          Player.Test = 0;
  }*/

	if (Player.Xoffset) {
		if (Player.Xoffset > 0) {
			Player.Xoffset--;
		} else {
			Player.Xoffset++;
		}
	}

	if (Keystate.run) {
		Player.Walkspeed = PLAYER_RUNSPEED;
		Player.Walkspeed2 = PLAYER_WALKSPEED;

		/*if( (Player.Attribs & 0b01000000) ){//able to throw fireballs ?
            if( !Previous_keystate.Run ){
                    Player_add_fireball();
            };
    };*/

		if (!Previous_keystate.run) {
			if ((SavePlayer.Attribs &
			     0b01000000)) { // able to throw fireballs ?
				Player_add_fireball();
			}

			if ((SavePlayer.Attribs &
			     0b00100000)) { // racoon suit ?
				Player.Face = -Player.Face;
				SavePlayer.Spritenr = 2;

				Player.Racoonspin = 9;

				SavePlayer.Maskbase = SavePlayer.Spritebase =
					((Player.Face == 1) ? 3 : 2);

				/*if(Player.Face==1){
                Player.Spritebase = Player.Maskbase = 3;
        }
        else{
                Player.Spritebase = Player.Maskbase = 2;
        };*/
			}
		}
	} else {
		Player.Walkspeed = PLAYER_WALKSPEED;
		Player.Walkspeed2 = PLAYER_WALKSPEED;
	}

	if ((Keystate.jump || (Player.Yoffset < 0)) && (!Player.IsClimbing)) {
		// fly code
		if ((SavePlayer.Attribs & 0b00100000) &&
		    (/*Player.Runcount*/ Player.Flycount >=
		     flycondition)) { // racoon suit (and allowed to fly)?

			Player.IsJumping = jumpheight; // flylength;
			// Player.Falling=0;
			Player.Jumpspeed = flyspeed;
			Player.RacoonTaleAnim = 1;
		}

		Set_player_attribs_up(Attribs);
		if (Attribs[5] > 0) { // free up?
			if (Player.IsJumping > 0) { // allready jumping

				Player.Y -=
					(Attribs[5] > Player.Jumpspeed) ?
						(Player.Jumpspeed) :
						(Attribs[5]); // the smallest one
				SavePlayer.Spritenr = 7;
				Player.IsJumping =
					Player.IsJumping - Player.Jumpspeed;
				if (Player.IsJumping < 0)
					Player.IsJumping = 0;
			} else { // NOT allready jumping
				Set_player_attribs_down(Attribs2);
				if ((Attribs2[5] == 0) &&
				    (!Previous_keystate.jump)) { // not free down
					Player.Y -=
						(Attribs[5] >
						 Player.Jumpspeed) ?
							(Player.Jumpspeed) :
							(Attribs[5]); // the largest one
					SavePlayer.Spritenr = 7;
					Player.IsJumping =
						Player.Max_jumpheight;
				}
			}
		} else { // NOT free up
			// test if player is on the very edge of solid tile
			// if so, adjust x position and continue jump
			// if not, then terminate jump

			// if( (!(Player.SlopeAbove)) && (Attribs[1]>=solid_low) &&
			// (Attribs[2]<solid_low) && ((16-(Player.X%16))<=6) && !(Keystate.Left)
			// ){//adjust right
			if ((!(Player.SlopeAbove)) &&
			    (Attribs[1] >= solid_low) &&
			    (Attribs[2] < solid_low) &&
			    ((16 - (Player.X & 0x000f)) <= 6) &&
			    !(Keystate.left)) { // adjust right
				// Player.X += (16-(Player.X%16));
				Player.X += (16 - (Player.X & 0x000f));
			} else {
				// if( (!(Player.SlopeAbove)) && (Attribs[1]<solid_low) &&
				// (Attribs[2]>=solid_low) && ((Player.X%16)<=6) && !(Keystate.Right)
				// ){//adjust left
				if ((!(Player.SlopeAbove)) &&
				    (Attribs[1] < solid_low) &&
				    (Attribs[2] >= solid_low) &&
				    ((Player.X & 0x000f) <= 6) &&
				    !(Keystate.right)) { // adjust left
					// Player.X -= (Player.X%16);
					Player.X -= (Player.X & 0x000f);
				} else {
					// terminate jump and
					// test if interactive tile above
					Player.Yoffset = 0;

					Temp = Get_tile(Player.X, Player.Y - 1);
					if ((Temp >=
					     solid_interactive_up_low) &&
					    (Temp <=
					     solid_interactive_up_down_high)) {
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X,
								Player.Y - 1);
						Add_bounching_tile(
							Player.X, Player.Y - 1);
					}
					Temp = Get_tile(Player.X + 16,
							Player.Y - 1);
					if ((Temp >=
					     solid_interactive_up_low) &&
					    (Temp <=
					     solid_interactive_up_down_high)) {
						Interactive_tile_handlers
							[Temp -
							 solid_interactive_low](
								Player.X + 16,
								Player.Y - 1);
						Add_bounching_tile(
							Player.X + 16,
							Player.Y - 1);
					}
					if (Player.Flycount &&
					    (SavePlayer.Attribs & 0b00100000)) {
						SavePlayer.Spritenr = 7;
					} else {
						Player.IsJumping = 0;
					}
				}
			}
		}

	} else { // not trying to jump
		Player.IsJumping = 0;
	}

	Set_player_attribs_down(Attribs);
	if (Attribs[5] > 0) { // free down?

		Player.White_magic_box_counter = 0;

		if (!Player.IsJumping) { // fall

			if (Keystate.jump &&
			    (SavePlayer.Attribs & 0b00100000)) {
				Player.Fallspeed = 2;
				Player.RacoonTaleAnim = 1;
			}

			Player.Y += (Attribs[5] >= Player.Fallspeed) ?
					    (Player.Fallspeed) :
					    (Attribs[5]); // the smallest one
			// Player.IsFalling++;
			Player.IsFalling = Player.IsFalling + Player.Fallspeed;
			SavePlayer.Spritenr = 8; // fallsprite

			if (Player.Y >= (Leveldata.Height * 16)) {
				Player_die_hard();
			}
		}

	} else { // not free down
		// Player.Sprite = standingsprite;
		// Player.Mask = standingmask;
		Player.IsFalling = 0;
		Player.Max_jumpheight = jumpheight;
		// Player.Yoffset = 0;

		// fly code
		Runflag++;

		int16_t White_box = 0;
		// test if interactive tile below
		Temp = Get_tile(Player.X, Player.Y + Player.Height);
		if ((Temp >= solid_interactive_up_down_low) &&
		    (Temp <= solid_interactive_down_high)) {
			Interactive_tile_handlers[Temp - solid_interactive_low](
				Player.X, Player.Y + Player.Height);
		} else {
			if ((Temp >= solid_down_interactive_low) &&
			    (Temp <= solid_down_interactive_high)) {
				White_magic_box_handler(
					Player.X, Player.Y + Player.Height);
				White_box++;
			}
		}

		Temp = Get_tile(Player.X + 15, Player.Y + Player.Height);
		if ((Temp >= solid_interactive_up_down_low) &&
		    (Temp <= solid_interactive_down_high)) {
			Interactive_tile_handlers[Temp - solid_interactive_low](
				Player.X + 15, Player.Y + Player.Height);
		} else {
			if ((Temp >= solid_down_interactive_low) &&
			    (Temp <= solid_down_interactive_high)) {
				White_magic_box_handler(
					Player.X, Player.Y + Player.Height);
				White_box++;
			}
		}

		if (!White_box) {
			Player.White_magic_box_counter = 0;
		}

		if (Keystate.down && !Keystate.right && !Keystate.left &&
		    (SavePlayer.Life >= 2) && !Player.IsClimbing &&
		    !(Player.IsInWater &&
		      Free_down(Player.X, Player.Y + Player.Height))) {
			// duck
			Player.Height = Player.Height2 = 18;
			SavePlayer.Spritenr = 10;
			Player.Y += 9;
		}
	}

	// set player.jumpspeed and fallspeed (numbers might change, optimization is
	// coming
	if (!Player.Yoffset) {
		// Temp = Player.Max_jumpheight/4;
		Temp = (Player.Max_jumpheight >> 2);
		if ((Player.IsJumping >= (3 * Temp)) ||
		    (Player.IsFalling >= (3 * Temp))) {
			Player.Jumpspeed = 8;
			Player.Fallspeed = 8;
		} else {
			if (((Player.IsJumping < (3 * Temp)) &&
			     ((Player.IsJumping >= (2 * Temp)))) ||
			    ((Player.IsFalling >= (2 * Temp)) &&
			     (Player.IsFalling < (3 * Temp)))) {
				Player.Jumpspeed = 6;
				Player.Fallspeed = 6;
			} else {
				if (((Player.IsJumping < (2 * Temp)) &&
				     ((Player.IsJumping >= (Temp)))) ||
				    ((Player.IsFalling >= Temp) &&
				     (Player.IsFalling < (2 * Temp)))) {
					Player.Jumpspeed = 4;
					Player.Fallspeed = 4;
				} else {
					if ((Player.IsJumping < Temp) ||
					    (Player.IsFalling < Temp)) {
						Player.Jumpspeed = 2;
						Player.Fallspeed = 2;
					}
				}
			}
		}

		// Player.Jumpspeed = 2;
		// Player.Fallspeed = 2;
	} else {
		if (Player.Yoffset < 0) {
			Player.Yoffset++;
		} else {
			Player.Yoffset--;
		}
	}

	/*
  if( (Player.Attribs & 0b01000000) ){//able to throw fireballs ?
          if( Keystate.Fire && (!Previous_keystate.Fire) ){
                  Player_add_fireball();
          };
  };
  */
	if (Player.IsClimbing > 0)
		Player.IsClimbing--;

	Player.IsInWater = 0;
	Player.IsInQsand = 0;

	SavePlayer.Attribs = SavePlayer.Attribs &
			     0b11101111; // draw player before fg OFF

	// scan through the tiles covered by mario.test for bonustiles, hostile tiles
	for (C = 0; C <= 15; C += 15) {
		Temp = Get_tile(Player.X + C, Player.Y); // Upper
		if ((Temp >= non_solid_bonus_low) &&
		    (Temp <= non_solid_bonus_high))
			Player_collect_bonus(Player.X + C,
					     Player.Y); // collect bonus
		if ((Temp >= non_solid_interactive_low) &&
		    (Temp <= non_solid_interactive_high))
			Interactive_non_solid_tile_handlers
				[Temp - non_solid_interactive_low](Player.X + C,
								   Player.Y);
		if ((Temp >= non_solid_hostile_low) &&
		    (Temp <= non_solid_hostile_high))
			Player_die();

		Temp = Get_tile(Player.X + C, Player.Y + 15); // middle
		if ((Temp >= non_solid_bonus_low) &&
		    (Temp <= non_solid_bonus_high))
			Player_collect_bonus(Player.X + C,
					     Player.Y + 15); // collect bonus
		if ((Temp >= non_solid_interactive_low) &&
		    (Temp <= non_solid_interactive_high))
			Interactive_non_solid_tile_handlers
				[Temp - non_solid_interactive_low](
					Player.X + C, Player.Y + 15);
		if ((Temp >= non_solid_hostile_low) &&
		    (Temp <= non_solid_hostile_high))
			Player_die();

		Temp = Get_tile(Player.X + C,
				Player.Y + Player.Height - 1); // lower
		if ((Temp >= non_solid_bonus_low) &&
		    (Temp <= non_solid_bonus_high))
			Player_collect_bonus(Player.X + C,
					     Player.Y + Player.Height -
						     1); // collect bonus
		if ((Temp >= non_solid_interactive_low) &&
		    (Temp <= non_solid_interactive_high))
			Interactive_non_solid_tile_handlers
				[Temp - non_solid_interactive_low](
					Player.X + C, Player.Y + Player.Height);
		if ((Temp >= non_solid_hostile_low) &&
		    (Temp <= non_solid_hostile_high))
			Player_die();
	}

	// fly code
	if (Runflag == 2) {
		if (Player.PrevFace == Player.Face) {
			// if(Player.Runcount<flycondition)
			if (Player.Runcount < (2 * flycondition))
				Player.Runcount++;

			if (Player.Runcount >= flycondition)
				Player.Flycount = flycondition + flylength;

		} else {
			// Player.Runcount = Player.Runcount/2;
			Player.Runcount = (Player.Runcount >> 1);
		}

	} else {
		if (Player.Runcount > 0)
			Player.Runcount--;
	}

	if (Player.Flycount > 0)
		Player.Flycount--;

	//	Player.Test =Runflag ;
	if ((Runflag < 2) && (!Player.IsJumping)) //(Runflag & Player.IsJumping)
		Player.Flycount = 0;

	Player.PrevFace = Player.Face;

	if (SavePlayer.Attribs & 0b00001000) { // P-wing
		Player.Flycount = flycondition + flylength;
		Player.Runcount = flycondition;
	}
}

void Player_die()
{
	if (!(Player.Immortal)) {
		SavePlayer.Life--;
		Player.Immortal = 60; // Nr of frames to be immortal

		if (SavePlayer.Life == 1) {
			if (Player.Height == 18) {
				Player.Y += 2;
				SavePlayer.Spritenr = 0;
			} else {
				Player.Y += 11;
			}
			Player.Height = Player.Height2 = 16;
			// Player.Y += 11;
			SavePlayer.Maskbase = SavePlayer.Spritebase =
				(Player.Face == 1) ? 1 : 0;
			// Player.Attribs = Player.Attribs&0b10111111;//disable fireball
		}

		if (SavePlayer.Life == 2) {
			SavePlayer.Maskbase = SavePlayer.Spritebase =
				(Player.Face == 1) ? 3 : 2;
			SavePlayer.Attribs =
				SavePlayer.Attribs &
				0b10010111; // disable fireball, racoon suit and p-wing
		}
		if (SavePlayer.Life <= 0) { // player dead
			// die,animate,exit

			Player.Immortal = 0;
			Player_die_hard();
		}
	}
}

void Player_die_hard()
{
	// New: V 1.04 Increased speed of die animation
	/*char*/ int16_t Temp = -2;

	SavePlayer.Spritenr = 9;

	while (Player.Y <
	       FgY + screen_height) { // die animation: jump up and fall
		// down, out of screen

		if (Player.Y < FgY + 10) {
			// New: V 1.04 Increased speed of die animation
			Temp = 3;
		}
		Player.Y += Temp;

		Render();
	}
	SavePlayer.Lives--;
	SavePlayer.Life = 1;
	Player.Height = Player.Height2 = 16;
	SavePlayer.Maskbase = SavePlayer.Spritebase = 1;
	Player.Face = 1;
	SavePlayer.Attribs = 0;
	Exit = 1;
}

// get the id-numner of the tile in which coordinates x and y is
uint8_t Get_tile(int16_t X, int16_t Y)
{
	if ((Y >= 0) && (Y < (Leveldata.Height * 16))) {
		// if( (Y>=0) && (Y<(Leveldata.Height<<4)) ){
		// return *( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) );
		return *((char *)(Fg_plane.p.matrix +
				  (Y >> 4) * Fg_plane.p.width + (X >> 4)));
	}
	return 0; // to avoid unpredictable bugs when player goes outside level
	// upwards and downwards
}

void Put_tile(int16_t X, int16_t Y, uint8_t Tile)
{
	/*		if( (X>(Leveldata.Width*16)) && (Y>(Leveldata.Height*16)) )
                          return;*/

	//*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) ) = Tile;
	*((char *)(Fg_plane.p.matrix + (Y >> 4) * Fg_plane.p.width +
		   (X >> 4))) = Tile;
	/*	Fg_plane.p.force_update=1;
          Fg_mask.p.force_update=1;*/
}

void Dummy_func(int16_t X, int16_t Y)
{
	// Nothing
}

void Player_add_fireball()
{ // adds a new fireball if free slot. This func can
	// be optimized when using only two fireballs at
	// once
	uint16_t C;
	// DrawStr(30,30,"fireball",A_NORMAL);

	for (C = 0; C < nr_of_player_smallshots; C++) {
		if (Player_smallshots[C].Mode == 0) {
			//			Player_smallshots[C].Mode =
			// Player.Face;//activate and give x-direction
			Player_smallshots[C].Mode = 1;
			Player_smallshots[C].Data2 =
				Player.Face; // activate and give x-direction
			Player_smallshots[C].X = Player.X + 8;
			Player_smallshots[C].Y = Player.Y + 3;
			Player_smallshots[C].Data0 = 1; // y-dir
			Player_smallshots[C].Data1 = 0;
			C = nr_of_player_smallshots;
		}
	}
}

void Player_handle_fireballs()
{
	int16_t C;

	for (C = 0; C < nr_of_player_smallshots; C++) {
		if ((FgX > Player_smallshots[C].X) ||
		    ((FgX + screen_width) < Player_smallshots[C].X) ||
		    ((FgY - 16) > Player_smallshots[C].Y) ||
		    ((FgY + screen_height) < Player_smallshots[C].Y)) {
			Player_smallshots[C].Mode = 0;
		} else {
			if ((Player_smallshots[C].Mode != 0))
				Player_fireball_handler(&Player_smallshots[C]);
		}
	}
}

// void Player_fireball_handler(smallshot *Fireball){
void Player_fireball_handler(struct shot *Fireball)
{
	// short C;

	//	Fireball->X += player_fireball_speed*Fireball->Mode;
	Fireball->X += player_fireball_speed * Fireball->Data2;

	//	if( (FgX>Fireball->X) || ((FgX+screen_width) < Fireball->X) || (FgY >
	// Fireball->Y) || ((FgY+screen_height)<Fireball->Y) )
	// Fireball->Mode = 0;//kill fireball if outside screen, now done in
	// handle_shots

	Fireball->Y += (Fireball->Data0 * player_fireball_speed);
	Fireball->Data1++;

	if ((Fireball->Data0 < 1) && (Fireball->Data1 >= 5)) {
		Fireball->Data0++;
		Fireball->Data1 = 0;
	}

	//	if(!Free_down(Fireball->X,Fireball->Y+8) ){//X-4
	if (Free_down(Fireball->X, Fireball->Y + 8) <
	    player_fireball_speed) { // X-4
		//	if(Free_down(Fireball->X,Fireball->Y+8) < player_fireball_speed){//X-4
		// if( (Get_tile(Fireball->X+4,Fireball->Y+8) >= solid_down_low) ){//testing
		// middle point, will possibly change
		// if(!(Free_down(Fireball->X,Fireball->Y+8)>=player_fireball_speed)){
		Fireball->Data0 = -1; // bounche upwards
		Fireball->Data1 = 0;
		//		Fireball->Y -= 2;
	}

	//	if( ((Get_tile(Fireball->X+4,Fireball->Y+2) >= solid_low) /*&&
	//(Get_tile(Fireball->X+4,Fireball->Y+1)<slope_under_left_low)*/) ||
	//((Get_tile(Fireball->X+4,Fireball->Y+6) >= solid_low) /*&&
	//(Get_tile(Fireball->X+4,Fireball->Y+6)<slope_under_left_low)*/) ){
	if ((Get_tile(Fireball->X + 4, Fireball->Y + 4) >= solid_low)) {
		Add_dustsky(Fireball->X, Fireball->Y);

		Fireball->Mode = 0; // kill fireball
	}
}
