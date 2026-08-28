#include "level.h"
#include "bosses.h"
#include "comp.h"
#include "compat/alloc.h"
#include "compat/tios.h"
#include "control.h"
#include "flying.h"
#include "gfx.h"
#include "items.h"
#include "levelset.h"
#include "map.h"
#include "objects.h"
#include "player.h"
#include "render.h"
#include "rle.h"
#include "savegame.h"
#include "shells.h"
#include <stdlib.h>
#include <string.h>

leveldata Leveldata;
map_data Map_data;
struct levelsetdata Levelsetdata;

char Levelfilename[9];
char Commonfilename[9];

// Byte-swap a level header just read out of a level file.
//
// This is the one structure tools/mkdata.py cannot convert at build time,
// because it cannot tell where the headers are. levelfiledata.Nr_of_levels
// would be the obvious answer and is wrong: it says 1 for common.9xy, which
// holds at least eight levels - the title screen is level 7 of it - and 2 for
// world5. Nothing writes the field and the game never reads it, indexing
// Levels[] directly, so it was free to rot. The trailing entries of Levels[]
// are junk for the same reason, which rules out simply converting all twenty.
//
// The loader, by contrast, knows exactly which level it was asked for. It also
// swaps the copy rather than the file, so loading a level twice is safe.
//
// Only the 16-bit fields move; Bg_scrollrate, Background, Event, Condition, EX
// and EY are bytes. The compressed foreground and the enemy, trigger and
// platform arrays that follow the header are byte data too, which is why
// mkdata.py leaves the level payloads alone.
static void Swap_leveldata(leveldata *Data) {

  Data->Width = be16(Data->Width);
  Data->Height = be16(Data->Height);
  Data->Bg_height = be16(Data->Bg_height);
  Data->Border_left = be16(Data->Border_left);
  Data->Border_right = be16(Data->Border_right);
  Data->PlayerX = be16(Data->PlayerX);
  Data->PlayerY = be16(Data->PlayerY);
  Data->Boss = be16(Data->Boss);
  Data->Nr_of_enemies = be16(Data->Nr_of_enemies);
  Data->Nr_of_triggers = be16(Data->Nr_of_triggers);
  Data->Nr_of_flying_platforms = be16(Data->Nr_of_flying_platforms);
  Data->Bg_offset = be16(Data->Bg_offset);
  Data->Size = be16(Data->Size);
  Data->TCSize = be16(Data->TCSize);
  Data->Spare1 = be16(Data->Spare1);
  Data->Spare2 = be16(Data->Spare2);
}

