#include "items.h"
#include "bounch.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "compat/gray.h"
#include "compat/utils.h"
#include "control.h"
#include "flying.h"
#include "gfx.h"
#include "level.h"
#include "objects.h"
#include "player.h"
#include "render.h"
#include "scankeys.h"
#include "shells.h"
#include <stdlib.h>

item *Items;

trigger *Triggers;

inline void Handleitems() {
  short C, D;

  for (C = 0; C < nr_of_items; C++) {
    if (Items[C].Active) { // if existing

      Items[C].Handler(&Items[C]);

      if (BoundsCollide(Items[C].X, Items[C].Y, Items[C].Height, Player.X,
                        Player.Y, Player.Height)) {
        Items[C].Collect(&Items[C]);
      };

      for (D = 0; D < nr_of_bounching_tiles; D++) {
        if (Bounching_tiles[D].Active) {
          if ((Items[C].Height == Items[C].Height2) &&
              BoundsCollide(Bounching_tiles[D].X, Bounching_tiles[D].Y, 16,
                            Items[C].X, Items[C].Y, Items[C].Height)) {

            Items[C].Y -= 8;

            if (Items[C].X < Bounching_tiles[D].X) {
              Items[C].X -= 4;
              // Items[C].Y -= 8;
              Items[C].Active = -abs(Items[C].Active);
            } else {
              Items[C].X += 4;
              // Items[C].Y -= 8;
              Items[C].Active = abs(Items[C].Active);
            };
          };
        };
      };
    };
  };
};

void Dark_gray_magic_mode_handler(short X, short Y) {

  if (Player.Behind_fg) {
    Leveldata.Condition = 240;
    Exit = 2;
  }
}

void White_magic_box_handler(short X, short Y) {

  if (Keystate.Down) {
    if ((++Player.White_magic_box_counter) == 100) {
      Player.Behind_fg = 200;
      Player.White_magic_box_counter = 0;
      Player.Y++;
    }
  } else {
    // standing on magix box, not crounching
    Player.White_magic_box_counter = 0;
  }
};

void Player_collect_bonus(short X, short Y) {

  Bonushandlers[Get_tile(X, Y) - non_solid_bonus_low](X, Y);
}

void Player_get_coin(short X, short Y) {

  if ((++SavePlayer.Coins) == 100) {
    SavePlayer.Coins = 0;
    SavePlayer.Lives++;
    Score_anim(X, Y - 8, score_1up);
  };
  SavePlayer.Score++;
};

void Player_collect_coin(short X, short Y) {

  Player_get_coin(X, Y);

  Put_tile(X, Y, 0);
}

void Player_collect_coin_water(short X, short Y) {

  Player_get_coin(X, Y);

  Put_tile(X, Y, water);
}

void Player_collect_flower(short X, short Y) {
  unsigned char Temp;

  if (SavePlayer.Life == 1) {

    SavePlayer.Life = 2;
    Player.Height2 = Player.Height = 27;
    Player.Y -= 11; // to avoid falling through solid tiles immedeately after
                    // becoming large
    SavePlayer.Maskbase = SavePlayer.Spritebase =
        (Player.Face == 1) ? 3 : 2; // set new sprites

  } else {

    SavePlayer.Attribs = SavePlayer.Attribs | 0b01000000; // add fireball
                                                          // ability
    SavePlayer.Attribs =
        SavePlayer.Attribs & 0b11010111; // remove racoon suit and p-wing
    SavePlayer.Life = 3;

    SavePlayer.Spritebase =
        (Player.Face == 1) ? 5 : 4; // set new sprites,changes to come when
                                    // fireball sprites are ready
    SavePlayer.Maskbase = (Player.Face == 1) ? 3 : 2;
  };

  Score_anim(/*16*(X/16)*/ X & 0xfff0, (Y & 0xfff0) - 8 /*16*(Y/16)-8*/,
             score_1000);

  if (Get_tile(X, Y) == flower_water) { //
    Temp = water;
  } else {
    Temp = 0;
  };

  Put_tile(X, Y, Temp);
}

void Trigger_pow_item(short X, short Y) {
  short C;

  Put_tile(X, Y, pow_item_after);

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = 1;
    //(object*)Objects[C].Handler = Handle_timed_coinconvert;
    Objects[C].Draw = Handle_timed_coinconvert; // Dummy_func_;
    Objects[C].X = 0;
  };

  // convert all bricks to coins and initialize timer
  Convert_brick_coin();
};

void Convert_brick_coin() {
  unsigned short C;

  for (C = 0; C < (Fg_plane.p.width * Leveldata.Height); C++) {
    if ((*((unsigned char *)(Fg_plane.p.matrix + C)) == brick) ||
        (*((unsigned char *)(Fg_plane.p.matrix + C)) == bounching_brick)) {
      *((char *)(Fg_plane.p.matrix + C)) = coin;
    } else {
      if (*((char *)(Fg_plane.p.matrix + C)) == coin) {
        *((char *)(Fg_plane.p.matrix + C)) = brick;
      };
    };
  };
};

void Pipe_down_left_handler(short X, short Y) {
  /*if( Player.X >= ((X/16)*16) )
          Pipe_down_handler( (X/16) ,(Y/16) );*/
  if (Player.X >= (short)(X & 0xfff0)) // same as above, but optimized
    Pipe_down_handler((X >> 4), (Y >> 4));
};

void Pipe_down_right_handler(short X, short Y) {
  /*if( Player.X <= ((X/16)*16) )
          Pipe_down_handler( (X/16)-1 ,(Y/16) );*/
  if (Player.X <= (short)(X & 0xfff0)) // same as above, but optimized
    Pipe_down_handler((X >> 4) - 1, (Y >> 4));
};

void Pipe_down_handler(short X, short Y) {
  /*char*/ short Height = Player.Height;
  short C;

  if (Keystate.Down) {

    SavePlayer.Spritenr = 9;
    Keystate.Down = 0;

    while (Player.Height - 1) {
      Player.Y++;
      Player.Height--;
      Render();
    };
    Player.Height = Height;

    Execute_trigger(X, Y);
  };
};

void Pipe_up_left_handler(short X, short Y) {
  /*if( Player.X >= ((X/16)*16) )
          Pipe_up_handler( (X/16) ,(Y/16) );*/
  if (Player.X >= (short)(X & 0xfff0)) // same as above, but optimized
    Pipe_up_handler((X >> 4), (Y >> 4));
};

void Pipe_up_right_handler(short X, short Y) {
  /*if( Player.X <= ((X/16)*16) )
          Pipe_up_handler( (X/16)-1 ,(Y/16) );*/
  if (Player.X <= (short)(X & 0xfff0)) // same as above, but optimized
    Pipe_up_handler((X >> 4) - 1, (Y >> 4));
};

void Pipe_up_handler(short X, short Y) {
  short C;

  short Height = Player.Height;

  if (Keystate.Up) {

    SavePlayer.Spritenr = 9;
    Keystate.Down = 0;
    while (Player.Height - 1) {

      Player.SpriteOffset++;
      Player.Height--;

      Render();
    };

    Player.SpriteOffset = 0;
    Player.Height = Height;

    Execute_trigger(X, Y);
  };
};

void Pipe_right_handler(short X, short Y) {
  short C;

  // if( ((Player.Y+Player.Height) <= ((Y/16)*16+16)) &&
  // (Player.Y>=((Y/16)*16-16)) ){
  if (((Player.Y + Player.Height) <= ((short)(Y & 0xfff0) + 16)) &&
      (Player.Y >= ((short)(Y & 0xfff0) - 16))) {
    if (Keystate.Right) {

      // animate
      Player.Y -= 2;

      for (C = 0; C < 16; C++) {
        Player.Blit = Player.Blit << 1;
        Player.X++;
        Render();

        if ((++SavePlayer.Spritenr) >= 6) {
          SavePlayer.Spritenr = 0;
        };
      };
      Player.Blit = 0xffff;
      // Player.X -= 16;
      // Player.Attribs = Player.Attribs & 0b11101111;//draw player before fg
      // OFF

      // Execute_trigger( (X/16),(Y/16) );
      Execute_trigger((X >> 4), (Y >> 4));
    };
  };
};