short Load_level(char *Levelfile, short Level_nr) {
  SYM_ENTRY *Levelfile_sym;
  HANDLE Temp;
  levelfiledata *Levelfileinfo;

  // char buffer[20];
  short C, Byte;

  if (!Skip_anim) {
    Enter_level_anim();
  };

  Skip_anim = 0;

  //	SavePlayer.Spritebase += (Player.Face%2?0:1);
  //	Player.Face = 1;
  Player.Face = (SavePlayer.Spritebase % 2 ? 1 : -1);
  SavePlayer.Spritenr = 0;

  SavePlayer.Attribs =
      SavePlayer.Attribs & 0b11101111; // turn of "draw mario before fg"
  Player.Xoffset = Player.Yoffset = 0;

  if (Player.Immortal !=
      star_immortal_time) { // if star not enabled through itemlist
    Player.Immortal = 0;
    SavePlayer.Attribs = SavePlayer.Attribs & 0b01111111; // disable star
  };

  // delete all objects such as shells, items, fireballs etc.
  // mod this to use memset!!!

  // Not useable yet, need more modifications
  // memset(Items,0,sizeof(item)*nr_of_items+sizeof(shell)*max_nr_of_shells+sizeof(object)*nr_of_objects);

  for (C = 0;
       C < sizeof(item) * nr_of_items + sizeof(shell) * max_nr_of_shells +
               sizeof(object) * nr_of_objects;
       C++)
    *(((char *)Items) + C) = 0;

  /*
  for(C=0;C<nr_of_items;C++)
          Items[C].Active = 0;

  for(C=0;C<max_nr_of_shells;C++)
          Shells[C].Active = 0;

  for(C=0;C<nr_of_objects;C++)
          Objects[C].Active = 0;
  */
  for (C = 0; C < nr_of_enemy_smallshots; C++)
    Enemyshots[C].Mode = 0;

  for (C = 0; C < nr_of_player_smallshots; C++)
    Player_smallshots[C].Mode = 0;

  // Error: Level not found possible
  /*
  if( !(Levelfile_sym = SymFindPtr (SYMSTR(Levelfile), 4)) ){//flags = 4 will
change when the documentation of VAT.h is updated ErrorCode = 3; Exit = 3;
  return 3;//error "failed to open level file"
};
if( !(Temp = Levelfile_sym->handle) ){
  ErrorCode = 3;
  Exit = 3;
  return 3;//error "failed to open level file"
};
  if( !(HeapLock( Temp )) ){//lock the file temporarly
          ErrorCode = 3;
  Exit = 3;
          return 3;//locking failed!
  };
          */

  if (!(Temp = File_get_pointer_and_lock(Levelfile /*,Levelfile_sym*/))) {
    ErrorCode = 3;
    Exit = 3;
    return 3; // error "failed to open level file"
  }

  void *RawData = HeapDeref(Temp) + 2;
  //	memcpy( &Levelfileinfo, HeapDeref (Temp)+2, sizeof(levelfiledata) );
  Levelfileinfo = (levelfiledata *)(/*HeapDeref (Temp)+2*/ RawData);
  memcpy(&Leveldata,
         /*HeapDeref (Temp)+2*/ RawData + Levelfileinfo->Levels[Level_nr],
         sizeof(leveldata));

  Swap_leveldata(&Leveldata);

  /*if(Level)//if level have already been used
          free(Level);*/
  Free(Level);

  // Error: Memory possible

  if (!(Level = malloc(Leveldata.Size))) {
    ErrorCode = 1;
    Exit = 3;
    return 1;
  }

  memset(Level, 0, Leveldata.Size); // temp ?

  /*
  char buffer[20];
  sprintf (buffer, "%u %u",Leveldata.Nr_of_enemies,Leveldata.Nr_of_triggers);
  DrawGrayStrExt2B(17,15,buffer,A_REPLACE,F_6x8,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  sprintf (buffer, "%u %u",Leveldata.Nr_of_flying_platforms,Leveldata.Size);
  DrawGrayStrExt2B(17,30,buffer,A_REPLACE,F_6x8,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  sprintf (buffer, "%u %u",Leveldata.TCSize,Leveldata.Width);
  DrawGrayStrExt2B(17,45,buffer,A_REPLACE,F_6x8,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayDBufToggleSync();

  //while(1);

  while(_rowread(0));//wait until key released
  while(!_rowread(0));//wait until key pressed
  */
  // Leveldata.Height*Leveldata.Width + ((Leveldata.Height*Leveldata.Width)%2)

  /*
  DrawGrayStrExt2B(17,45,"Hei!",A_REPLACE,F_6x8,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayDBufToggleSync();

  while(1);
  */

  void *Raw = /*HeapDeref (Temp)+2*/ RawData + Levelfileinfo->Levels[Level_nr] +
              sizeof(leveldata);

  // RLEDecompress(Level, HeapDeref (Temp)+2 + Levelfileinfo.Levels[Level_nr] +
  // sizeof(leveldata),/*(int)*/(Leveldata.Height*Leveldata.Width));//+(Leveldata.Height*Leveldata.Width)%2
  RLEDecompress(Level, Raw, (Leveldata.Height * Leveldata.Width));

  Raw += Leveldata.TCSize;

  // memcpy( Level, HeapDeref (Temp)+2 + Levelfileinfo.Levels[Level_nr] +
  // sizeof(leveldata), Leveldata.Size );

  // memcpy( Level+Leveldata.Height*Leveldata.Width +
  // ((Leveldata.Height*Leveldata.Width)%2), HeapDeref (Temp)+2 +
  // Levelfileinfo.Levels[Level_nr] + sizeof(leveldata)+Leveldata.TCSize,
  // Leveldata.Size - Leveldata.Height*Leveldata.Width -
  // ((Leveldata.Height*Leveldata.Width)%2) );

  // void* Raw = HeapDeref (Temp)+2 + Levelfileinfo.Levels[Level_nr] +
  // sizeof(leveldata) + Leveldata.TCSize;

  // sprintf (buffer, "%d   %d   %d   %d
  // %d",Levelfileinfo.Levels[0],Leveldata.Height,Leveldata.Width,Leveldata.Bg_scrollrate,Leveldata.Size);//test
  // DrawStr (0, 30, buffer, A_NORMAL);//test
  // while(1);

  Fg_plane.p.matrix = Fg_mask.p.matrix = Level;
  Fg_plane.p.width = Fg_mask.p.width = Leveldata.Width;
  Update_FG = 1; // Fg_plane.p.force_update = Fg_mask.p.force_update = 1;

  /*
  if( !(Bg_file_sym = SymFindPtr (SYMSTR("ma_bckgr"), 4)) )//flags = 4 will
change when the documentation of VAT.h is updated return -4;//error "failed to
open bg file"

if( !(Temp = Bg_file_sym->handle) )
    return -4;//error "failed to open bg file"

  if( !(HeapLock( Bg_file_sym->handle )) )
          return -111;
  */

  SetBg((short)Leveldata.Background /*,Bg_file_sym->handle*/);
  /*
  memcpy(&Bg_fileinfo,HeapDeref (Temp)+2,sizeof(bg_filedata));
  memcpy(&Bg_data,HeapDeref
(Temp)+2+Bg_fileinfo.Backgrounds[(short)Leveldata.Background],sizeof(bg_data));

  Bg_plane.matrix = Background = HeapDeref
(Temp)+2+Bg_fileinfo.Backgrounds[(short)Leveldata.Background]+sizeof(bg_data);
Leveldata.Bg_width = Bg_plane.width = Bg_data.Width;
  Bg_plane.force_update = 1;
  Leveldata.Bg_height = Bg_data.Height;
  */
  Player.X = Leveldata.PlayerX;
  Player.Y = Leveldata.PlayerY - Player.Height;

  Enemies = (struct enemy *)(Level + Leveldata.Height * Leveldata.Width +
                             ((Leveldata.Height * Leveldata.Width) %
                              2)); // last statement prevents address errors
                                   // when foreground size is an odd number

  // memset(Enemies,0,sizeof(enemy)*Leveldata.Nr_of_enemies);

  short Nr = 0;
  short NextRaw;
  char Model;

  unsigned char Byte3, Byte4, Byte5;

  unsigned short *Sprite;
  void *Handler;
  void *Die;
  unsigned char Attribs;
  char Face;
  char Life;

  while (Nr < Leveldata.Nr_of_enemies) {

    Model = *((char *)Raw);

    Enemies[Nr].X = (*((unsigned char *)Raw + 1)) * 16; // COMMON FOR ALL
    Enemies[Nr].Y = (*((unsigned char *)Raw + 2)) * 16; // COMMON FOR ALL
    Enemies[Nr].Active = 1;
    Enemies[Nr].Mode = 1;
    Enemies[Nr].Life = 1; // keep this, avoids bugs!
    NextRaw = 3;

    Byte3 = (*((unsigned char *)Raw + 3));
    Byte4 = (*((unsigned char *)Raw + 4));
    Byte5 = (*((unsigned char *)Raw + 5));
    /*
    Enemies[Nr].Jumping = 0;
    Enemies[Nr].Data0 = Enemies[Nr].Data1 = 0;
    */
    Attribs = 0;

    Life = 0;
    Face = 0;

    if (Model < 0) {
      // respawn
      Model = abs(Model);

      Enemies[Nr].RespawnX = Enemies[Nr].X >> 4; /// 16;
      Enemies[Nr].RespawnY = Enemies[Nr].Y >> 4; /// 16;
      Enemies[Nr].Respawn = Model;               //(*((char*)Raw+3));
      Enemies[Nr].Respawn2 = Byte4;              //(*((unsigned char*)Raw+4));
    }

    // start of "handler 1 enemies"
    /*  	if( (Model<=20) && (Model>0) ){
                    (enemy *)Enemies[Nr].Handler = Enemy_handler_1;//common for
       all 1-20 Enemies[Nr].Face = (*((char*)Raw+3));//common for all 1-20
                    Enemies[Nr].Height = Enemies[Nr].Height2 = 16;//common for
       most 1-20

                    Enemies[Nr].Attribs = 0b11111001;

                    NextRaw += 1;

                    if( !(Model%2) ){
                            Enemies[Nr].Data0 = Enemies[Nr].Data1 = (
       (*((unsigned char*)Raw+4))*16 );//common for all 1-20
                            if(Enemies[Nr].Data0==0){//if zero: Mode 3: walk to
       the end and turn Enemies[Nr].Active = 3;
                                    //Enemies[Nr].Data0 = Enemies[Nr].Data1 =
       0;//TEST
                            }
                            else{
                                    Enemies[Nr].Active = 2;
                            }

                            NextRaw += 1;
                    }
            };
    */  		//end of "handler 1 enemies"

    short Height1OK =
        0; // this tells whether or not Height2 shall be copied to Height

    if ((Model <= 20) && (Model > 0)) {

      NextRaw = Generate_handler_1_enemy(
          Nr, Model, (char)Byte3,
          Byte4); //(*((char*)Raw+3)) , (*((unsigned char*)Raw+4)) );
    }

    if ((Model >= 23) && (Model <= 30)) { // common for all flowers
      /*Enemies[Nr].*/ Attribs = 0b01010001;
      /*Enemies[Nr].*/ Die = Enemy_die_8;
      Enemies[Nr].Height2 = 32;
      Height1OK = 1;
      // Enemies[Nr].Height = 0;
      Enemies[Nr].X += 8;
    }

    if ((Model >= 31) &&
        (Model <= 42)) { // common for all enemies using "boomerang guy" sprite
      /*Enemies[Nr].*/ Sprite = boomerang_guy_left_sprite;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 23;
      Enemies[Nr].Y -= 23;
      /*Enemies[Nr].*/ Attribs = 0b11111001;
      /*Enemies[Nr].*/ Die = Enemy_die_6;
      /*Enemies[Nr].*/ Face = -1;
      Enemies[Nr].Mode = 0;

      //	Enemies[Nr].

      //  		Byte = (*((unsigned char*)Raw+3));//((unsigned
      //  char)(*(Raw+3)));
      if ((Model % 2) == 0) { // treasure
        /*Enemies[Nr].*/ Attribs = 0b11110001;
        //((unsigned char)(*(Raw+3)));
        /*Enemies[Nr].*/ Life = (Byte3 & 0b01111111);
        NextRaw = 4;
        /*Enemies[Nr].*/ Die = Enemy_die_10;
        Enemies[Nr].Active = 4;
      }
      if ((Model % 4) < 2) { // brother
        //(enemy *)Enemies[Nr].Die = Enemy_die_10;
        Enemies[Nr].Active = ((Byte3 & 0b10000000) ? 1 : 3);
        /*Enemies[Nr].*/ Attribs = 0b11110001;
        NextRaw = 4;
        /*Enemies[Nr].*/ Die = Enemy_die_10;
      }

    } // end boomerang

    //	Player.Test = Model;//Enemies[Nr].Data0;//

    switch (Model) {
      // start of "handler 1 enemies"
      /*  		case goomba_free:
                      case goomba_limited:
                              Enemies[Nr].Sprite = monster1_sprite;
                              //Enemies[Nr].Height = (Enemies[Nr].Height2 -= 1);
                              Enemies[Nr].Height = Enemies[Nr].Height2 = 15;
                              Enemies[Nr].Y += 1;
                              Enemies[Nr].Mode = 0;
                              (enemy *)Enemies[Nr].Die = Enemy_die_1;
                      break;
                      case turtle_free:
                      case turtle_limited:
                              Enemies[Nr].Sprite = turtle_right_sprite;
                              (enemy *)Enemies[Nr].Die = Enemy_die_2;
                      break;
                      case hedgehog_free:
                      case hedgehog_limited:
                              Enemies[Nr].Sprite = hedgehog_left_spr;
                              Enemies[Nr].Height = Enemies[Nr].Height2 = 15;
                              Enemies[Nr].Y += 1;
                              Enemies[Nr].Attribs = 0b00111001;
                              (enemy *)Enemies[Nr].Die =
         Dummy_func_;//E_dummy_handler; break; case beetle_free: case
         beetle_limited: Enemies[Nr].Sprite = beetle_left_sprite; (enemy
         *)Enemies[Nr].Die = Enemy_die_2; Enemies[Nr].Life = 2;
                              Enemies[Nr].Attribs = 0b10111000;
                      break;
                      case fish_free:
                      case fish_limited:
                              Enemies[Nr].Sprite = fish1_left_spr;
                              Enemies[Nr].Attribs = 0b01011001;
                              Enemies[Nr].Y -= 2;
                              (enemy *)Enemies[Nr].Die =
         Dummy_func_;//E_dummy_handler; break;

                      case skel_tur_free:
                      case skel_tur_limited:
                              Enemies[Nr].Y -= 27;
                                      Enemies[Nr].Height2 = Enemies[Nr].Height =
         27; Enemies[Nr].Sprite = turtle_skeleton_left_sprite;
                                      Enemies[Nr].Attribs = 0b10110000;
                                      (enemy *)Enemies[Nr].Die = Enemy_die_7;
                      break;
      */  			//end of "handler 1 enemies"

    /*
    case 21://flying_goomba:
    case 22:
            Enemies[Nr].Face = (*((char*)Raw+3));//((char)(*(Raw+3)));
            Enemies[Nr].Sprite = monster1_wings1_sprite;
            Enemies[Nr].Height = Enemies[Nr].Height2 = 22;
            Enemies[Nr].Life = 2;
            Enemies[Nr].Jumping = 1;
            (enemy *)Enemies[Nr].Handler = Enemy_handler_4;
            Enemies[Nr].Attribs = 0b11111001;
            (enemy *)Enemies[Nr].Die = Enemy_die_3;

            NextRaw += 1;

    break;
            */
    case flower1_upside_down:
      /*Enemies[Nr].*/ Life = -1;
    /*case flower1-3:
    case flower1-2:*/
    case flower1:
      /*Enemies[Nr].*/ Sprite = flower1_sprite;
      /*Enemies[Nr].*/ Handler = Enemy_handler_2;
      break;

    case flower2_upside_down:
      /*Enemies[Nr].*/ Life = -1;
    /*case flower2-3:
    case flower2-2:*/
    case flower2:
      /*Enemies[Nr].*/ Sprite = flower2_sprite_left_down;
      /*Enemies[Nr].*/ Handler = Enemy_handler_3;
      /*Enemies[Nr].Sprite = flower1_sprite;
      (enemy *)Enemies[Nr].Handler = Enemy_handler_2;*/
      break;

    case boomerang_guy_s:
    case boomerang_guy_b_t:
    case boomerang_guy_b:
    case boomerang_guy_t:
      Enemies[Nr].Data0 = Enemies[Nr].Data1 = 48;
      /*Enemies[Nr].*/ Handler = Enemy_handler_9;
      break;

    case fireball_guy_s:
    case fireball_guy_b_t:
    case fireball_guy_b:
    case fireball_guy_t:
      Enemies[Nr].Data1 = 48;
      /*Enemies[Nr].*/ Handler = Enemy_handler_18;
      break;

    case hammerman_s:
    case hammerman_b_t:
    case hammerman_b:
    case hammerman_t:
      Enemies[Nr].Data1 = 1; // is this really needed???
      /*Enemies[Nr].*/ Handler = Enemy_handler_22;
      break;

    /*case lavaball://Jumping fireball
            //Enemies[Nr].Sprite = ;
            //(enemy *)Enemies[Nr].Handler = Enemy_handler_x;

            NextRaw += 3;
    break;
            */
    case orb: // orb
      /*Enemies[Nr].*/ Sprite = circulating_fireball_sprite;
      Enemies[Nr].X -= 8;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 16;
      /*Enemies[Nr].*/ Face = -1;
      Enemies[Nr].Mode = 60;
      Enemies[Nr].Data0 = 14;
      Enemies[Nr].Data1 = 29;
      Enemies[Nr].Jumping = -1;
      /*Enemies[Nr].*/ Attribs = 0b00010000;
      /*Enemies[Nr].*/ Handler = Enemy_handler_11;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      break;

    case falling_brick: // Falling brick
      /*Enemies[Nr].*/ Sprite = falling_brick_sprite;
      /*Enemies[Nr].*/ Attribs = 0b00010000;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 19;
      /*Enemies[Nr].*/ Face = 3;
      Enemies[Nr].Mode = 1;
      Enemies[Nr].Y -= 2;
      /*Enemies[Nr].*/ Handler = Enemy_handler_12;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      break;

    case jumping_brick: // Jumping brick
      /*Enemies[Nr].*/ Sprite = jumping_brick_spr;
      Enemies[Nr].Height2 = 21;
      Enemies[Nr].Height = 16;
      /*Enemies[Nr].*/ Attribs = 0b10010101;
      /*Enemies[Nr].*/ Die = Enemy_die_9;
      /*Enemies[Nr].*/ Handler = Enemy_handler_13;
      // Enemies[Nr].Face = -1;
      // Enemies[Nr].Mode = 0;
      Height1OK = 1;
      break;

    case horizontal_cannon: // Horizontal cannon
      Enemies[Nr].X += 8;
      /*Enemies[Nr].*/ Face =
          Byte3; //(*((unsigned char*)Raw+3));//((unsigned char)(*(Raw+4)));
      Enemies[Nr].Data0 =
          Byte4; //(*((unsigned char*)Raw+4));
                 //  			Enemies[Nr].Attribs = 0b00000000;//not
                 //  neccessarry, done in memset
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      /*Enemies[Nr].*/ Handler = Enemy_handler_14;

      NextRaw = 5;
      break;

    case diagonal_cannon: // Diagonal cannon
      Enemies[Nr].Data0 =
          Byte3; //(*((unsigned char*)Raw+3));//((unsigned char)(*(Raw+3)));

      //  			Byte = (*((unsigned char*)Raw+4));
      Enemies[Nr].Data1 = ((Byte4 & 0b00001111) - 8);
      /*Enemies[Nr].*/ Face = ((Byte4 >> 4) - 8);

      //				Enemies[Nr].Attribs = 0b00000000;//not
      // neccessarry, done in memset
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      /*Enemies[Nr].*/ Handler = Enemy_handler_15;

      NextRaw = 5;
      break;

    case ghost_limited: // Ghost limited

      /*Enemies[Nr].*/ Life = Byte3;  //(*((unsigned char*)Raw+3));
      Enemies[Nr].Data1 = Byte4 * 16; //( (*((unsigned char*)Raw+4)) *16);
      Enemies[Nr].Data0 = Enemies[Nr].Data1 / 2;
      NextRaw = 5;
    case ghost_free: // Ghost unlimited

      if (Model == 54) {
        /*Enemies[Nr].*/ Life = 4;
        // NextRaw = 3;
      }
      /*Enemies[Nr].*/ Sprite = ghost_waiting_left_spr;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 16;
      /*Enemies[Nr].*/ Attribs = 0b00010000;

      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      /*Enemies[Nr].*/ Handler = Enemy_handler_16;

      break;

    case lavaball:
      /*Enemies[Nr].*/ Face = Byte3; //(*((unsigned char*)Raw+3));
      if (/*Enemies[Nr].*/ Face & 0b10000000)
        Enemies[Nr].X -= 8;
      if (/*Enemies[Nr].*/ Face & 0b01000000)
        Enemies[Nr].X += 8;
      if (/*Enemies[Nr].*/ Face & 0b00100000)
        Enemies[Nr].Y -= 8;
      if (/*Enemies[Nr].*/ Face & 0b00010000)
        Enemies[Nr].Y += 8;
      /*Enemies[Nr].*/ Face = ((/*Enemies[Nr].*/ Face & 0b00001111) - 8);
      /*Enemies[Nr].*/ Life = (/*Enemies[Nr].*/ Face > 0 ? 1 : -1);
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 18;
      /*Enemies[Nr].*/ Sprite = lavaball_step_2_sprite;
      Enemies[Nr].Mode = 10;
      Enemies[Nr].Data0 = Byte4; //(*((unsigned char*)Raw+4));
      Enemies[Nr].Data1 = Byte5; //(*((unsigned char*)Raw+5));
      /*Enemies[Nr].*/ Handler = Enemy_handler_10;
      /*Enemies[Nr].*/ Attribs = 0b00010000;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;

      NextRaw = 6;
      break;

    case mad_flower: // Mad flower jumping
      Enemies[Nr].Mode = 0;
    case mad_flower_walking: // Mad flower walking and jumping

      //				Enemies[Nr].Y += 1;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 15;
      /*Enemies[Nr].*/ Sprite = mad_flower_o_right_spr;
      /*if(Model==mad_flower)
              Enemies[Nr].Mode = 0;*/
      /*Enemies[Nr].*/ Handler = Enemy_handler_17;
      /*Enemies[Nr].*/ Attribs = 0b01111001;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      break;

    case flying_turtle: // flying turtle
      /*Enemies[Nr].*/ Sprite = turtle_fly1_left_sprite;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 27;
      Enemies[Nr].Y -= 27;
      Enemies[Nr].Mode = Byte3; //(*((unsigned char*)Raw+3));
      Enemies[Nr].Data0 = Enemies[Nr].Data1 =
          Byte4 * 16; //( (*((unsigned char*)Raw+4)) *16);
      Enemies[Nr].Jumping = 1;
      /*Enemies[Nr].*/ Face = -1; //!!!!!!!!!!!!!! more !!!!!!!!!!!!!!!!!!!!!!
      /*Enemies[Nr].*/ Handler = Enemy_handler_8;
      /*Enemies[Nr].*/ Attribs = 0b11111001;
      /*Enemies[Nr].*/ Die = Enemy_die_4;

      NextRaw += 2;
      break;

    case bottom_flower:
      Enemies[Nr].Y -= 28;
      Enemies[Nr].Height = Enemies[Nr].Height2 = 28;
      Enemies[Nr].Sprite = bottom_flower_l1;
      // Enemies[Nr].Mode = 1;
      Enemies[Nr].Data0 = 10;
      Enemies[Nr].Handler = Enemy_handler_23;
      Enemies[Nr].Attribs = 0b00010000;
      Enemies[Nr].Die =
          (void (*)(struct enemy *))Dummy_func_; // E_dummy_handler;

      Enemies[Nr + 1].X = Enemies[Nr].X + 16;
      Enemies[Nr + 1].Y = Enemies[Nr].Y;
      /*Enemies[Nr+1].Height =*/Enemies[Nr + 1].Height2 = 28;
      /*Enemies[Nr+1].*/ Sprite = bottom_flower_r1;
      /*Enemies[Nr+1].*/ Handler = Enemy_handler_23;
      /*Enemies[Nr+1].*/ Attribs = 0b00010000;
      /*Enemies[Nr+1].*/ Die = Dummy_func_; // E_dummy_handler;
      Enemies[Nr + 1].Active = Enemies[Nr + 1].Life = 1;

      Nr++;

      break;

    case flying_fish:
      /*Enemies[Nr].*/ Sprite = flying_fish_left_spr;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 16;
      /*Enemies[Nr].*/ Face = Byte3; //(*((unsigned char*)Raw+3));
      /*Enemies[Nr].*/ Life = Byte4; //(*((unsigned char*)Raw+4));
      Enemies[Nr].Mode = Byte5 * 16; //( (*((unsigned char*)Raw+5)) *16);
      Enemies[Nr].Data0 = Enemies[Nr].Data1 =
          ((*((unsigned char *)Raw + 6)) * 16);
      // 			Enemies[Nr].Jumping = 0;//not neccessarry, done
      // in memset
      /*Enemies[Nr].*/ Handler = Enemy_handler_19;
      /*Enemies[Nr].*/ Attribs = 0b01011101;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;

      NextRaw = 7;
      break;

    case flying_goomba:
      Enemies[Nr].Y -= 22;
      /*Enemies[Nr].*/ Life = 2;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 22;
      /*Enemies[Nr].*/ Sprite = monster1_wings1_sprite;
      /*Enemies[Nr].*/ Face = -1;
      Enemies[Nr].Mode = 0;
      Enemies[Nr].Jumping = 1;
      /*Enemies[Nr].*/ Handler = Enemy_handler_4;
      /*Enemies[Nr].*/ Attribs = 0b11111001;
      /*Enemies[Nr].*/ Die = Enemy_die_3;
      break;

    case jellyfish:
      // Enemies[Nr].Y += 3;
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 13;
      Enemies[Nr].Y += 3;
      /*Enemies[Nr].*/ Sprite = jellyfish_spr;
      Enemies[Nr].Active = 2;
      /*Enemies[Nr].*/ Face = -1;
      /*Enemies[Nr].*/ Handler = Enemy_handler_20;
      /*Enemies[Nr].*/ Attribs = 0b01011001;
      /*Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
      break;

    case bomb: // bomb

      Enemies[Nr].Data0 = Enemies[Nr].Data1 =
          Byte4 * 16; //( (*((unsigned char*)Raw+4))*16 );
                      //  			Enemies[Nr].Active = 1;

      Life = Enemies[Nr].Life = Byte5;  //(*((unsigned char*)Raw+5));
      if (/*Enemies[Nr].*/ Life == 4) { // falling
        /*Enemies[Nr].Respawn2 = 4;
        Enemies[Nr].Respawn = -bomb;*/
        Spawn_bomb2(&Enemies[Nr]);
      }
      /*Enemies[Nr].Height =*/Enemies[Nr].Height2 = 16;
      /*Enemies[Nr].*/ Face = Byte3; //(*((unsigned char*)Raw+3));
      /*Enemies[Nr].*/ Sprite =
          (Face < 0 ? bomb_walk_left_spr : bomb_walk_right_spr);
      /*Enemies[Nr].*/ Handler = Enemy_handler_21;
      /*Enemies[Nr].*/ Attribs = 0b10111001;
      /*Enemies[Nr].*/ Die = Enemy_die_11;
      if (/*Enemies[Nr].*/ Life == 8) { // shot out from cannon
        // 					Enemies[Nr].RespawnX =
        // Enemies[Nr].X>>4;
        // Enemies[Nr].RespawnY = Enemies[Nr].Y>>4;
        /*	Enemies[Nr].Respawn2 = Enemies[Nr].Face;
                Enemies[Nr].Face = 0;
// 					Enemies[Nr].Y -= 2;
                Enemies[Nr].Attribs = 0b10011001;
                Enemies[Nr].Height = 1;
                //Enemies[Nr].Active = 2;*/
        Enemies[Nr].Respawn2 = /*Enemies[Nr].*/ Face;
        // Enemies[Nr].Respawn = -bomb;
        //	Spawn_bomb(Nr);
        Spawn_bomb(&Enemies[Nr]);
      }
      NextRaw = 6;
      break;
    }

    if (Height1OK == 0) {
      Enemies[Nr].Height = Enemies[Nr].Height2;
    }

    if (Enemies[Nr].Sprite == NULL) {
      Enemies[Nr].Sprite = Sprite;
    }
    if (Enemies[Nr].Handler == NULL) {
      Enemies[Nr].Handler = Handler;
    }
    if (Enemies[Nr].Die == NULL) {
      Enemies[Nr].Die = Die;
    }
    if (Enemies[Nr].Attribs == 0) {
      Enemies[Nr].Attribs = Attribs;
    }
    if (Life) {
      Enemies[Nr].Life = Life;
    }
    if (Face) {
      Enemies[Nr].Face = Face;
    }

    Raw += NextRaw;
    Nr++;
  }

  // Leveldata.Nr_of_triggers =
  // Leveldata.Nr_of_flying_platforms =
  // Leveldata.Boss = 0;

  Triggers = (void *)Enemies +
             Leveldata.Nr_of_enemies *
                 sizeof(struct enemy); // + Leveldata.Nr_of_triggers%2

  memcpy(Triggers, Raw,
         Leveldata.Nr_of_triggers * sizeof(trigger)); // copy triggers

  // Player.Test = Triggers[0].X;

  Raw += Leveldata.Nr_of_triggers * sizeof(trigger);

  Flying_platforms = (void *)Triggers +
                     Leveldata.Nr_of_triggers * sizeof(trigger) +
                     (Leveldata.Nr_of_triggers % 2); //

  // memset(Flying_platforms,0,sizeof(flying_platform)*Leveldata.Nr_of_flying_platforms);

  Nr = 0;

  while (Nr < Leveldata.Nr_of_flying_platforms) {

    Model = *((char *)Raw);

    if (Model > 0) {

      Flying_platforms[Nr].X =
          (*((unsigned char *)Raw + 1)) * 16; // COMMON FOR ALL
      Flying_platforms[Nr].Y =
          (*((unsigned char *)Raw + 2)) * 16; // COMMON FOR ALL
      Flying_platforms[Nr].Width =
          (*((unsigned char *)Raw + 3)); // COMMON FOR ALL
      Flying_platforms[Nr].Active = 1;
      Flying_platforms[Nr].Sprite = 100; // platform_sprite;

      switch (Model) {

      case 11:
      case 12:

        Flying_platforms[Nr].Data0 = (*((unsigned char *)Raw + 4));
        Flying_platforms[Nr].Data2 = (*((char *)Raw + 5));
        Flying_platforms[Nr].Data3 = (*((char *)Raw + 6));
        Flying_platforms[Nr].Handler = Platform_handler_1;
        NextRaw = 7;
        if (Model == 11) {
          Flying_platforms[Nr].Data0 = Flying_platforms[Nr].Data0 * 16;
        } else { // Model==12
          Flying_platforms[Nr].Y += (*((char *)Raw + 7));
          NextRaw = 8;
        }
        break;

      case 21:
        Flying_platforms[Nr].Handler = Platform_handler_2;

        NextRaw = 4;
        break;
      }

      /*if( (Model==11) ){

              Flying_platforms[Nr].Data0 = (*((unsigned char*)Raw+4))*16;
              Flying_platforms[Nr].Data2 = (*(( char*)Raw+5));
              Flying_platforms[Nr].Data3 = (*(( char*)Raw+6));

              //Flying_platforms[Nr].Sprite = 100;//platform_sprite;
              (flying_platform*)Flying_platforms[Nr].Handler =
      Platform_handler_1;

              NextRaw = 7;
      }
      if( (Model==12) ){
              Flying_platforms[Nr].Data0 = (*((unsigned char*)Raw+4));
              Flying_platforms[Nr].Data2 = (*(( char*)Raw+5));
              Flying_platforms[Nr].Data3 = (*(( char*)Raw+6));

              Flying_platforms[Nr].Y += (*(( char*)Raw+7));//was 6
              //Flying_platforms[Nr].Sprite = 100;//platform_sprite;
              (flying_platform*)Flying_platforms[Nr].Handler =
      Platform_handler_1;

              NextRaw = 8;
      }

      if( (Model==21) ){

              //Flying_platforms[Nr].Sprite = 100;//platform_sprite;
              (flying_platform*)Flying_platforms[Nr].Handler =
      Platform_handler_2;

              NextRaw = 4;
      }*/

      //(long)Flying_platforms[Nr].Sprite = platform_sprite;
      // Flying_platforms[Nr].Data0 = Flying_platforms[Nr].Data1 =
      // Flying_platforms[Nr].Data2 = Flying_platforms[Nr].Data3 = 0;
      //(flying_platform*)Flying_platforms[Nr].Handler = Platform_handler_2;

    } else { // dummy platform

      NextRaw = 1;
    }

    Raw += NextRaw;

    Nr++;
  }

  /*
  for(C=0;C<Leveldata.Nr_of_flying_platforms;C++){
          //Flying_platforms[C].Sprite = Sprites +
  (long)Flying_platforms[C].Sprite;
          //Flying_platforms[C].Mask = blank_sprite;
          if((long)Flying_platforms[C].Handler == 1)
                  (flying_platform*)Flying_platforms[C].Handler =
  Platform_handler_1; if((long)Flying_platforms[C].Handler == 2)
                  (flying_platform*)Flying_platforms[C].Handler =
  Platform_handler_2;
  };
  */
  // Player.Test = Leveldata.Boss;
  BossG = (void *)Flying_platforms +
          Leveldata.Nr_of_flying_platforms * sizeof(struct flying_platform);
  Nr = 0;
  while (Nr < Leveldata.Boss) {
    // if(Leveldata.Boss){
    Model = *((char *)Raw);

    BossG->X = (*((unsigned char *)Raw + 1)) * 16; // COMMON FOR ALL
    BossG->Y = (*((unsigned char *)Raw + 2)) * 16; // COMMON FOR ALL
    BossG->Life = 3;
    BossG->Active = 0;
    BossG->Face = -1;
    BossG->Mode = 0;
    BossG->Data0 = 0;
    BossG->Data1 = 0;

    short Height;
    switch (Model) {
    case boss_1: // Boom boom

      BossG->Sprite = boss1_spikes_spr;

      Height = 23; // BossG->Height = BossG->Height2 = 23;
      BossG->Handler = (void (*)(void *))Boss_handler_1;
      BossG->Die = (void (*)(void *))Boss_die_1;
      break;
    case boss_2: // Koopa kid

      BossG->Sprite = boss2_left_spr; //(long)

      Height = 30; // BossG->Height = BossG->Height2 = 30;
      BossG->Handler = (void (*)(void *))Boss_handler_2;
      BossG->Die = (void (*)(void *))Boss_die_1; // 2
      break;
    case bowser: // boss_1:

      BossG->Sprite = bowser_facing_spr;
      BossG->Life = 10;
      Height = 38; // BossG->Height = BossG->Height2 = 38;

      BossG->Handler = (void (*)(void *))Bowser_handler; // Dummy_boss;
      BossG->Die = (void (*)(void *))Bowser_die;

      BossG->Y -= 16;

      break;
    }

    BossG->Height = BossG->Height2 = Height;
    BossG->Y -= BossG->Height;

    Nr++;
  };

  // Leveldata.Boss=0;
  /*
  if(Leveldata.Boss){





          Boss = (void*)Flying_platforms +
Leveldata.Nr_of_flying_platforms*sizeof(flying_platform); Boss->Sprite =
Boss_sprites + (long)Boss->Sprite;
          //Boss->Mask = Boss_masks + (long)Boss->Mask;



//		if((long)Boss->Handler == 0)
//			(boss*)Boss->Handler = Boss_dummy_h;
          if((long)Boss->Handler == 1)
                  (boss*)Boss->Handler = Boss_handler_1;
          if((long)Boss->Handler == 2)
                  (boss*)Boss->Handler = Boss_handler_2;

          if((long)Boss->Die == 1)
                  (boss*)Boss->Die = Boss_die_1;
          if((long)Boss->Die == 2)
                  (boss*)Boss->Die = Boss_die_1;//Boss_die_2;

  };
  */
  HeapUnlock(Temp /*Levelfile_sym->handle*/); // unlock levelfile

  return 0;
}

short Generate_handler_1_enemy(short Nr, char Model, char Face,
                               unsigned char D0D1) {
  // All handler_1 enemies is now generated in this func because it is needed in
  // both when loading level and when respawning an enemy
  short NextRaw = 4;

  Enemies[Nr].Active = 1; // common for all
  Enemies[Nr].Mode = 1;
  Enemies[Nr].Life = 1;

  Enemies[Nr].Handler = Enemy_handler_1; // common for all 1-20
  Enemies[Nr].Face = Face;               // common for all 1-20
  // 	/*Enemies[Nr].Height = */Enemies[Nr].Height2 = 16;//common for most 1-20

  // Enemies[Nr].Attribs = 0b11111001;

  //  NextRaw += 1;

  if (!(Model % 2)) {
    Enemies[Nr].Data0 = Enemies[Nr].Data1 = (D0D1 * 16); // common for all 1-20
    char Active;
    if (Enemies[Nr].Data0 == 0) { // if zero: Mode 3: walk to the end and turn
      /*Enemies[Nr].*/ Active = 3;
    } else {
      /*Enemies[Nr].*/ Active = 2;
    }
    Enemies[Nr].Active = Active;

    NextRaw = 5; // NextRaw += 1;
  }

  unsigned short *Sprite;
  void *Die;
  unsigned char Attribs = 0b11111001;
  short Height2 = 16;

  switch (Model) {
    // start of "handler 1 enemies"
  case goomba_free:
  case goomba_limited:
    /*Enemies[Nr].*/ Sprite = monster1_sprite;
    // Enemies[Nr].Height = (Enemies[Nr].Height2 -= 1);
    /*Enemies[Nr].Height = */ /*Enemies[Nr].*/ Height2 = 15;
    Enemies[Nr].Y += 1;
    Enemies[Nr].Mode = 0;
    /*(enemy *)Enemies[Nr].*/ Die = Enemy_die_1;
    break;
  case turtle_free:
  case turtle_limited:
    /*Enemies[Nr].*/ Sprite = turtle_right_sprite;
    /*(enemy *)Enemies[Nr].*/ Die = Enemy_die_2;
    Height2 = 26;
    Enemies[Nr].Y -= 10;
    break;
  case hedgehog_free:
  case hedgehog_limited:
    /*Enemies[Nr].*/ Sprite = hedgehog_left_spr;
    /*Enemies[Nr].Height = */ /*Enemies[Nr].*/ Height2 = 15;
    Enemies[Nr].Y += 1;
    /*Enemies[Nr].*/ Attribs = 0b00111001;
    /*(enemy *)Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
    break;
  case beetle_free:
  case beetle_limited:
    /*Enemies[Nr].*/ Sprite = beetle_left_sprite;
    /*(enemy *)Enemies[Nr].*/ Die = Enemy_die_2;
    Enemies[Nr].Life = 2;
    /*Enemies[Nr].*/ Attribs = 0b10111000;
    break;
  case fish_free:
  case fish_limited:
    /*Enemies[Nr].*/ Sprite = fish1_left_spr;
    /*Enemies[Nr].*/ Attribs = 0b01011001;
    Enemies[Nr].Y -= 2;
    /*	(enemy *)Enemies[Nr].*/ Die = Dummy_func_; // E_dummy_handler;
    break;

  case skel_tur_free:
  case skel_tur_limited:
    Enemies[Nr].Y -= 27;
    /*Enemies[Nr].Height = */ /*Enemies[Nr].*/ Height2 = 27;
    /*Enemies[Nr].*/ Sprite = turtle_skeleton_left_sprite;
    /*Enemies[Nr].*/ Attribs = 0b10110000;
    /*(enemy *)Enemies[Nr].*/ Die = Enemy_die_7;
    break;
  };

  if ((abs(Model) > 2) && (Enemies[Nr].Face > 0)) {
    Sprite += 3 * Height2;
  }

  Enemies[Nr].Attribs = Attribs;
  //	if(Enemies[Nr].Die==NULL)
  Enemies[Nr].Die = Die;
  //	if(Sprite==NULL)
  Enemies[Nr].Sprite = Sprite;
  Enemies[Nr].Height = Enemies[Nr].Height2 =
      Height2; // Height is neccessarry for goomba resurrection!
  return NextRaw;
}