void Pipe_left_handler(short X, short Y) {
  short C;

  // if( ((Player.Y+Player.Height) <= ((Y/16)*16+16)) &&
  // (Player.Y>=((Y/16)*16-16)) ){
  if (((Player.Y + Player.Height) <= ((short)(Y & 0xfff0) + 16)) &&
      (Player.Y >= ((short)(Y & 0xfff0) - 16))) {
    if (Keystate.Left) {

      // animate
      Player.Y -= 2;
      Player.Blit = 0x7fff;

      for (C = 0; C < 16; C++) {
        Player.Blit = Player.Blit >> 1;
        Player.X--;
        Render();
        if ((++SavePlayer.Spritenr) >= 6) {
          SavePlayer.Spritenr = 0;
        };
      };
      Player.Blit = 0xffff;
      // Player.X += 16;
      // Player.Attribs = Player.Attribs & 0b11101111;//draw player before fg
      // OFF

      // Execute_trigger( (X/16),(Y/16) );
      Execute_trigger((X >> 4), (Y >> 4));
    };
  };
};

void Add_break_brick_anim(short X, short Y) {
  short C;

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = 1;
    Objects[C].Data0 = 0;
    Objects[C].Data1 = 0;
    Objects[C].X = X;
    Objects[C].Y = Y;
    //(object*)Objects[C].Handler = Dummy_func_;//Handle_brick_fragments;
    Objects[C].Draw = Draw_brick_fragments;
  };
}

void Break_brick(short X, short Y) {
  short C;

  // if((Player.Life==1) && (Player.Y==((Y/16)*16+16)) ){
  if ((SavePlayer.Life == 1) && (Player.Y == ((short)(Y & 0xfff0) + 16)) &&
      (abs(Player.X - (short)(X & 0xfff0)) <= 16)) {
    Elastic_tile_animate(X, Y, brick, 0b10000000, 20); // 4
    Put_tile(X, Y, bounching_brick);
  } else { // break

    Put_tile(X, Y, 0);
    // animate

    Add_break_brick_anim(X, Y - 8);
  };
};

void Falling_block_handler(short X, short Y) {
  short C;

  for (C = 0; C < nr_of_dummy_platforms; C++) {
    if (!Flying_platforms[C].Active) { // find free dummy platform

      Flying_platforms[C].Active = 1; // initiate
      //			Flying_platforms[C].DrawMode = 1;
      Flying_platforms[C].Data1 = 1;
      Flying_platforms[C].Data0 = Flying_platforms[C].Data2 =
          Flying_platforms[C].Data3 = 0;
      Flying_platforms[C].Width = 1;
      Flying_platforms[C].Sprite = 60000; // 190;
      Flying_platforms[C].X = X & 0xfff0; // 16*(X>>4);///16);
      Flying_platforms[C].Y = Y & 0xfff0; // 16*(Y>>4);///16);
      Flying_platforms[C].Handler = Platform_handler_3;

      Put_tile(X, Y, blank_solid); // make tile invincible
      Update_FG = 1; // Fg_plane.p.force_update = Fg_mask.p.force_update = 1;
      break;
    };
  };
}

void Brick_hidden_coins(short X, short Y) {
  //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) ) =
  // bonusbox_8_coins;
  Put_tile(X, Y, bonusbox_8_coins);
  Bonus_box_coins(X, Y);
};

void Bonus_box_coin(short X, short Y) {

  // Player_get_coin(X,Y);

  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12); // 0

  Bonusbox_coin_animate(X, Y);
};

void Bonus_box_coins(short X, short Y) {

  // Player_get_coin(X,Y);

  // Elastic_tile_animate(X, Y, (--(*(
  // (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) ))), 0b10000000,
  // 0 );
  Elastic_tile_animate(
      X, Y,
      (--(*((char *)(Fg_plane.p.matrix + (Y >> 4) * Fg_plane.p.width +
                     (X >> 4))))),
      0b10000000, 12);

  Bonusbox_coin_animate(X, Y);
};

void Bonus_box_powerup_1(short X, short Y) {

  // Add_power_up_1(16*(X/16),16*(Y/16));
  Add_power_up_1(X & 0xfff0, Y & 0xfff0);

  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12);
};

void Bonus_box_powerup_2(short X, short Y) {

  // Add_power_up_2(16*(X/16),16*(Y/16)-2,2);
  Add_power_up_2(X & 0xfff0, (Y & 0xfff0) - 2, 2);

  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12);
};

void Bonus_box_1up(short X, short Y) {
  short C;
  X = (X & 0xfff0);

  if ((C = Find_free_item()) != -1) {
    Items[C].X = X;                //(X & 0xfff0);//16*(X/16);
    Items[C].Y = (Y & 0xfff0) - 2; // 16*(Y/16)-2;
    Items[C].Height = 2;
    Items[C].Height2 = 16;
    Items[C].Active = ((Player.X > /*Items[C].*/ X) ? -one_up_mushrom_speed
                                                    : one_up_mushrom_speed);
    Items[C].Sprite = one_up_mushrom_sprite;
    Items[C].Collect = Player_collect_one_up_mushrom;
    Items[C].Handler = Mushrom_handler;
  };

  //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16))
  //)=bonusbox_after;
  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12);
};

void Bonus_box_pow(short X, short Y) {

  //*( (char*)(Fg_plane.p.matrix+((Y/16)-1)*Fg_plane.p.width+(X/16)) )=pow_item;
  //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16))
  //)=bonusbox_after;
  Put_tile(X, Y - 16, pow_item);
  Put_tile(X, Y, bonusbox_after);
};

void Bonus_box_star(short X, short Y) {
  short C;
  X = (X & 0xfff0);

  if ((C = Find_free_item()) != -1) {
    Items[C].X = X;                //(X & 0xfff0);//16*(X/16);
    Items[C].Y = (Y & 0xfff0) - 2; // 16*(Y/16)-2;
    Items[C].Height = 2;
    Items[C].Height2 = 16;
    Items[C].Active = (Player.X > /*Items[C].*/ X) ? -star_speed : star_speed;
    Items[C].Data0 = 16;
    Items[C].Data1 = -2;
    Items[C].Sprite = star_anim;
    Items[C].Collect = Player_collect_star;
    Items[C].Handler = Star_handler;
  };

  //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16))
  //)=bonusbox_after;
  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12);
};

void Bonus_box_climbing_flower(short X, short Y) {
  short C;

  if ((C = Find_free_object()) ==
      -1) { // if no free slots, use the first slot, which has been occupied for
            // the longest time
    C = 0;
  };

  Objects[C].Active = 1;
  Objects[C].Data0 = 2;
  Objects[C].X = (X & 0xfff0);     // 16*(X/16);//X & 0xfff0;
  Objects[C].Y = (Y & 0xfff0) - 2; // 16*(Y/16)-2;//; & 0xfff0
  //(object*)Objects[C].Handler = Dummy_func_;//Handle_climbing_flower;
  Objects[C].Draw = Draw_climbing_flower;

  /*
  Fg_plane.p.force_update=1;
  Fg_mask.p.force_update=1;
  */

  Elastic_tile_animate(X, Y, bonusbox_after, 0b10000000, 12);
};