void SetBg(short Bg_nr /*,HANDLE Temp*/) { // this func sets the pointer to th
                                           // bg tilemap, updates the bg plane
                                           // struct and leveldata
  // HANDLE Temp;
  /*	bg_data Bg_data;
    bg_filedata Bg_fileinfo;
  */
  // if( !(Temp = Bg_file_sym->handle) )
  //  return -4;

  // Temp = Bg_file_sym->handle;

  //	 HANDLE Temp = Bg_file_sym->handle;

  bg_data Bg_data;
  bg_filedata *Bg_fileinfo;

  //	HANDLE Temp = Bg_file_sym_h;

  char *Raw = HeapDeref(Bg_file_sym_h) + 2;

  //	memcpy(&Bg_fileinfo,HeapDeref (Temp)+2,sizeof(bg_filedata));
  Bg_fileinfo = (bg_filedata *)(Raw); // HeapDeref (Temp)+2);
  Raw += Bg_fileinfo->Backgrounds[Bg_nr];
  memcpy(&Bg_data,
         /*HeapDeref (Temp)+2*/ Raw /*+Bg_fileinfo->Backgrounds[Bg_nr]*/,
         sizeof(bg_data));

  Bg_plane.matrix =
      /*HeapDeref (Temp)+2*/ Raw /*+Bg_fileinfo->Backgrounds[Bg_nr]*/ +
      sizeof(bg_data);
  Bg_plane.width = Bg_data.Width;
  Bg_plane.force_update = 1;
  Leveldata.Bg_height = Bg_data.Height;
  Leveldata.Background = Bg_nr;

  /*
  //working, but unoptimized
  bg_data Bg_data;
bg_filedata Bg_fileinfo;

  HANDLE Temp = Bg_file_sym_h;

  memcpy(&Bg_fileinfo,HeapDeref (Temp)+2,sizeof(bg_filedata));
  memcpy(&Bg_data,HeapDeref
(Temp)+2+Bg_fileinfo.Backgrounds[Bg_nr],sizeof(bg_data));

  Bg_plane.matrix = HeapDeref
(Temp)+2+Bg_fileinfo.Backgrounds[Bg_nr]+sizeof(bg_data); Bg_plane.width =
Bg_data.Width; Bg_plane.force_update = 1; Leveldata.Bg_height = Bg_data.Height;
  Leveldata.Background = Bg_nr;
  */
};