void Note_handler(short X, short Y) {
  char Temp;
  char Yoffset;

  if (/*(16*(Y/16))*/ (short)(Y & 0xfff0) < Player.Y) {
    /*Player.*/ Yoffset = 3; // 5
    Player.Fallspeed = 2;    // 5
    Temp = 0b10000000;
  } else {
    /*Player.*/ Yoffset = -5; //-5
    Player.Jumpspeed = 4;     // 8
    Temp = 0b01000000;
  };
  Player.Yoffset = Yoffset;

  Player.Xoffset = 4 * Player.Face;
  Player.Walkspeed = 4;  // 8
  Player.Walkspeed2 = 2; // 4

  Player.Max_jumpheight = Player.IsJumping = 2 * Player.Max_jumpheight;
  Elastic_tile_animate(X, Y, note, Temp, 17); // 1
};

void Pu_2_note_handler(short X, short Y) {
  char Temp;
  char Yoffset;

  // if( (16*(Y/16)) < Player.Y){
  if ((short)(Y & 0xfff0) < Player.Y) {
    /*Player.*/ Yoffset = 3; // 5
    Player.Fallspeed = 2;    // 5
    Temp = 0b10000000;
    Add_power_up_2((X & 0xfff0), (Y & 0xfff0) - 2,
                   2); // Add_power_up_2(16*(X/16),16*(Y/16)-2,2);
  } else {
    /*Player.*/ Yoffset = -5; //-5
    Player.Jumpspeed = 4;     // 8
    Temp = 0b01000000;
    Add_power_up_2((X & 0xfff0), (Y & 0xfff0) + 18,
                   16); // Add_power_up_2(16*(X/16),16*(Y/16)+18,16);
  };
  Player.Yoffset = Yoffset;

  Player.Xoffset = 4 * Player.Face;
  Player.Walkspeed = 4;  // 8
  Player.Walkspeed2 = 2; // 4

  Player.Max_jumpheight = Player.IsJumping = 2 * Player.Max_jumpheight;
  Elastic_tile_animate(X, Y, note, Temp, 17); // 1
};

void Dark_note_handler(short X, short Y) {
  char Temp;
  char Yoffset;

  Player.IsJumping = Player.Max_jumpheight;

  // if( (16*(Y/16)) < Player.Y){
  if ((short)(Y & 0xfff0) < Player.Y) {
    /*Player.*/ Yoffset = 3; // 5;
    Player.Fallspeed = 2;    // 5;
    Temp = 0b10000000;
  } else {
    /*Player.*/ Yoffset = -5; //-8;
    Player.Jumpspeed = 4;     // 8;
    Temp = 0b01000000;
    // if(Keystate.Jump && (Player.X>=((X/16)*16-2)) &&
    // (Player.X<=((X/16)*16+2)) ){
    X = X & 0xfff0;
    if (Keystate.Jump && (Player.X >= ((short)(X /*&0xfff0*/) - 2)) &&
        (Player.X <= ((short)(X /*&0xfff0*/) + 2))) {
      // animate
      SavePlayer.Spritenr = 7;
      while (Player.Y >= 0) {
        Player.Y -= 4;
        Render();
      };

      Player.IsJumping = 0;
      // Execute_trigger( (X/16),(Y/16) );
      Execute_trigger((X >> 4), (Y >> 4));
    };
  };
  Player.Yoffset = Yoffset;

  Player.Xoffset = 4 * Player.Face;
  Player.Walkspeed = 4;                            // 8;
  Player.Walkspeed2 = 2;                           // 4;
  Elastic_tile_animate(X, Y, note_dark, Temp, 47); // 2
};

void Wood_block_pu2_handler(short X, short Y) {
  unsigned char Temp;

  X = X & 0xfff0;
  Add_power_up_2((X /*& 0xfff0*/), (Y & 0xfff0) - 2,
                 2); // Add_power_up_2( 16*(X/16), 16*(Y/16)-2, 2);

  // if( (Player.X+15)<=((X/16)*16) ){
  if ((Player.X + 15) <= (short)(X /*&0xfff0*/)) {
    Temp = 0b00010000;
  } else {
    Temp = 0b00100000;
  };
  Elastic_tile_animate(X, Y, wood_block, Temp, 73); // 3
};

void Door_handler(short X, short Y) {

  // X = ((X/16)*16);
  X = X & 0xfff0;

  //	if( (Player.X>=X-4) && (Player.X<=X+4) ){
  if (abs(Player.X - X) <= 4) {
    if (Keystate.Up && !(Previous_keystate.Up)) {

      Execute_trigger((X >> 4), (Y >> 4));

      Previous_keystate.Up = 1;
    };
  };
};

short HiddenConditions(short Y) {
  // using this func instead of repeated code gave a significant size decrease
  if (Player.IsJumping) {
    if (((short)(Y & 0xfff0)) <= Player.Y) {
      Player.Flycount = Player.IsJumping = 0;
      return 1;
    }
  }
  return 0;
}

void Hidden_bonusbox_1_coin_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Bonus_box_coin(X,Y);

          };
  };	*/

  if (HiddenConditions(Y)) {
    Bonus_box_coin(X, Y);
  }
};

void Hidden_bonusbox_8_coins_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Put_tile(X,Y,bonusbox_8_coins);

          Bonus_box_coins(X,Y);

          };
  };*/
  if (HiddenConditions(Y)) {

    Put_tile(X, Y, bonusbox_8_coins);
    Bonus_box_coins(X, Y);
  }
};

void Hidden_bonusbox_1_up_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Bonus_box_1up(X,Y);

          };
  };*/

  if (HiddenConditions(Y)) {
    Bonus_box_1up(X, Y);
  }
};

void Hidden_bonusbox_pu_1_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Bonus_box_powerup_1(X,Y);

          };
  };*/
  if (HiddenConditions(Y)) {
    Bonus_box_powerup_1(X, Y);
  }
};

void Hidden_bonusbox_pu_2_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Bonus_box_powerup_2(X,Y);

          };
  };*/
  if (HiddenConditions(Y)) {
    Bonus_box_powerup_2(X, Y);
  }
};

void Hidden_bonusbox_pow_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

          Player.Flycount = Player.IsJumping = 0;

          Bonus_box_pow(X,Y);

          };
  };*/
  if (HiddenConditions(Y)) {
    Bonus_box_pow(X, Y);
  }
};

void Hidden_note_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

//		Put_tile(X,Y,note);

          Note_handler(X,Y);

          Player.Flycount = Player.IsJumping = 0;

          };
  };*/
  if (HiddenConditions(Y)) {
    Note_handler(X, Y);
    Player.Flycount = Player.IsJumping = 0;
  }
};

void Hidden_dark_note_handler(short X, short Y) {

  /*if(Player.IsJumping){
          //if( (((Y/16)*16)) <= Player.Y ){
          if( ((short)(Y&0xfff0)) <= Player.Y ){

//		Put_tile(X,Y,note_dark);

          Dark_note_handler(X,Y);

          Player.Flycount = Player.IsJumping = 0;

          };
  };*/
  if (HiddenConditions(Y)) {
    Dark_note_handler(X, Y);
    Player.Flycount = Player.IsJumping = 0;
  }
};

void Ladder_handler(short X, short Y) {

  if (abs((Player.X) - (short)(X & 0xfff0)) <= 6) {

    if (Keystate.Up || Player.IsClimbing) {

      SavePlayer.Spritenr = 4;

      if (Player.IsClimbing == 1) {
        if (Keystate.Up && (Free_up(Player.X, Player.Y) >= 2))
          Player.Y -= 2;

        if (Keystate.Down &&
            (Free_down(Player.X, Player.Y + Player.Height - 1) >= 2))
          Player.Y += 2;
      };
      Player.IsClimbing = 2;
      Player.Face = 0;
    };
  };
};