// The map payload - the tile matrix, then the triggers, then the objects - is
// the one block mkdata.py cannot reach: where it starts depends on
// Height*Width and Nr_of_triggers, which are fields of the very header it is
// swapping. So it swaps here instead, once Load_map() has the copy in place,
// the same bargain Swap_leveldata() strikes.
//
// The tile matrix is bytes and needs nothing. Every field of a map_trigger is
// a short, so that array swaps uniformly. A map_object mixes widths: X and Y
// are shorts, Mode through Treasure are bytes, and Handler, Sprite and Mask
// are 32-bit - holding a handler tag and a sprite index at this point, which
// the loop in Load_map() turns into real pointers once this has run.
static void Swap_map_payload() {

  long I;
  short C;

  { // a map_trigger is fourteen shorts and nothing else
    short *Field = (short *)Map_triggers;
    long Fields =
        (long)Map_data.Nr_of_triggers * (sizeof(map_trigger) / sizeof(short));

    for (I = 0; I < Fields; I++)
      Field[I] = be16(Field[I]);
  }

  // Only the objects that came out of the file: Load_map() adds three slots of
  // its own - doom ship, money ship and card game - which it left zeroed.
  for (C = 0; C < Map_data.Nr_of_objects - 3; C++) {

    Map_objects[C].X = be16(Map_objects[C].X);
    Map_objects[C].Y = be16(Map_objects[C].Y);

    Map_objects[C].Handler =
        (void (*)(void *))be32((unsigned long)Map_objects[C].Handler);
    Map_objects[C].Sprite =
        (unsigned short *)be32((unsigned long)Map_objects[C].Sprite);
    Map_objects[C].Mask =
        (unsigned short *)be32((unsigned long)Map_objects[C].Mask);
  }
}