void Quicksand_handler(short X, short Y) {

  if (!Player.IsInQsand) {

    Player.Jumpspeed = 2;
    Player.Walkspeed = 2;
    Player.Walkspeed2 = 2;

    Player.Behind_fg = 1;

    if (Free_down(Player.X, Player.Y + Player.Height))
      Player.Y++;

    Player.IsInQsand = 1;
  };
};

void Lava_handler(short X, short Y) {

  Water_handler(X, Y); // let the player swim through lava if he is immortal

  Player_die();
};

void Water_handler(short X, short Y) { // optimize!!!!!!!!!!
  short Temp;

  if (!Player.IsInWater) {

    Temp = Free_down(Player.X, Player.Y + Player.Height);

    if (Temp > 0) {
      if (!Keystate.Jump)
        Player.Y++;
      if (Keystate.Down && (Temp >= 2))
        Player.Y++;
      SavePlayer.Spritenr = 6;
    };

    Player.Jumpspeed = 2;

    Player.IsJumping = jumpheight; // 40;//jumpheight/2;

    if ((Keystate.Jump) || (Player.Xoffset /*>8*/)) {
      Player.Walkspeed = 2;
      Player.Walkspeed2 = 2;
      if (Keystate.Jump)
        Player.Yoffset = -4;
      if (Fg_plane.step % 2) {
        SavePlayer.Spritenr = 5;
      }

      if ((Keystate.Jump) &&
          (Player.Xoffset == 0)) { //&& (Keystate.Left || Keystate.Right) ){
        if (Keystate.Left) {
          Player.Xoffset = -16;
        } else {
          if (Keystate.Right) {
            Player.Xoffset = 16;
          };
        };
      };

    } else {
      char Walkspeed;
      char Walkspeed2;

      if ((Fg_plane.frame % 2)) {
        /*Player.*/ Walkspeed = 2;
        /*Player.*/ Walkspeed2 = 2;
      } else {
        /*Player.*/ Walkspeed = 0;
        /*Player.*/ Walkspeed2 = 0;
      };
      Player.Walkspeed = Walkspeed;
      Player.Walkspeed2 = Walkspeed2;
    };

    Player.IsInWater = 1;
  };
};

// Optimize

void Water_surface_handler(short X, short Y) {

  if ((SavePlayer.Spritenr == 8) /*|| (Player.Spritenr==7)*/) {
    Splash(X, (Y & 0xfff0) - 16 + 9);
  }

  if ((Player.Y) <= ((short)(Y & 0xfff0) + 2)) {
    if (Keystate.Jump &&
        (Keystate.Up /*|| !Free_down(Player.X,Player.Y+Player.Height)*/)) {
      Player.Max_jumpheight = 40;

    } else {
      if (Player.IsInWater) { // !(not deep water (one tile only, or one tile +
                              // slope))
        Player.IsJumping = 0;
        Player.Jumpspeed = 0;
      };
    };
  } else {
    Water_handler(X, Y);
  }
}

// item handlers
void Mushrom_handler(item *Item) { // handles power up mushrom and 1 up mushrom
  short Temp;

  if (Item->Height < 16) {
    Item->Height++;
    Item->Y--;
  } else {
    if (Item->Active > 0) { // dir = right
      if (Free_right(Item->X, &Item->Y, Item->Height, Item->Active) >=
          abs(Item->Active)) { // not free right ?
        Item->X += Item->Active;
      } else {
        Item->Active = -Item->Active;
      };
    } else { // dir = left
      if (Free_left(Item->X, &Item->Y, Item->Height, Item->Active) >=
          abs(Item->Active)) { // not free left ?
        Item->X += Item->Active;
      } else {
        Item->Active = -Item->Active;
      };
    };

    Temp = Free_down(Item->X, Item->Y + Item->Height);
    Item->Y += (Temp >= enemy_fallspeed) ? (enemy_fallspeed) : (Temp);
  };
}

void Leaf_handler(item *Item) {

  if (Item->Data1) {
    Item->Y -= 2;
    Item->Data1--;
  } else {
    Item->Y++;
    Item->X += Item->Active;

    Item->Data0 += Item->Active;

    switch (Item->Data0) {
    case 0:
    case 24:
      Item->Active = -Item->Active;
      break;

    case 8:
      if (Item->Active < 0) {
        Item->Sprite = leaf_l_sprite;
        // Item->Mask = leaf_l_mask;
      } else {
        Item->Sprite = leaf_r_sprite;
        // Item->Mask = leaf_r_mask;
      };
    };
  };
};

void Star_handler(item *Item) {

  if (Item->Height < 16) { // coming out of box
    Item->Height++;
    Item->Y--;
  } else { // free (out of box)

    // optimize: free_dir
    if (Item->Active > 0) { // dir = right
      if( !Free_right(Item->X+star_speed,&Item->Y,16,star_speed)/*(Get_tile(Item->X+15+star_speed,Item->Y)>=solid_low) || (Get_tile(Item->X+15+star_speed,Item->Y+15)>=solid_low)*/ ){//not free right ?
        Item->Active = -Item->Active;
      };
    } else { // dir = left
      if( !Free_left(Item->X-star_speed,&Item->Y,16,-star_speed)/*(Get_tile(Item->X-star_speed,Item->Y)>=solid_low) || (Get_tile(Item->X-star_speed,Item->Y+15)>=solid_low)*/ ){//not free left ?
        Item->Active = -Item->Active;
      };
    };

    Item->X += Item->Active;

    if (Item->Data0) {
      if ((--Item->Data0) == 0)
        Item->Data1 = -Item->Data1;
    };

    if (Item->Data1 > 0) { // down
      if (!Free_down(Item->X, Item->Y + Item->Height)) {
        Item->Data1 = -Item->Data1;
        Item->Data0 = 16; // timer
      };
    } else { // up
      if (!Free_up(Item->X, Item->Y)) {
        Item->Data1 = -Item->Data1;
      };
    };

    Item->Y += Item->Data1;
  };
}

void Dead_boss_handler(item *Item) {

  if (Free_down(Item->X, Item->Y + 16))
    Item->Y += 2;
}

void Player_collect_dead_boss(item *Item) {
  unsigned short C; //,E;

  for (C = 0; C < 20; C++) {

    unsigned short D;
    if (C % 2) {

      //			memset(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,0xffff,LCD_SIZE);
      //			memset(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,0xffff,LCD_SIZE);

      //			FastFilledRect_Draw_R(dBufHPL_G,0,0,239,127);
      // FastFilledRect_Draw_R(dBufHPD_G,0,0,239,127);
      FilledRectDark(0, 0, 239, 127);

      /*
      FastFilledRect_Draw_R(GrayDBufGetHiddenPlane(LIGHT_PLANE),0,0,239,127);
      FastFilledRect_Draw_R(GrayDBufGetHiddenPlane(DARK_PLANE),0,0,239,127);
      */
      // GrayDBufToggleSync();
      GrayDBufToggleSync_SetPointers();
    } else {
      Render();
    }

    for (D = 0; D < 65000; D++)
      ;
  };

  Exit = 2;
  Map_plane.p.force_update = 1;
}

void Player_collect_pu_mushrom(item *Item) {
  unsigned char C;
  unsigned short D;

  Item->Active = 0;
  Score_anim(Item->X, Item->Y - 8, score_1000);

  if (SavePlayer.Life == 1) {
    SavePlayer.Life = 2;
    Player.Height2 = Player.Height = 27;
    Player.Y -= 11; // to avoid falling through solid tiles immedeately after
                    // becoming large
    SavePlayer.Maskbase = SavePlayer.Spritebase =
        (Player.Face == 1) ? 3 : 2; // set new sprites

    short H;
    short DY;
    short DBase;
    for (C = 0; C < 6; C++) {
      Render();
      if (!(C % 2)) {
        DBase = -2; // Player.Maskbase = SavePlayer.Spritebase	-=2;
        H = 16;     // Player.Height = Player.Height2 = 16;
        DY = 11;    // Player.Y += 11;
      } else {
        DBase = 2; // Player.Maskbase = SavePlayer.Spritebase +=2;
        H = 27;    // Player.Height = Player.Height2 = 27;
        DY = -11;  // Player.Y -= 11;
      };
      Player.Height = Player.Height2 = H;
      Player.Y += DY;
      SavePlayer.Maskbase = SavePlayer.Spritebase += DBase;
      for (D = 0; D < 65535; D++)
        ;
    };
  };
}

void Player_collect_one_up_mushrom(item *Item) {

  SavePlayer.Lives++;
  // Statusbardata.Update_lives=1;
  // animate
  Score_anim(Item->X, Item->Y - 8, score_1up);

  Item->Active = 0;
}

void Player_collect_star(item *Item) {

  SavePlayer.Attribs = SavePlayer.Attribs | 0b10000000;
  Player.Immortal = star_immortal_time;
  // animate
  Score_anim(Item->X, Item->Y - 8, score_1000);

  Item->Active = 0;
}

void Player_collect_leaf(item *Item) {

  if (SavePlayer.Life == 1) {

    SavePlayer.Life = 2;
    Player.Height = Player.Height2 = 27;
    Player.Y -= 11; // to avoid falling through solid tiles immedeately after
                    // becoming large
    SavePlayer.Maskbase = SavePlayer.Spritebase =
        (Player.Face == 1) ? 3 : 2; // set new sprites

  } else {

    SavePlayer.Attribs = SavePlayer.Attribs | 0b00100000; // add racoon suit
    SavePlayer.Attribs =
        SavePlayer.Attribs & 0b10111111; // remove fireball ability
    SavePlayer.Life = 3;

    SavePlayer.Maskbase = SavePlayer.Spritebase =
        (Player.Face == 1) ? 3 : 2; // set new sprites
    //		Player.Maskbase = (Player.Face==1) ? 3:2 ;
  };

  Item->Active = 0;

  Score_anim((Item->X & 0xfff0), (Item->Y & 0xfff0) - 8,
             score_1000); // Score_anim( 16*(Item->X/16), 16*(Item->Y/16)-8,
                          // score_1000);

  Add_dustsky(Player.X, Player.Y + 8);
};

short Free_down(short X, short Y) { // works width 16 objects only
  /*char*/ short Val = 16;          // char C;
  // unsigned char Temp1,Temp2;

  /*Temp1=Get_tile(X,Y+15);
  Temp2=Get_tile(X+15,Y+15);*/

  // if( ((Temp1>=solid_down_low) && (Temp1<=solid_high)) ||
  // ((Temp2>=solid_down_low) && (Temp2<=solid_high)) )
  if ((Get_tile(X, Y + 15) >= solid_down_low) ||
      (Get_tile(X + 15, Y + 15) >= solid_down_low))
    Val = (15 -
           ((Y - 1) &
            0x000f)); // Val = (15-(Y-1)%16);	//distance to next tile down
                      // //Val = 15-(Enemy->Y+Enemy->Height-1)%16;
  /*	else
                  return 16;	//return Val;*/

  if ((Get_tile(X + 15, Y) >= slope_under_left_low) &&
      (Get_tile(X + 15, Y) <= slope_under_left_high)) {
    Val = ((15 - ((X + 15) & 0x000f)) -
           ((Y) & 0x000f)); // Val = ( (15-((X+15)%16))-((Y)%16) );
  };

  if ((Get_tile(X, Y) >= slope_under_right_low) &&
      (Get_tile(X, Y) <= slope_under_right_high)) {
    Val = (((X) & 0x000f) - ((Y) & 0x000f)); // Val = ( ((X)%16)-((Y)%16) );
  };

  return Val;
}

short Free_up(short X, short Y) { // works width 16 objects only
  // char C;//Val = 16,

  if ((Get_tile(X, Y - 15) >= solid_low) ||
      (Get_tile(X + 15, Y - 15) >= solid_low))
    return ((Y) & 0x000f); // return ((Y)%16);	//distance to next tile down
                           // //Val = 15-(Enemy->Y+Enemy->Height-1)%16;
  else
    return 16; // return Val;
}

short Free_right(short X, short *Y, short Height, char Face) {
  /*char*/ short Val = 16, C;

  if (((Get_tile(X + 15, *Y + Height - 1) >= slope_under_left_low) &&
       (Get_tile(X + 15, *Y + Height - 1) < slope_under_left_high)) ||
      ((Get_tile(X + 18, *Y + Height - 1) >= slope_under_left_low) &&
       (Get_tile(X + 18, *Y + Height - 1) < slope_under_left_high)))
    *Y -= abs(Face);

  for (C = 0; C <= (Height - 1);
       C += min(16, Height - 1)) { // C=0;C!=(Height-1);C+=16
    /*if(C>(Height-1))
            C = (Height-1);*/
    if ((Get_tile(X + 15 + 15, *Y + C) >= solid_low) &&
        (Get_tile(X + 15 + 15, *Y + C) <= solid_high))
      Val = 15 - ((X + 15) & 0x000f); // Val = 15-(X+15)%16;
                                      // //distance to next tile right
  };

  return Val;
};

short Free_left(short X, short *Y, short Height, char Face) {
  /*char*/ short Val = 16, C;

  if (((Get_tile(X, *Y + Height - 1) >= slope_under_right_low) &&
       (Get_tile(X, *Y + Height - 1) <= slope_under_right_high)) ||
      ((Get_tile(X - 3, *Y + Height - 1) >= slope_under_right_low) &&
       (Get_tile(X - 3, *Y + Height - 1) <= slope_under_right_high)))
    *Y -= abs(Face);

  for (C = 0; C <= (Height - 1);
       C += min(16, Height - 1)) { // C=0;C!=(Height-1);C+=16

    if ((Get_tile(X - 16, *Y + C) >= solid_low) &&
        (Get_tile(X - 16, *Y + C) <= solid_high))
      Val = (X) &
            0x000f; // Val = (X)%16;		//distance to next tile left
  };

  return Val;
};

void Elastic_tile_animate(short X, short Y, unsigned char After, char Attribs,
                          unsigned char Spriteoffset) {
  short C;

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = elastic_tile_anim_time;
    Objects[C].Data0 = After;
    Objects[C].Data1 = Spriteoffset;
    Objects[C].Data2 =
        Attribs; // 0b10000000;//data2: b7:up b6:down b5:left b4:right
    Objects[C].X = X & 0xfff0; // 16*(X/16);
    Objects[C].Y = Y & 0xfff0; // 16*(Y/16);
    //(object*)Objects[C].Handler = Dummy_func_;//Handle_elastic_tile;
    Objects[C].Draw = Draw_elastic_tiles;
    //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) ) =
    //(Get_tile(X,Y+16)==water?dark_gray_solid:blank_solid);

    Put_tile(X, Y,
             (Get_tile(X, Y + 16) == water ? dark_gray_solid : blank_solid));

    Update_FG = 1; // Fg_plane.p.force_update=1;Fg_mask.p.force_update=1;
  } else {         // no free slots

    Put_tile(X, Y, After);
  };
}