short Load_map(char *Levelfile) {
  SYM_ENTRY *Levelfile_sym;
  HANDLE Temp;
  levelfiledata *Levelfileinfo;
  short C;

  // Error: Map not found
  /*
  if( !(Levelfile_sym = SymFindPtr (SYMSTR(Levelfile), 4)) ){//flags = 4 will
change when the documentation of VAT.h is updated ErrorCode = 4; Exit = 3;
  return 4;//error "failed to open map"
};

if( !(Temp = Levelfile_sym->handle) ){
          ErrorCode = 4;
  Exit = 3;
    return 4;//error "failed to open map"
  }*/

  if (!(Temp = File_get_pointer_and_lock(Levelfile /*,Levelfile_sym*/))) {
    ErrorCode = 4;
    Exit = 3;
    return 4; // error "failed to open map"
  }

  //	memcpy( &Levelfileinfo, HeapDeref (Temp)+2, sizeof(levelfiledata) );

  void *RawData = HeapDeref(Temp) + 2;
  Levelfileinfo = (levelfiledata *)(RawData /*HeapDeref (Temp)+2*/);

  if (Levelfileinfo->Mode == 1) {

    memcpy(&Map_data, RawData /*HeapDeref (Temp)+2*/ + sizeof(levelfiledata),
           sizeof(map_data));

    /*if(Map)
                  free(Map);*/
    Free(Map);

    Map_data.Nr_of_objects +=
        3; // 1;//1 slot for doom ship, 1 for money ship and 1 for cardgame

    // Error: Memory

    if (!(Map = malloc(Map_data.Height * Map_data.Width +
                       sizeof(map_trigger) * Map_data.Nr_of_triggers +
                       sizeof(map_object) * (Map_data.Nr_of_objects)))) {
      ErrorCode = 1;
      Exit = 3;
      return 1;
    }

    // memset(Map+Map_data.Height*Map_data.Width+Map_data.Nr_of_triggers*sizeof(map_trigger),0,sizeof(map_object)*Map_data.Nr_of_objects);
    // memset(Map,0,Map_data.Height*Map_data.Width+sizeof(map_trigger)*Map_data.Nr_of_triggers+sizeof(map_object)*(Map_data.Nr_of_objects)
    // );

    for (C = 0; C < Map_data.Height * Map_data.Width +
                        Map_data.Nr_of_triggers * sizeof(map_trigger) +
                        sizeof(map_object) * Map_data.Nr_of_objects;
         C++)
      *(((char *)Map) + C) = 0; // optimizaton

    memcpy(Map,
           RawData /*HeapDeref (Temp)+2*/ + sizeof(levelfiledata) +
               sizeof(map_data),
           Map_data.Height * Map_data.Width +
               sizeof(map_trigger) * Map_data.Nr_of_triggers +
               sizeof(map_object) * (Map_data.Nr_of_objects - 3));

    Map_plane.p.matrix = Map;
    Map_plane.p.width = Map_data.Width;
    Map_plane.p.force_update = 1;

    SavePlayer.PrevCompX = SavePlayer.MapX = Map_data.PlayerX;
    SavePlayer.PrevCompY = SavePlayer.MapY = Map_data.PlayerY;
    FgX = Map_data.FgX;
    FgY = Map_data.FgY;

  } else {
    return 0;
  };

  //(void*)
  Map_triggers = ((void *)Map + Map_data.Height * Map_data.Width);

  Map_objects =
      ((void *)Map_triggers + Map_data.Nr_of_triggers * sizeof(map_trigger));
  // Map_objects = (void*)Map + Map_data.Height*Map_data.Width;

  Swap_map_payload();

  for (C = 0; C < Map_data.Nr_of_objects; C++) {

    if (Map_objects[C].Mode) {
      Map_objects[C].Sprite = Sprites + (long)Map_objects[C].Sprite;
      // Map_objects[C].Mask = Masks + (long)Map_objects[C].Mask;

      if ((long)Map_objects[C].Handler == 1)
        Map_objects[C].Handler = (void (*)(void *))Handle_ship;
      if ((long)Map_objects[C].Handler == 2)
        Map_objects[C].Handler = (void (*)(void *))Handle_map_monster;
      if ((long)Map_objects[C].Handler == 3)
        Map_objects[C].Handler = (void (*)(void *))Handle_boat;
    };
  };

  HeapUnlock(Temp); // unlock levelfile

  return 0;
};

void Free(void *Mem) {
  if (Mem)
    free(Mem);
};