void Bonusbox_coin_animate(short X, short Y) {
  short C;

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = 25;
    Objects[C].Data0 = 0;
    Objects[C].Data1 = 16;
    Objects[C].X = X & 0xfff0; // 16*(X/16);
    Objects[C].Y = Y & 0xfff0; // 16*(Y/16);
    //(object*)Objects[C].Handler = Dummy_func_;//Handle_bb_coin;
    Objects[C].Draw = Draw_bb_coin;
  };

  Player_get_coin(X, Y);
};

short Find_free_item() {
  short C;

  for (C = 0; C < nr_of_items; C++) {
    if (!(Items[C].Active)) {
      return C;
    };
  };
  return -1;
}

void Add_power_up_1(short X, short Y) {
  short C;
  unsigned char Temp;

  if (SavePlayer.Life == 1) {

    Add_pu_mushrom(X, Y - 2, 2);
  } else {
    if (Get_tile(X, Y - 16) == water) { //
      Temp = flower_water;
    } else {
      Temp = flower_power;
    }

    Put_tile(X, Y - 16, Temp);
  };
};

void Add_power_up_2(short X, short Y, short Height) {
  short C;

  if (SavePlayer.Life == 1) {

    Add_pu_mushrom(X, Y, Height);

  } else { // leaf

    if ((C = Find_free_item()) != -1) {
      Items[C].X = X;
      Items[C].Y = Y & 0xfff0; // 16*(Y/16);//-16;//16*(Y/16)-16;//Y-16;
      Items[C].Data1 = (Height == 16 ? 0 : 12);
      Items[C].Height = Items[C].Height2 = 14;
      Items[C].Active = 2;
      Items[C].Data0 = 12;
      // Items[C].Data1 = 12;
      Items[C].Sprite = leaf_r_sprite;
      // Items[C].Mask = leaf_r_mask;
      Items[C].Collect = Player_collect_leaf;
      Items[C].Handler = Leaf_handler;
    };
  };
};

void Add_pu_mushrom(short X, short Y, short Height) {
  short C;

  if ((C = Find_free_item()) != -1) {
    Items[C].X = X;
    Items[C].Y = Y;
    Items[C].Height = Height;
    Items[C].Height2 = 16;
    Items[C].Active =
        (Player.X > Items[C].X) ? -pu_mushrom_speed : pu_mushrom_speed;
    Items[C].Sprite = pu_mushrom_sprite;
    // Items[C].Mask = mushrom_mask;
    Items[C].Collect = Player_collect_pu_mushrom;
    Items[C].Handler = Mushrom_handler;
  };
}

void Score_anim(short X, short Y, char Score) {
  short C;

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = 10;
    Objects[C].Data0 = Score;
    Objects[C].X = X;
    Objects[C].Y = Y;
    //(object*)Objects[C].Handler = Dummy_func_;//Handle_score;
    Objects[C].Draw = Draw_score;
  };

  short S;
  switch (Score) {
  case score_100:
    S = 1; // SavePlayer.Score++;
    break;
  case score_200:
    S = 2; // SavePlayer.Score+=2;
    break;
  default:
    S = 10; // SavePlayer.Score+=10;
  }

  SavePlayer.Score += S;
};

void Execute_trigger(short X, short Y) {
  short C;

  // trigcomp
  for (C = 0; C < Leveldata.Nr_of_triggers; C++) {
    if ((Triggers[C].X == X) && (Triggers[C].Y == Y)) {
      Player.X = (Triggers[C].NewX * 16);
      Player.Y = (Triggers[C].NewY * 16);
      Leveldata.Border_left = (Triggers[C].Border_left * 16 + 2);
      Leveldata.Border_right = (Triggers[C].Border_right * 16 - 2);

      if (Triggers[C].NewBg >= 0) {
        SetBg(Triggers[C].NewBg /*,Bg_file_sym->handle*/);
        Leveldata.Bg_offset = (Triggers[C].NewBgOffset * 4);
      };
      Animate_out_of_wrapper(C);
    };
  };
};

void Animate_out_of_wrapper(short T) {
  short C;

  // Player.Attribs = Player.Attribs | 0b00010000;//draw player before fg ON

  Adjust_renderpoint();

  /*if(Triggers[T].Anim == 0){
          //Keystate.Up = 0;
          //Previous_keystate.Up = 1;
          Player.Y -= Player.Height;
  };*/
  char Anim = Triggers[T].Anim;
  if ((/*Triggers[T].*/ Anim == 1) || (/*Triggers[T].*/ Anim == 2)) {

    for (C = 0; C < Leveldata.Nr_of_enemies; C++) {

      //			if( (Enemies[C].X == Player.X) &&
      //(Enemies[C].Sprite >= flower1_sprite) && (Enemies[C].Sprite <=
      // flower2_sprite_right_up) ){
      if ((abs(Enemies[C].X - Player.X) <= 16) &&
          (Enemies[C].Sprite >= flower1_sprite) &&
          (Enemies[C].Sprite <= flower2_sprite_right_up)) {
        Enemies[C].Y = Enemies[C].Y + Enemies[C].Height;
        Enemies[C].Height = Enemies[C].Mode = 0;
      };
    };
  };

  switch (/*Triggers[T].*/ Anim) {
    short Height;
  case 0:
    Player.Y -= Player.Height;
    break;
  case 1:
    // if(Triggers[T].Anim == 1){//up from pipe
    /*short*/ Height = Player.Height;

    Player.X += 8; // trigcomp
    SavePlayer.Spritenr = 9;
    Player.Height = 0;
    // Player.SpriteOffset = Height-1;

    while (Player.Height != Height) {
      // Player.SpriteOffset--;
      Player.Y--;
      Player.Height++;
      Render();
    };
    // Player.Height=Height;

    /*Player.Spritenr = 9;
    //Player.Y += Player.Height;

    for(C=0;C<Player.Height;C++){
            Player.Y--;
            Render();
    };*/

    break; //};
  case 2:
    //	if(Triggers[T].Anim == 2){//down from pipe
    /*short*/ Height = Player.Height;

    Player.X += 8;
    SavePlayer.Spritenr = 9;
    Player.Height = 0;
    Player.SpriteOffset = Height;

    while (Player.Height != Height) {

      Player.SpriteOffset--;
      Player.Height++;

      Render();
    };

    Player.SpriteOffset = 0;
    // Player.Height=Height;

    /*Player.Spritenr = 9;
    Player.Y -= Player.Height;

    for(C=0;C<Player.Height;C++){
                    Player.Y++;
                    Render();
    };*/

    break; //};

  case 3:
    //	if(Triggers[T].Anim == 3){//leftwards out of pipe

    Player.Y -= Player.Height + 2;
    Player.X += 16;
    Player.Face = -1;

    if ((SavePlayer.Spritebase % 2)) {
      SavePlayer.Spritebase -= 1;
      SavePlayer.Maskbase -= 1;
    };
    Player.Blit = 0x1000;
    for (C = 0; C < 16; C++) {
      Player.Blit = Player.Blit >> 1;
      Player.Blit = Player.Blit | 0x8000; // 1
      Player.X--;
      Render();
      if ((++SavePlayer.Spritenr) >= 4) {
        SavePlayer.Spritenr = 0;
      };
    };
    Player.Blit = 0xffff;
    Player.Y += 2;

    break; //};
  case 4:
    //	if(Triggers[T].Anim == 4){//rightwards out of pipe

    Player.Y -= Player.Height + 2;
    Player.X -= 16;
    Player.Face = 1;

    if (!(SavePlayer.Spritebase % 2)) {
      SavePlayer.Spritebase += 1;
      SavePlayer.Maskbase += 1;
    };
    Player.Blit = 0x0001;
    for (C = 0; C < 16; C++) {
      //	long D;
      Player.Blit = Player.Blit << 1; //<<
      Player.Blit = Player.Blit | 0x0001;
      Player.X++;
      Render();
      // for(D=0;D<40000;D++);
      if ((++SavePlayer.Spritenr) >= 4) {
        SavePlayer.Spritenr = 0;
      };
    };
    Player.Blit = 0xffff;
    Player.Y += 2;

    break; //};
  case 5:
    //	if(Triggers[T].Anim == 5){
    Player.Y += 32;

    for (C = 0; C < 16; C++) {

      Player.Y -= 2;
      Render();
    };

    break; //};
  case 6:
    //	if(Triggers[T].Anim == 6){
    Exit = 1;
    break; //};
  case 7:
    //	if(Triggers[T].Anim == 7){
    Exit = 2;
    break; //};

  }; // end switch

  Player.Xoffset = Player.Yoffset = 0;

  // New: V 1.04
  // Clearing all arrow keypresses: Prevents that other triggers is executed
  // when mario comes out of pipes
  Keystate.Down = Keystate.Left = Keystate.Right = 0;
  if (Anim)
    Keystate.Up = 0;

  // Player.Attribs = Player.Attribs & 0b11101111;//draw player before fg OFF
};
/*
void Handle_treasure_star( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = star_anim+32;
        Items[0].Mask = star_mask;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(4);

        Treasure_anim_end();

};

void Handle_treasure_hammer( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = hammer_i_spr;
        Items[0].Mask = hammer_i_msk;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(6);

        Treasure_anim_end();

};



void Handle_treasure_pwing( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;

        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = p_wing_spr;
        Items[0].Mask = p_wing_msk;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(7);

        Treasure_anim_end();

};

void Handle_treasure_whistle( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = whistle_spr;
        Items[0].Mask = whistle_msk;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(5);

        Treasure_anim_end();

};

void Handle_treasure_cloud( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = cloud_item_spr;
        Items[0].Mask = cloud_item_msk;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(8);

        Treasure_anim_end();

};

void Handle_treasure_anchor( short X, short Y ){
        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        Items[0].Active=1;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Sprite = anchor_spr;
        Items[0].Mask = anchor_msk;
        Items[0].Height = Items[0].Height2 = 16;
        Itemlist_add(9);

        Treasure_anim_end();

};

void Handle_treasure_rand( short X, short Y ){
        short H;//C

        //C=Find_free_object();

        //*( (char*)(Fg_plane.p.matrix+(Y/16)*Fg_plane.p.width+(X/16)) )=0;
        Put_tile(X,Y,0);

        H = 16;//Items[0].Height = Items[0].Height2 = 16;

        switch(Fg_plane.step){

                case 0://mushrom
                {
                        Items[0].Sprite = pu_mushrom_sprite;
                        Items[0].Mask = mushrom_mask;
                        //H = Items[0].Height = Items[0].Height2 = 16;
                        Itemlist_add(1);
                        break;
                };
                case 1://flower
                {
                        Items[0].Sprite = flower_spr;
                        Items[0].Mask = flower_msk;
                        //H = Items[0].Height = Items[0].Height2 = 16;
                        Itemlist_add(2);
                        break;
                };
                case 2://leaf
                case 3://leaf
                {
                        Items[0].Sprite = leaf_r_sprite;
                        Items[0].Mask = leaf_r_mask;
                        H = 14;//Items[0].Height = Items[0].Height2 = 14;
                        Itemlist_add(3);
                        break;
                };
        };

        Items[0].Active = 2;
        Items[0].X = X;
        Items[0].Y = Y;
        Items[0].Height = Items[0].Height2 = H;

        Treasure_anim_end();

};*/

void Handle_treasure_all(short X, short Y) {

  unsigned char T = Get_tile(X, Y);
  unsigned char TileAbove = Get_tile(X, Y - 16);
  unsigned char NewTile = 0;

  if ((Leveldata.Background == 6) && (Keystate.Run == 0)) // UGLY CODE !
    return; // This makes sure you have to use the "Run" key to open a treasure
            // box in mushrom houses if not in a mushrom house the box will
            // automaticly open on contact Mushrom house is detected by
            // background == 6

  if ((TileAbove == water) || (TileAbove == water_surface)) {
    NewTile = water;
  }

  Put_tile(X, Y, NewTile); // 0

  short Height = 16;
  //		Items[0].Height = Items[0].Height2 = 16;
  unsigned short *Sprite;

  switch (T) {

  case treasure_rand:

    switch (Fg_plane.step) {

    case 0: // mushrom
    {
      /*Items[0].*/ Sprite = pu_mushrom_sprite;
      // Items[0].Mask = mushrom_mask;
      Itemlist_add(1);
      break;
    };
    case 1: // flower
    {
      /*Items[0].*/ Sprite = flower_spr;
      // Items[0].Mask = flower_msk;
      Itemlist_add(2);
      break;
    };
    case 2: // leaf
    case 3: // leaf
    {
      /*Items[0].*/ Sprite = leaf_r_sprite;
      // Items[0].Mask = leaf_r_mask;
      Height = 14; // Items[0].Height = Items[0].Height2 = 14;
      Itemlist_add(3);
      break;
    };
    };

    break;

  case treasure_star:
    /*Items[0].*/ Sprite = star_anim;
    // Items[0].Mask = star_mask;
    Itemlist_add(4);
    break;

  case treasure_whistle:
    /*Items[0].*/ Sprite = whistle_spr;
    // Items[0].Mask = whistle_msk;
    Itemlist_add(5);
    break;

  case treasure_hammer:
    /*Items[0].*/ Sprite = hammer_i_spr;
    // Items[0].Mask = hammer_i_msk;
    Itemlist_add(6);
    break;

  case treasure_pwing:
    /*Items[0].*/ Sprite = p_wing_spr;
    // Items[0].Mask = p_wing_msk;
    Itemlist_add(7);
    break;

  case treasure_cloud:
    /*Items[0].*/ Sprite = cloud_item_spr;
    // Items[0].Mask = cloud_item_msk;
    Itemlist_add(8);
    break;

  case treasure_anchor:
    /*Items[0].*/ Sprite = anchor_spr;
    // Items[0].Mask = anchor_msk;
    Itemlist_add(9);
    break;
  }

  Items[0].Sprite = Sprite;
  Items[0].Active = 1;
  Items[0].X = X;
  Items[0].Y = Y;
  // New: V 1.04 Removed flashing line at the top of the item
  Items[0].Height = Items[0].Height2 = Height;
  // Treasure_anim_end();
  { // put inline for memory optimization: saved  68 bytes
    short C, H = /*Items[0].*/ Height;

    for (C = 0; C < 70; C++) {

      if (Items[0].Y > (FgY + 30)) {
        Items[0].Y -= 2;
      } else {

        // if(Fg_plane.step%2){//flashing
        //	/*Items[0].*/Height = 1;//0 causes crash...
        // }
        // else{
        //	/*Items[0].*/Height = H;
        // };

        // New: V 1.04 Removed flashing line at the top of the item
        Items[0].Active = Fg_plane.step % 2;
      };

      // Items[0].Height = Items[0].Height2 = Height;

      Render();

      if (Free_down(Player.X, Player.Y + Player.Height - 1))
        Player.Y++;
    };

    Items[0].Active = 0;
    Exit = 2;
    Player.Immortal = 1;
  }
}
/*
void Treasure_anim_end(){
                short C,H = Items[0].Height;

                for(C=0;C<70;C++){

                if(Items[0].Y>(FgY+30)){
                        Items[0].Y-=2;
                }
                else{

                        if(Fg_plane.step%2){//flashing
                                Items[0].Height = 1;//0 causes crash...
                        }
                        else{
                                Items[0].Height = H;
                        };

                };

                Render();

                if( Free_down( Player.X , Player.Y+Player.Height-1 ) )
                        Player.Y++;

        };

        Items[0].Active = 0;
        Exit=2;
};*/

void Itemlist_add(char Item) {
  short C;

  for (C = 0; ((SavePlayer.Itemlist[C] != 0) && (C < (itemlist_length - 1)));
       C++)
    ; // find first free slot or replace the last element

  SavePlayer.Itemlist[C] = Item;
};

void Itemlist_remove(char Index) {
  short C;

  for (C = Index; C < (itemlist_length - 1);
       C++) { // move all elements one step to the left
    SavePlayer.Itemlist[C] = SavePlayer.Itemlist[C + 1];
  };

  SavePlayer.Itemlist[itemlist_length - 1] = 0; // last slot must be empty
};

void Levelend_handler(short X, short Y) {
  short C;
  short P;

  X = X & 0xfff0; //(X/16)*16;
  Y = Y & 0xfff0; //(Y/16)*16;

  SavePlayer.Score += 50; // this is 5000. Score get padded with "00"

  // get card
  char CurrCard = SavePlayer.CurrCard;
  SavePlayer.Cards[(short)/*SavePlayer.*/ CurrCard] = Fg_plane.step;
  if (SavePlayer.Cards[(short)/*SavePlayer.*/ CurrCard] == 3)
    SavePlayer.Cards[(short)/*SavePlayer.*/ CurrCard] = 1;

  for (C = -1; C < 2; C++) {
    for (P = -1; P < 2; P++)
      //*( (char*)(Fg_plane.p.matrix+(Y/16+P)*Fg_plane.p.width+(X/16+C)) ) =
      // dark_gray;
      Put_tile(X + 16 * C, Y + 16 * P, dark_gray);
  };

  Player.Face = 1;
  if (!(SavePlayer.Spritebase % 2)) {
    SavePlayer.Spritebase += 1;
    SavePlayer.Maskbase += 1;
  };

  for (; Player.X < Leveldata.Border_right + 2; Player.X++) {
    Adjust_renderpoint();
    Render();
    if ((++SavePlayer.Spritenr) >= 4) {
      SavePlayer.Spritenr = 0;
    };

    C = Free_down(Player.X, Player.Y + Player.Height);
    if (C > 0) {
      Player.Y += (C >= 2 ? 2 : C);
      SavePlayer.Spritenr = 8;
    };
  };

  /*
  DrawGrayStrExt2B(80,8,"COURSE CLEAR!",A_REVERSE,F_4x6,dBufHPD_G,dBufHPD_G);
  DrawGrayStrExt2B(80,18,"YOU GOT A CARD",A_REVERSE,F_4x6,dBufHPD_G,dBufHPD_G);
  */
  DrawGrayStrExt2B(screen_width - 80, 8, "COURSE CLEAR!", A_REVERSE, F_4x6,
                   dBufHPD_G, dBufHPD_G);
  DrawGrayStrExt2B(screen_width - 80, 18, "YOU GOT A CARD", A_REVERSE, F_4x6,
                   dBufHPD_G, dBufHPD_G);
  // DrawString(screen_width-80,8,"COURSE CLEAR!",A_REVERSE,F_4x6);
  // DrawString(screen_width-80,18,"YOU GOT A CARD",A_REVERSE,F_4x6);

  // show card
  //	GrayClipISprite16_RPLC_R(100,28,16,(unsigned
  // short*)Fg_plane.p.sprites+(79+SavePlayer.Cards[(short)SavePlayer.CurrCard])*32,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
  GrayClipISprite16_RPLC_R(
      screen_width - 60, 28, 16,
      (unsigned short *)Fg_plane.p.sprites +
          (79 + SavePlayer.Cards[(short)/*SavePlayer.*/ CurrCard]) * 32,
      /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
      /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);

  // x up
  if (/*SavePlayer.*/ CurrCard == 2) {

    short Lives;
    if ((SavePlayer.Cards[0] == SavePlayer.Cards[1]) &&
        (SavePlayer.Cards[0] == SavePlayer.Cards[2])) { // 3 equal cards? :-D
      /*if(Player.Cards[0]==0){//flower
              Player.Lives +=3;
              P=24+2*30;

      };
      if(Player.Cards[0]==1){//mushrom
              Player.Lives +=2;
              P=24+30;

      };
      if(Player.Cards[0]==2){//star
              Player.Lives +=5;
              P=24+3*30;
      };*/

      switch (SavePlayer.Cards[0]) {
      case 0:
        Lives = 3; // SavePlayer.Lives +=3;
        P = 24 + 2 * 30;
        break;
      case 1:
        Lives = 2; // SavePlayer.Lives +=2;
        P = 24 + 30;
        break;
      case 2:
        Lives = 5; // SavePlayer.Lives +=5;
        P = 24 + 3 * 30;
        break;
      }

      // show cards

    } else {     // Not 3 equal cards :-(
      Lives = 1; // SavePlayer.Lives ++;
      P = 24;
    };
    SavePlayer.Lives += Lives;

    /*for(X=0;X<SHRT_MAX;Y++){
                    for(Y=0;Y<SHRT_MAX;Y++);
    };*/
    DelayNFrames(DEFAULT_FPS);

    for (C = 0; C <= 2; C++) {
      // GrayClipISprite16_RPLC_R(83+17*C,50,16,(unsigned
      // short*)Fg_plane.p.sprites+(79+SavePlayer.Cards[C])*32,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
      GrayClipISprite16_RPLC_R(
          screen_width - 77 + 17 * C, 50, 16,
          (unsigned short *)Fg_plane.p.sprites +
              (79 + SavePlayer.Cards[C]) * 32,
          /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
          /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
    };

    // GrayClipSprite16_MASK_R(87,44,15,up_sprite,up_sprite+15,blank_sprite,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
    // GrayClipSprite16_SMASK_R(87,44,15,up_sprite,up_sprite+15,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
    // GrayClipSprite16_SMASK_R(screen_width-73,44,15,up_sprite,up_sprite+15,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
    DrawSprite16SMASKR(screen_width - 73, 44, 15, up_sprite, up_sprite + 15,
                       blank_sprite);

    // GrayClipSprite8_MASK_R(75,44,15,Smallsprites+P,Smallsprites+P+15,(unsigned
    // char*)blank_sprite,(unsigned
    // char*)blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
    // GrayClipSprite8_SMASK_R(75,44,15,Smallsprites+P,Smallsprites+P+15,(unsigned
    // char*)blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
    GrayClipSprite8_SMASK_R(screen_width - 85, 44, 15, Smallsprites + P,
                            Smallsprites + P + 15,
                            (unsigned char *)blank_sprite,
                            /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                            /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);

    for (C = 0; C <= 2; C++) {
      SavePlayer.Cards[C] = 0;
    }

    // SavePlayer.Cards[0]=SavePlayer.Cards[1]=SavePlayer.Cards[2]=0;
    SavePlayer.CurrCard = -1;
  };

  GrayDBufToggleSync_SetPointers(); // GrayDBufToggleSync();

  /*
  for(X=0;X<SHRT_MAX;X++){//delay
          for(Y=0;Y<100;Y++);
  };
  */
  DelayNFrames(DEFAULT_FPS);

  Exit = 2; // Exit == 2: level completed
  SavePlayer.CurrCard++;
};

void Add_dustsky(short X, short Y) {
  short C;

  if ((C = Find_free_object()) != -1) {
    Objects[C].Active = 1;
    Objects[C].Data0 = 0;
    Objects[C].X = X;
    Objects[C].Y = Y;
    //(object*)Objects[C].Handler = Dummy_func_;//Handle_killed_fireball;
    Objects[C].Draw = Draw_killed_fireballs;
  };
};