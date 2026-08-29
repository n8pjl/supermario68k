
#include "map.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "compat/gray.h"
#include "control.h"
#include "gameloop.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "levelset.h"
#include "player.h"
#include "render.h"
#include "scankeys.h"
#include "shells.h"
#include "smallgames.h"
#include <stdint.h>

inline void Handle_player_map() {
  int16_t OldFgX, OldFgY, Temp = 0, Temp2;
  static int16_t CX = 0, CY = 0, /*L=0,*/ C = 0;
  static const int16_t Searchtable[4] = {0, 0, -16, 16};
  // static short PrevCompX,PrevCompY;

  if (Player.Offset > 0) { //
    CX = SavePlayer.PrevCompX - SavePlayer.MapX;
    CY = SavePlayer.PrevCompY - SavePlayer.MapY;
    SavePlayer.L = 0;
  }

  Player.Offset = 0;

  // Player.Test = L;

  if (Keystate.run && !Previous_keystate.run) { // toggle map statusbar state

    if (Map_statusbar == status) {
      Map_statusbar = itemlist;
      Player.Curr_item = 0;
    } else {
      Map_statusbar = status;
    };
  };

  if (Map_statusbar == status) {

    if ((CX == 0) && (CY == 0)) {

      if (Keystate.right /*&& ((Player.MapX)<(Map_data.Border_right))*/ &&
          ((SavePlayer.L == 0) || (SavePlayer.L == 2))) {
        CX = 0;
        Player.Face = 1;
        if (!SavePlayer.IsOnMapBoat) { // not in boat
          if ((CX = Free_right_map(SavePlayer.MapX, SavePlayer.MapY)) ==
              0) { // set player motion

            for (C = 0; C < Map_data.Nr_of_objects;
                 C++) { // check if boat in range
              if ((Map_objects[C].Mode == 30) &&
                  (Map_objects[C].X == (SavePlayer.MapX + 16)) &&
                  (Map_objects[C].Y == (SavePlayer.MapY))) {
                CX = 16;                    // move to boat
                SavePlayer.IsOnMapBoat = C; //"subscribe" to this boat
              };
            };
          } else { // CX!=0
            if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY) >= levels_low) {
              SavePlayer.IsClouded = 0;
            }
            if (Get_map_tile(SavePlayer.MapX + CX, SavePlayer.MapY) >=
                levels_low) {

              if (!SavePlayer.IsClouded) {
                SavePlayer.L = 1;
              } else {
                // Player.IsClouded = 0;
              }
            } else {
              SavePlayer.L = 0;
            };
          };

        } else { // player in boat
          if ((Get_map_tile(SavePlayer.MapX + 16, SavePlayer.MapY) ==
               water_map) &&
              (Map_data.Border_right >
               (SavePlayer.MapX + 16))) { // check if water
            CX = 16;
            Map_objects[(int16_t)SavePlayer.IsOnMapBoat].MoveX =
                16; // move boat along with player
          };
          if (Get_map_tile(SavePlayer.MapX + 16, SavePlayer.MapY) ==
              dock) {                   // check if dock in range
            CX = 16;                    // move to dock
            SavePlayer.IsOnMapBoat = 0; //"unsubscribe" the current boat
            SavePlayer.PrevCompX = SavePlayer.MapX + 16;
            SavePlayer.PrevCompY = SavePlayer.MapY;
          };
        };
      } else {

        if (Keystate.left /*&& (Player.MapX>Map_data.Border_left)*/ &&
            ((SavePlayer.L == 0) || (SavePlayer.L == 1))) {
          CX = 0;
          Player.Face = -1;
          // Replace_by_road(4*16,5*16);//test case

          if (!SavePlayer.IsOnMapBoat) { // not in boat
            if ((CX = Free_left_map(SavePlayer.MapX, SavePlayer.MapY)) ==
                0) { // set player motion
              for (C = 0; C < Map_data.Nr_of_objects;
                   C++) { // check if boat in range
                if ((Map_objects[C].Mode == 30) &&
                    (Map_objects[C].X == (SavePlayer.MapX - 16)) &&
                    (Map_objects[C].Y == (SavePlayer.MapY))) {
                  CX = -16;                   // move to boat
                  SavePlayer.IsOnMapBoat = C; //"subscribe"  to this boat
                };
              };
            } else { // CX!=0
              if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY) >=
                  levels_low) {
                SavePlayer.IsClouded = 0;
              }
              if (Get_map_tile(SavePlayer.MapX + CX, SavePlayer.MapY) >=
                  levels_low) {

                if (!SavePlayer.IsClouded) {
                  SavePlayer.L = 2;
                } else {
                  // Player.IsClouded = 0;
                }
              } else {
                SavePlayer.L = 0;
              };
            };
          } else { // player in boat
            if ((Get_map_tile(SavePlayer.MapX - 16, SavePlayer.MapY) ==
                 water_map) &&
                (Map_data.Border_left < SavePlayer.MapX)) { // check if water
              CX = -16;
              Map_objects[(int16_t)SavePlayer.IsOnMapBoat].MoveX =
                  -16; // move boat along with player
            };
            if (Get_map_tile(SavePlayer.MapX - 16, SavePlayer.MapY) ==
                dock) { // check if dock in range
              CX = -16;
              SavePlayer.IsOnMapBoat = 0; //"unsubscribe" the current boat
              SavePlayer.PrevCompX = SavePlayer.MapX - 16;
              SavePlayer.PrevCompY = SavePlayer.MapY;
            };
          };
        } else {

          if (Keystate.up && (SavePlayer.MapY > 0) &&
              ((SavePlayer.L == 0) || (SavePlayer.L == 4))) {
            CY = 0;
            /*
            New_card_game();
            Play_card_game();//test case
            */
            Player.Face = 2;

            if (!SavePlayer.IsOnMapBoat) { // not in boat
              if ((CY = Free_up_map(SavePlayer.MapX, SavePlayer.MapY)) ==
                  0) { // set player motion
                for (C = 0; C < Map_data.Nr_of_objects;
                     C++) { // check if boat in range
                  if ((Map_objects[C].Mode == 30) &&
                      (Map_objects[C].X == (SavePlayer.MapX)) &&
                      (Map_objects[C].Y == (SavePlayer.MapY - 16))) {
                    CY = -16;                   // move to boat
                    SavePlayer.IsOnMapBoat = C; //"subscribe"  to this boat
                  };
                };
              } else { // CX!=0
                if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY) >=
                    levels_low) {
                  SavePlayer.IsClouded = 0;
                }
                if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY + CY) >=
                    levels_low) {

                  if (!SavePlayer.IsClouded) {
                    SavePlayer.L = 3;
                  } else {
                    // Player.IsClouded = 0;
                  }
                } else {
                  SavePlayer.L = 0;
                };
              };
            } else { // player in boat
              if ((Get_map_tile(SavePlayer.MapX, SavePlayer.MapY - 16) ==
                   water_map)) { // check if water
                CY = -16;
                Map_objects[(int16_t)SavePlayer.IsOnMapBoat].MoveY =
                    -16; // move boat along with player
              };
              if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY - 16) ==
                  dock) { // check if dock in range
                CY = -16;
                SavePlayer.IsOnMapBoat = 0; //"unsubscribe" the current boat
                SavePlayer.PrevCompX = SavePlayer.MapX;
                SavePlayer.PrevCompY = SavePlayer.MapY - 16;
              };
            };
          } else {

            if (Keystate.down &&
                (SavePlayer.MapY < (16 * Map_data.Height - 1)) &&
                ((SavePlayer.L == 0) || (SavePlayer.L == 3))) {
              CY = 0;
              Player.Face = -2;
              if (!SavePlayer.IsOnMapBoat) { // not in boat
                if ((CY = Free_down_map(SavePlayer.MapX, SavePlayer.MapY)) ==
                    0) { // set player motion
                  for (C = 0; C < Map_data.Nr_of_objects;
                       C++) { // check if boat in range
                    if ((Map_objects[C].Mode == 30) &&
                        (Map_objects[C].X == (SavePlayer.MapX)) &&
                        (Map_objects[C].Y == (SavePlayer.MapY + 16))) {
                      CY = 16;                    // move to boat
                      SavePlayer.IsOnMapBoat = C; //"subscribe"  to this boat
                    };
                  };
                } else { // CX!=0
                  if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY) >=
                      levels_low) {
                    SavePlayer.IsClouded = 0;
                  }
                  if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY + CY) >=
                      levels_low) {

                    if (!SavePlayer.IsClouded) {
                      SavePlayer.L = 4;
                    } else {
                      // Player.IsClouded = 0;
                    }
                  } else {
                    SavePlayer.L = 0;
                  };
                };
              } else { // player in boat
                if ((Get_map_tile(SavePlayer.MapX, SavePlayer.MapY + 16) ==
                     water_map)) { // check if water
                  CY = 16;
                  Map_objects[(int16_t)SavePlayer.IsOnMapBoat].MoveY =
                      16; // move boat along with player
                };
                if (Get_map_tile(SavePlayer.MapX, SavePlayer.MapY + 16) ==
                    dock) { // check if dock in range
                  CY = 16;
                  SavePlayer.IsOnMapBoat = 0; //"unsubscribe" the current boat
                  SavePlayer.PrevCompX = SavePlayer.MapX;
                  SavePlayer.PrevCompY = SavePlayer.MapY + 16;
                };
              };
            };
          };
        };
      };
    } // end of (CX==0) && (CY==0)
    else {
      Player.Offset = -1;
    };

    // Player.Test = L;//test

    if (CX) {
      SavePlayer.MapX += (CX > 0 ? map_speed : -map_speed);
      CX -= (CX > 0 ? map_speed : -map_speed);
    };

    if (CY) {
      SavePlayer.MapY += (CY > 0 ? map_speed : -map_speed);
      CY -= (CY > 0 ? map_speed : -map_speed);
    };

    FgX = SavePlayer.MapX - screen_width / 2;
    if (FgX < Map_data.Border_left) {
      FgX = Map_data.Border_left;
    } else {
      if (FgX > (Map_data.Border_right - screen_width)) {
        FgX = (Map_data.Border_right) - screen_width;
      };
    };

    FgY = SavePlayer.MapY - (screen_height - 0) / 2;
    if (FgY < 0) {
      FgY = 0;
    } else {
      if (FgY > ((Map_data.Height + 1) * 16 - screen_height)) {
        FgY = (Map_data.Height + 1) * 16 - screen_height;
      };
    };

    if (Keystate.jump && !Previous_keystate.jump) {

      Temp2 = Get_map_tile(SavePlayer.MapX, SavePlayer.MapY);

      if ((Temp2 >= levels_low) && (Temp2 <= levels_high)) {

        // Enter_level_anim();
        char Coins = SavePlayer.Coins;
        Load_level(Levelfilename,
                   Get_map_tile(SavePlayer.MapX, SavePlayer.MapY) - levels_low);

        OldFgX = FgX;
        OldFgY = FgY;

        // Adjust_renderpoint();//Render();
        Playloop();

        Map_plane.p.force_update = 1;

        Move_map_objects = 1;

        // Replace_map_tile(locked_door);

        if (Exit == 2) {

          /*if(Temp2==bowser_castle){
                  Exit = 255;
          }*/

          if ((Temp2 == small_castle) || (Temp2 == small_castle_2)) {
            // Temp = demolished_castle
            Temp =
                (Map_data.Color == 2 ? demolished_castle_dark
                                     : demolished_castle); // demolished_castle;
            Replace_map_tile(
                (Temp2 == small_castle ? locked_door : locked_door_2));
          } else {
            Temp = M_tile;
          };

          Put_map_tile(SavePlayer.MapX, SavePlayer.MapY, Temp);

          SavePlayer.PrevCompX = SavePlayer.MapX;
          SavePlayer.PrevCompY = SavePlayer.MapY;

          // check: Event? (Appearance of Card Game, money ship, hidden mushrom
          // house)

          if (SavePlayer.Coins > Coins) {
            Coins = SavePlayer.Coins - Coins;
          } else {
            Coins = SavePlayer.Coins + (100 - Coins);
          }

          if ((Leveldata.Condition == Coins) || (Leveldata.Condition >= 240)) {
            /*
            switch(Leveldata.Event){
                    case 1://card game
                            Add_card_game();
                            break;
                    case 5://money ship
                            Add_money_ship();
                            break;
            }*/
            Add_map_event(Leveldata.Event);
            /*
            if(Leveldata.Event<=50){
                    Add_map_event(Leveldata.Event);
            }
            else{//hidden mushrom house
                    Exit = 0;
                    Enter_mushrom_house(Leveldata.Event);
            }
            */
          }

        } else {
#ifndef debug
          if (Exit == 99 || Exit == 9) // ( gives debug mode!)
            return;
#endif
#ifdef debug
          if (Exit == 99 /*|| Exit==9*/) // ( gives debug mode!)
            return;
#endif
          Player.Offset = 1;

          //					CX = SavePlayer.PrevCompX -
          // SavePlayer.MapX; 					CY =
          // SavePlayer.PrevCompY - SavePlayer.MapY;
        };
        Exit = 0;
        SavePlayer.L = 0;
        FgX = OldFgX;
        FgY = OldFgY;

      } else { // not level
        switch (Temp2) {

        case big_castle:

          // Enter_level_anim();
          for (C = 0; C < Map_data.Nr_of_objects;
               C++) { // big castle=>releases ship
            if (!Map_objects[C].Mode) {
              Map_objects[C].X = SavePlayer.MapX;
              Map_objects[C].Y = SavePlayer.MapY;
              Map_objects[C].Mode = 1;
              Map_objects[C].Data0 = 0;
              Map_objects[C].Data1 = 1;
              Map_objects[C].Sprite = ship_spr;
              Map_objects[C].Mask = ship_msk;
              Map_objects[C].Handler = (void (*)(void *))Handle_ship;
              // C=Map_data.Nr_of_objects;
              Temp = big_castle_visited;
              break;
            };
          };
          Enter_enemy_ship(&Map_objects[C]);
          if (Leveldata.Condition == 0) { // no ship (i.e. castle)
            Map_objects[C].Mode = 0;
            Temp = big_castle;
          }
          break;

        case mushrom_house: // mushrom house

          // Enter_level_anim();
          Enter_mushrom_house(0);
          Temp = M_tile;
          break;

        case game_house: // game house

          // Enter_level_anim();
          Enter_game_house();
          Temp = M_tile;
          break;

        case pipe:

          // Enter_level_anim();

          for (C = 0; C < Map_data.Nr_of_triggers; C++) {

            if ((Map_triggers[C].X == SavePlayer.MapX) &&
                (Map_triggers[C].Y == SavePlayer.MapY)) {

              // Load_level("common",Map_triggers[C].LevelNr);
              if (Map_triggers[C].NewMap >=
                  0) { // the pipe wraps to another world (warp zone)
                // jump to another world
                // animate
                Levelsetdata.CurrentWorld = Map_triggers[C].NewMap;
                Exit = 3;
              } else { // the pipe wraps to another location in the current
                       // world

                if (Map_triggers[C].LevelNr < 20) {
                  Load_level(Commonfilename, Map_triggers[C].LevelNr);
                } else {
                  Load_level(Levelfilename, Map_triggers[C].LevelNr - 20);
                };

                Player.X = Map_triggers[C].LX;
                Player.Y = Map_triggers[C].LY - Player.Height;

                if (Map_triggers[C].Exit) {
                  Triggers[Map_triggers[C].Exit - 1].Anim = 7;
                };
                /*
                OldFgX=FgX;
                OldFgY=FgY;

                Render();
                Playloop();

                FgX=OldFgX;
                FgY=OldFgY;
                */
                Play_level();

                Map_plane.p.force_update = 1;

                if (Exit == 2) {
                  SavePlayer.MapX = Map_triggers[C].NewX;
                  SavePlayer.MapY = Map_triggers[C].NewY;
                  Map_data.Border_left = Map_triggers[C].NewBorder_left;
                  Map_data.Border_right = Map_triggers[C].NewBorder_right;
                  Map_data.Color = Map_triggers[C].Color;
                  SavePlayer.PrevCompX = SavePlayer.MapX;
                  SavePlayer.PrevCompY = SavePlayer.MapY;
                };

                Exit = 0;

                break; // continue;
              };
            };
          };

          Temp = pipe;
          break;

        default:

          Temp = Temp2;

        }; // end switch

        Put_map_tile(SavePlayer.MapX, SavePlayer.MapY, Temp);
      };
    };

  } else { // Map_statusbar==itemlist
    if (Keystate.left && !Previous_keystate.left && Player.Curr_item > 0) {
      Player.Curr_item--;
    };
    if (Keystate.right && !Previous_keystate.right &&
        SavePlayer.Itemlist[Player.Curr_item + 1] &&
        Player.Curr_item < (itemlist_length - 1)) {
      Player.Curr_item++;
    };
    if (Keystate.jump && !Previous_keystate.jump) {
      if (SavePlayer.Itemlist[(int16_t)Player.Curr_item]) {
        // collect item
        int16_t Remove = 1;
        switch (SavePlayer.Itemlist[(int16_t)Player.Curr_item]) {
        case 1: { // mushrom

          if (SavePlayer.Life ==
              1) { // only effective if small mario (if racoon mario or fire
                   // mario: don't do anything)
            SavePlayer.Life = 2;
            Player.Height = Player.Height2 = 27;
            SavePlayer.Maskbase = SavePlayer.Spritebase =
                (Player.Face == 1) ? 3 : 2;
          };
          break;
        };
        case 2: { // fire flower

          SavePlayer.Attribs =
              SavePlayer.Attribs | 0b01000000; // add fireball ability
          SavePlayer.Attribs =
              SavePlayer.Attribs & 0b11010111; // remove racoon suit and p-wing
          SavePlayer.Life = 3;
          SavePlayer.Spritebase = (Player.Face == 1) ? 5 : 4;
          SavePlayer.Maskbase = (Player.Face == 1) ? 3 : 2;
          Player.Height = Player.Height2 = 27;

          break;
        };
        case 7: {                                               // P-wing
          SavePlayer.Attribs = SavePlayer.Attribs | 0b00001000; // add P-wing
          // No break here!
        };
        case 3: { //(leaf) racoon suit

          SavePlayer.Attribs =
              SavePlayer.Attribs | 0b00100000; // add racoon suit
          SavePlayer.Attribs =
              SavePlayer.Attribs & 0b10111111; // remove fireball ability
          SavePlayer.Life = 3;
          SavePlayer.Spritebase = (Player.Face == 1) ? 3 : 2;
          SavePlayer.Maskbase = (Player.Face == 1) ? 3 : 2;
          Player.Height = Player.Height2 = 27;

          break;
        };
        case 4: { // star

          SavePlayer.Attribs = SavePlayer.Attribs | 0b10000000;
          Player.Immortal = star_immortal_time;

          break;
        };
        case 5: { // whistle (warp zone)
          Remove = 0;
          // animation
          for (C = 0; C < 12; C++) {

            uint16_t D;
            if (C % 2) {

              //								memset(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,0x0000,LCD_SIZE);
              //								memset(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,0xffff,LCD_SIZE);

              // FastFilledRect_Draw_R(dBufHPD_G,0,0,239,127);
              FilledRectDark(0, 0, 239, 127);

              //								FastFilledRect_Draw_R(dBufHPL_G,0,0,239,127);

              GrayDBufToggleSync_SetPointers();
            } else {
              Render_map();
            }
            for (D = 0; D < 65000; D++)
              ;
          };
          // end

          //						warp_cloud_spr
          Map_objects[0].X = FgX + screen_width;
          Map_objects[0].Y = SavePlayer.MapY;
          Map_objects[0].Sprite = warp_cloud_spr;
          Map_objects[0].Mode = 1;
          Map_objects[0].Handler = Dummy_func_;

          /*short AX=scren_width;
          short AY = SavePlayer.MapY;*/

          do {

            Map_objects[0].X -= 2;
            if (Map_objects[0].X <= SavePlayer.MapX) {
              SavePlayer.MapX = -16;
            }
            Render_map();
            int16_t C;
            for (C = 0; C < 100; C++)
              ;

          } while (Map_objects[0].X > FgX);

          Map_statusbar = status;
          int16_t WarpX = Map_data.WarpX;
          int16_t WarpY = Map_data.WarpY;
          Levelsetdata.CurrentWorld = Levelsetdata.Commonfile;
          Load_map(Commonfilename);
          // set MapY, depending on current world
          SavePlayer.MapX = WarpX;
          SavePlayer.MapY = WarpY;
          SavePlayer.L = 0;

          Itemlist_remove(Player.Curr_item);
          Gameloop();

          //						Levelsetdata.CurrentWorld
          //= Levelsetdata.Nr_of_files-2;
          // Exit = 100;

          // Exit=0;

          break;
        };
        case 6: { // hammer
          for (C = 0; C <= 3; C++) {

            if (Get_map_tile(SavePlayer.MapX + Searchtable[C],
                             SavePlayer.MapY + Searchtable[3 - C]) ==
                rock) { // rock?

              int16_t X = SavePlayer.MapX + 2 * Searchtable[C];
              int16_t Y = SavePlayer.MapY + 2 * Searchtable[3 - C];
              if ((Get_map_tile(X, Y) <= walkable_high) && (X > 0) &&
                  (X < (Map_data.Width * 16)) && (Y > 0) &&
                  (Y < (Map_data.Height * 16))) { // walkable behind the rock?

                Temp = (C < 2 ? 1 : 0); // decide which direction (x/y)=>which
                                        // road to apply

              } else {
                Temp = ground; // grass
              };
              // set new tile

              Put_map_tile(SavePlayer.MapX + Searchtable[C],
                           SavePlayer.MapY + Searchtable[3 - C], Temp);
            };
          };
          break;
        };
        case 8: { // cloud
          SavePlayer.L = 0;
          SavePlayer.IsClouded = 1;
          break;
        };
        case 9: { // anchor
          for (C = 0; C < Map_data.Nr_of_objects;
               C++) { // big castle=>releases ship
            if (Map_objects[C].Mode == 1) {
              Map_objects[C].Mode = -1; // anchor this ship
            };
          };
          break;
        };

          /*case 5:{

                  break;
          };*/
        };

        SavePlayer.Score += 10;
        if (Remove)
          Itemlist_remove(Player.Curr_item);
      };
    };
  };

#ifdef debug
  SavePlayer.L = 0; // Debug mode: can allways pass any uncompleted level
#endif
};

void Enter_mushrom_house(uint8_t Treasure) {
  int16_t OldFgX, OldFgY, C;

  Load_level(Commonfilename, 0);

  if (Treasure) {
    Put_tile(5 * 16, 7 * 16, 0);
    Put_tile(9 * 16, 7 * 16, 0);
    Put_tile(7 * 16, 7 * 16, Treasure);
  }
  /*
  OldFgX=FgX;
  OldFgY=FgY;

//	Render();
  Playloop();

  FgX=OldFgX;
  FgY=OldFgY;
  */
  Play_level();

  Map_plane.p.force_update = 1;

  Exit = 0;
};

void Enter_enemy_ship(struct map_object *Ship) {
  int16_t OldFgX, OldFgY, C;

  Load_level(Levelfilename, 7);
  /*
  OldFgX=FgX;
  OldFgY=FgY;

  Render();
  Playloop();


  FgX=OldFgX;
  FgY=OldFgY;
  */
  Play_level();

  // Bug: FgX==0 ! WTF???
  /*
  if( (FgX==0) || (FgY==0) ){
          while(1);
  }
  */
  SavePlayer.Attribs =
      SavePlayer.Attribs & 0b01110111; // disable star and p-wing
  Map_plane.p.force_update = 1;

  if (Exit == 1) { // mario failed,
    // run away

    if (Leveldata.Condition && (Ship->Mode == 1)) { // ship and not anchored

      C = ((Ship->X >> 4) + Map_data.Width * (Ship->Y >> 4) +
           1); // find array index in map (ship position)

      while (((*(Map + C)) != M_tile) && ((*(Map + C)) != demolished_castle)) {
        C++;
        if (C == (Map_data.Height * Map_data.Width))
          C = 0;
      };

      OldFgX = ((C % Map_data.Width) * 16); // Calculate new position
      OldFgY = ((C / Map_data.Width) * 16);

      while ((OldFgX != Ship->X) || (OldFgY != Ship->Y)) {

        if (OldFgX != Ship->X)
          Ship->X += (OldFgX > Ship->X ? 2 : -2); // 1
        if (OldFgY != Ship->Y)
          Ship->Y += (OldFgY > Ship->Y ? 2 : -2); // 1
        if (!(BoundsCollide2(
                Ship->X, Ship->Y, 16, 16, FgX, FgY, screen_height,
                screen_width))) { // if ship outside visible part of screen
          Ship->X = OldFgX;
          Ship->Y = OldFgY;
        };

        Render_map();

        for (C = 0; C < 15000; C++)
          ; // time delay
      };
    };

    // animate
    Exit = 0;
  };
  if (Exit == 2) { // world completed!!!

    // FastFilledRect_Draw_R(dBufHPD_G,0,0,239,127);//Dark gray screen
    FilledRectDark(0, 0, 239, 127);
    // FastFilledRect_Erase_R(dBufHPL_G,0,0,239,127);
    FilledRectEraseLight(0, 0, 239, 127);

    // FastFilledRect_Erase_R(dBufHPD_G,20,20,140,80);//white rectangle
    FilledRectEraseDark(20, 20, 140, 80);

    //		FastFilledRect_Erase_R(dBufHPL_G,0,25,43,75);

    // some text here...
    // DrawGrayStrExt2B(28,23,Texts+GameTextData.ThankU1,A_REPLACE,F_4x6,dBufHPD_G,dBufHPL_G);//"THANK
    // YOU MARIO, BUT OUR"
    // DrawGrayStrExt2B(28,33,Texts+GameTextData.ThankU2,A_REPLACE,F_4x6,dBufHPD_G,dBufHPL_G);//"PRINCESS
    // IS IN ANOTHER CASTLE!"
    // DrawGrayStrExt2B(28,43,Texts+GameTextData.ThankU3,A_REPLACE,F_4x6,dBufHPD_G,dBufHPL_G);//spare,
    // in case of needed
    DrawString(28, 23, Texts + GameTextData.ThankU1, A_REPLACE, F_4x6);
    DrawString(28, 33, Texts + GameTextData.ThankU2, A_REPLACE, F_4x6);
    DrawString(28, 43, Texts + GameTextData.ThankU3, A_REPLACE, F_4x6);

    //		DrawGrayStrExt2B(28,43,Texts+GameTextData.ThankU2,A_REPLACE,F_4x6,dBufHPD_G,dBufHPL_G);
    //		DrawGrayStrExt2B(28,48
    // Move text into txt file
    // Use three lines in case some translator comus up with a very long
    // sentence....

    // DrawItem(blah blah blah);

    uint16_t *Sprite;
    int16_t Height = 16;
    //		Leveldata.Event=8;
    /*	switch(Leveldata.Event){

                            case 1:{
                                    Sprite = pu_mushrom_sprite;
                                    //Mask = mushrom_mask;
                                    //Height = 16;
                                    break;
                            };
                            case 2:{
                                    Sprite = flower_spr;
                                    //Mask = flower_msk;
                                    //Height = 16;
                                    break;
                            };
                            case 3:{
                                    Sprite = leaf_r_sprite;
                                    //Mask = leaf_r_mask;
                                    Height = 14;
                                    break;
                            };
                            case 4:{
                                    Sprite = star_anim;
                                    //Mask = star_mask;
                                    break;
                            };
                            case 5:{
                                    Sprite = whistle_spr;
                                    //Mask = whistle_msk;
                                    break;
                            };
                            case 6:{
                                    Sprite = hammer_i_spr;
                                    //Mask = hammer_i_msk;
                                    break;
                            };

                            case 7:{
                                    Sprite = p_wing_spr;
                                    //Mask = p_wing_msk;
                                    break;
                            };
                            case 8:{
                                    Sprite = cloud_item_spr;
                                    //Mask = cloud_item_msk;
                                    break;
                            };
                            case 9:{
                                    Sprite = anchor_spr;
                                    //Mask = anchor_msk;
                                    break;
                            };
                    };
            */
    //		GrayClipSprite16_SMASK_R(40,58,Height,Sprite,Sprite+Height,Sprite+2*Height,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);

    DrawItem(40, 58, Leveldata.Event);

    // GrayClipSprite16_SMASK_R(104,51,30,princess_spr,princess_spr+30,princess_spr+2*30,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
    DrawSprite16SMASKR(104, 51, 30, princess_spr, princess_spr + 30,
                       princess_spr + 2 * 30);

    // Letter from princess ???

    GrayDBufToggleSync_SetPointers();

    // get an item
    Itemlist_add(Leveldata.Event);

    /*while( (_rowread(0)) );
    while( !(_rowread(0)) );*/
    WaitKeyPress();
  };

  // Exit=0;
};

inline void Handle_map_objects() {
  int16_t C; //,OldFgX,OldFgY;

  for (C = 0; C < Map_data.Nr_of_objects; C++) {

    if (Map_objects[C].Mode) {

      Map_objects[C].Handler(&Map_objects[C]);

      if ((BoundsCollide16(SavePlayer.MapX, SavePlayer.MapY, Map_objects[C].X,
                           Map_objects[C].Y)) &&
          (Map_objects[C].Mode <= 20))
        Player.Passified = 1;

      if ((SavePlayer.MapX == (Map_objects[C].X)) &&
          (SavePlayer.MapY == Map_objects[C].Y)) {

        if (Map_objects[C].Mode <= 20)
          // Enter_level_anim();

          switch (Map_objects[C].Mode) {
          case (-1):
          case 1: { // ship
            Enter_enemy_ship(&Map_objects[C]);

            break;
          };
          case 2: // monster
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
            Fight_monster(&Map_objects[C]);
            if (Exit == 2) {
              Map_objects[C].Mode = 0;
            } else {
              Player.Offset = 1;
            };
            if (Exit != 9)
              Exit = 0;
            break;
          case 10: // Hidden mushrom house Exit = 0;
            Enter_mushrom_house(Map_objects[C].Treasure);
            Map_objects[C].Mode = 0;
            break;
          case 12: // Money ship
            Load_level(Commonfilename, Map_objects[C].Mode - 1);

            Play_level();

            Map_objects[C].Mode = 0;
            if (Exit != 9)
              Exit = 0;
            break;
          case 14: // Card game
            Play_card_game();
            Map_objects[C].Mode = 0;
            break;
          };
      };
    };
  };
}

uint8_t Get_map_tile(int16_t X, int16_t Y) {

  return *(
      (char *)(Map_plane.p.matrix + (Y >> 4) * Map_plane.p.width + (X >> 4)));
}

void Put_map_tile(int16_t X, int16_t Y, uint8_t Tile) {

  *((char *)(Map_plane.p.matrix + (Y >> 4) * Map_plane.p.width + (X >> 4))) =
      Tile;
}

void Handle_boat(struct map_object *Boat) {

  int16_t Dx = 0, Dy = 0;

  if (Boat->MoveX > 0) {
    Dx = map_speed; // Boat->X += map_speed;
    // Boat->MoveX -= map_speed;
  };
  if (Boat->MoveX < 0) {
    Dx = -map_speed; // Boat->X -= map_speed;
    // Boat->MoveX += map_speed;
  };
  if (Boat->MoveY > 0) {
    Dy = map_speed; // Boat->Y += map_speed;
    // Boat->MoveY -= map_speed;
  };
  if (Boat->MoveY < 0) {
    Dy = -map_speed; // Boat->Y -= map_speed;
    // Boat->MoveY += map_speed;
  };

  Boat->Y += Dy;
  Boat->MoveY -= Dy;
  Boat->X += Dx;
  Boat->MoveX -= Dx;

  if (Map_plane.frame % 2) {

    if (Boat->Data0 == -1)
      Boat->Data1 = 1;
    if (Boat->Data0 == 1)
      Boat->Data1 = -1;

    Boat->Y += Boat->Data1;
    Boat->Data0 += Boat->Data1;
  };
}

void Handle_ship(struct map_object *Ship) {
  if (Ship->Data0 == -4)
    Ship->Data1 = 1;
  if (Ship->Data0 == 4)
    Ship->Data1 = -1;

  Ship->X += Ship->Data1;
  Ship->Data0 += Ship->Data1;
}

void Handle_map_monster(struct map_object *Monster) {
  int16_t C;

  C = 0;

  if (Move_map_objects) {

    Player.Passified = 1;

    Monster->X -= Monster->Data0;
    Monster->Data0 = 0;

    int16_t X = Monster->X; // small size optimisation
    int16_t Y = Monster->Y;

    switch (
        Map_plane
            .step) { // generates pseudo random direction for monster movement

    case 0: {

      if ((Monster->MoveX = Free_right_map(/*Monster->*/ X, /*Monster->*/ Y))) {
        Monster->Sprite = map_monster_right_spr;
        // Monster->Mask = map_monster_right_msk;
      };
      break;
    };
    case 1: {

      if ((Monster->MoveX = Free_left_map(/*Monster->*/ X, /*Monster->*/ Y))) {
        Monster->Sprite = map_monster_left_spr;
        // Monster->Mask = map_monster_left_msk;
      };

      break;
    };
    case 2: {

      Monster->MoveY = Free_up_map(/*Monster->*/ X, /*Monster->*/ Y);
      break;
    };
    case 3: {

      Monster->MoveY = Free_down_map(/*Monster->*/ X, /*Monster->*/ Y);
      break;
    };
    };

    // avoid settling at non-nodes
    uint8_t Temp = Get_map_tile(Monster->X + Monster->MoveX - Monster->Data0,
                                Monster->Y + Monster->MoveY);
    if (!((Temp >= walkable_node_low) && (Temp <= walkable_node_high))) {
      Monster->MoveX = Monster->MoveY =
          0; // abort, if target tile is not a node, i.e. a path
      //			Player.Passified=0;
      //			Move_map_objects=0;
      //			Player.Offset = 0;
    }

  } else {

    if (Monster->MoveX || Monster->MoveY) {

      // intside visible screen?
      if ((Monster->X >= (FgX - 16)) && (Monster->X <= (FgX + screen_width)) &&
          (Monster->Y >= (FgY - 16)) &&
          (Monster->Y <= (FgY + screen_height - 16))) {

        Player.Passified = 1;

        if (Monster->MoveX > 0) {
          Monster->X += 2;
          Monster->MoveX -= 2;
        };
        if (Monster->MoveX < 0) {
          Monster->X -= 2;
          Monster->MoveX += 2;
        };
        if (Monster->MoveY > 0) {
          Monster->Y += 2;
          Monster->MoveY -= 2;
        };
        if (Monster->MoveY < 0) {
          Monster->Y -= 2;
          Monster->MoveY += 2;
        };

      } else { // outside visible screen
        Monster->X += Monster->MoveX;
        Monster->Y += Monster->MoveY;
        Monster->MoveX = 0;
        Monster->MoveY = 0;
      }

    } else {

      if (Monster->Data0 == -8) {
        Monster->Data1 = 1;
        Monster->Sprite = map_monster_right_spr;
        // Monster->Mask = map_monster_right_msk;
      };
      if (Monster->Data0 == 8) {
        Monster->Data1 = -1;
        Monster->Sprite = map_monster_left_spr;
        // Monster->Mask = map_monster_left_msk;
      };
      Monster->X += Monster->Data1;
      Monster->Data0 += Monster->Data1;
    };
  };
};
/*
void Enter_map_anim(){

        short
LX=SavePlayer.MapX,RX=SavePlayer.MapX+16,UY=SavePlayer.MapY,LY=SavePlayer.MapY+16;

        do{

                Render_map();

//
GrayFastFillRect2B240_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),0,0,screen_width,UY,COLOR_DARKGRAY);
//
GrayFastFillRect2B240_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),0,0,LX,screen_height,COLOR_DARKGRAY);
                //GrayFastFilledRect_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),0,UY,LX,LY,COLOR_DARKGRAY);


//
GrayFastFillRect2B240_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),RX,0,screen_width,screen_height,COLOR_DARKGRAY);
                //GrayFastFilledRect_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),RX,UY,screen_width,LY,COLOR_DARKGRAY);

//
GrayFastFillRect2B240_R(GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE),0,LY,screen_width,screen_height,COLOR_DARKGRAY);


                if(LX>0)
                        LX--;
                if(RX<screen_width)
                        RX++;
                if(UY>0)
                        UY--;
                if(LY<screen_height)
                        LY++;

        }while( (LX>0) || (RX<screen_width) || (UY>0) || (LY<screen_height) );





}*/

void Enter_level_anim() {

  int16_t LX = 0, RX = screen_width, HY = 0, LY = screen_height, C;

  while ((LX < RX) || (LY > HY)) {
    if (LX < (SavePlayer.MapX - FgX))
      LX++;
    if (RX > (SavePlayer.MapX - FgX))
      RX--;
    if (LY > (SavePlayer.MapY - FgY))
      LY--;
    if (HY < (SavePlayer.MapY - FgY))
      HY++;

    GrayFastOutlineRect_R(GrayDBufGetActivePlane(LIGHT_PLANE),
                          GrayDBufGetActivePlane(DARK_PLANE), LX, HY, RX, LY,
                          COLOR_DARKGRAY);

    for (C = 0; C < 2500; C++)
      ;
  };

  // GrayDBufGetHiddenPlane(LIGHT_PLANE)
  // FastDrawLine_R(register unsigned char* plane asm("%a0"),register short x1
  // asm("%d0"),register short y1 asm("%d1"),register short x2
  // asm("%d2"),register short y2 asm("%d3"),short mode)
};

void Fight_monster(struct map_object *Monster) {
  int16_t OldFgX, OldFgY, C;

  Load_level(Commonfilename, Monster->Mode - 1);

  for (C = 0; C < Leveldata.Nr_of_enemies; C++) {
    Enemies[C].Life = Monster->Treasure;
  }
  /*
  OldFgX=FgX;
  OldFgY=FgY;

  Render();
  Playloop();

  FgX=OldFgX;
  FgY=OldFgY;
  */
  Play_level();

  SavePlayer.Attribs =
      SavePlayer.Attribs & 0b01110111; // disable star and p-wing
  Map_plane.p.force_update = 1;

  Treasure = 0;
  // Exit=0;
}

int16_t Free_left_map(int16_t X, int16_t Y) {
  int16_t DX = 0;

  while ((Get_map_tile(X - 16 + DX, Y) <= levels_high) &&
         ((X + DX) > Map_data.Border_left)) {
    DX -= 16;
    if (Get_map_tile(X + DX, Y) >= walkable_node_low) {
      break;
    };
  };

  return DX;
};

int16_t Free_right_map(int16_t X, int16_t Y) {
  int16_t DX = 0;

  while ((Get_map_tile(X + 16 + DX, Y) <= levels_high) &&
         ((X + DX + 16) < Map_data.Border_right)) {
    DX += 16;
    if (Get_map_tile(X + DX, Y) >= walkable_node_low) {
      break;
    };
  };

  return DX;
};

int16_t Free_up_map(int16_t X, int16_t Y) {
  int16_t DY = 0;

  while ((Get_map_tile(X, Y - 16 + DY) <= levels_high)) {
    DY -= 16;
    if (Get_map_tile(X, Y + DY) >= walkable_node_low) {
      break;
    };
  };

  return DY;
};

int16_t Free_down_map(int16_t X, int16_t Y) {
  int16_t DY = 0;

  while ((Get_map_tile(X, Y + 16 + DY) <= levels_high)) {
    DY += 16;
    if (Get_map_tile(X, Y + DY) >= walkable_node_low) {
      break;
    };
  };

  return DY;
};

void Replace_map_tile(uint8_t Tile) {
  int16_t C;

  for (C = 0; C < Map_data.Height * Map_data.Width; C++) {

    if (*((uint8_t *)(Map_plane.p.matrix + C)) == Tile) {
      Replace_by_road((C % Map_data.Width) * 16, (C / Map_data.Width) * 16);
    }
    /*else{

    }*/
  };
}

void Replace_by_road(int16_t X, int16_t Y) {
  uint8_t Tile1, Tile2;
  /*unsigned char Grass[6] = {0,1,15,16,18,22};
  unsigned char Sky[6] = {9,10,26,27,28,29};
  unsigned char Dark[6] = {11,12,35,36,37,38};*/
  /*
  //Old
  unsigned char Tiles[6*3]={
          0,1,15,16,18,22,//grass
          9,10,26,27,28,29,//sky
          11,12,35,36,37,38,//dark
  };*/

  uint8_t Tiles[6 * 3] = {
      0,  1,  17, 18, 20, 24, // grass
      9,  10, 28, 29, 30, 31, // sky
      11, 12, 37, 38, 39, 40, // dark
  };

  // scan 3x3 area around given tile to find which tileset/theme to use...
  /*	short Set=-1;
          short R,C;
          for(C=-16;(C=<16) && (Set==-1);C+=16){
                  for(R=-16;(R=<16) && (Set==-1);R+=16){
                          Set = IsInTileSet( Tiles , Get_tile(X+C,Y+R) );
                  }
          }*/

  /*if( (X==0) || (Y==0) || (X==(Map_data.Height)) || (Y==(Map_data.Width))
  ){//if on the edge of the map... Put_map_tile(X,Y,);//no road, plain ground
  instead
  }*/

  int16_t TileNr;

  if (Get_map_tile(X - 16, Y) <= walkable_high) {   //-
    if (Get_map_tile(X, Y - 16) <= walkable_high) { //_| found
      // put tile: 18
      //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 18;
      //			Put_map_tile(X,Y,18);
      TileNr = 4;
    } else {
      if (Get_map_tile(X, Y + 16) <= walkable_high) { //^^| found
        // put tile: 15
        //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 15;
        //				Put_map_tile(X,Y,15);
        TileNr = 2;
      } else { // must be - - now.
        // put tile: 0
        //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 0;
        //				Put_map_tile(X,Y,0);
        TileNr = 0;
      }
    }
  } else { // not -
    // IF THIS SHOWS TO BE BUGGY: tRY LOOKIUNG FOR _| FIRST
    if (Get_map_tile(X, Y - 16) <= walkable_high) {   //|
      if (Get_map_tile(X + 16, Y) <= walkable_high) { //|_ found
        // put tile : 16
        //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 16;
        //				Put_map_tile(X,Y,16);
        TileNr = 3;
      } else { // must be  |
        //							|
        // put tile: 1
        //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 1;
        //				Put_map_tile(X,Y,1);
        TileNr = 1;
      }
    } else { // not |
      // only legal option remaining: |^^
      // put tile: 22
      //*( (char*)(Map_plane.p.matrix+(Y>>4)*Map_plane.p.width+(X>>4)) ) = 22;
      //			Put_map_tile(X,Y,22);
      TileNr = 5;
    }
  }

  Put_map_tile(X, Y, Tiles[6 * Map_data.Color + TileNr]);
};
/*
void Add_money_ship(){
        short C;

        for(C=0;C<Map_data.Nr_of_objects;C++){//big castle=>releases ship
                if(!Map_objects[C].Mode){
                        Map_objects[C].X = Leveldata.EX*16;
                        Map_objects[C].Y = Leveldata.EY*16;
                        Map_objects[C].Mode = 12;
                        Map_objects[C].Data0 = 0;
                        Map_objects[C].Data1 = 1;
                        Map_objects[C].Sprite = money_ship_spr;
                        //Map_objects[C].Mask = ship_msk;
                        (map_object*)Map_objects[C].Handler = Handle_ship;
                        break;
                };
        };



};

void Add_card_game(){
        short C;

        for(C=0;C<Map_data.Nr_of_objects;C++){//big castle=>releases ship
                if(!Map_objects[C].Mode){
                        Map_objects[C].X = Leveldata.EX*16;
                        Map_objects[C].Y = Leveldata.EY*16;
                        Map_objects[C].Mode = 14;
                        Map_objects[C].Data0 = 0;
                        Map_objects[C].Data1 = 1;
                        Map_objects[C].Sprite = card_game_spr;
                        //Map_objects[C].Mask = ship_msk;
                        (map_object*)Map_objects[C].Handler = Handle_ship;
                        break;
                };
        };

};
*/

void Add_map_event(uint8_t Event) {
  int16_t C;

  if ((Event >= 240) && (Event <= 250)) { //
    Exit = 0;
    // Fg_plane.p.force_update = Fg_mask.p.force_update = 0;
    Skip_anim = 1;
    Enter_mushrom_house(Event - 187);

  } else {

    for (C = 0; C < Map_data.Nr_of_objects; C++) { // big castle=>releases ship
      if (!Map_objects[C].Mode) {
        Map_objects[C].X = Leveldata.EX * 16;
        Map_objects[C].Y = Leveldata.EY * 16;
        Map_objects[C].Data0 = 0;
        Map_objects[C].Data1 = 1;
        Map_objects[C].Handler = (void (*)(void *))Handle_ship;

        uint16_t *Sprite;
        char Mode;

        if (Event == 1) {
          /*Map_objects[C].*/ Sprite = card_game_spr;
          /*Map_objects[C].*/ Mode = 14;
        }
        if (Event == 5) {
          /*Map_objects[C].*/ Sprite = money_ship_spr;
          /*Map_objects[C].*/ Mode = 12;
        }

        if (Event > 50) {
          /*Map_objects[C].*/ Sprite = mhouse_hidden_spr;
          /*Map_objects[C].*/ Mode = 10;
          Map_objects[C].Treasure = Event;
        }

        Map_objects[C].Sprite = Sprite;
        Map_objects[C].Mode = Mode;
        /*
        switch(Event){
                case 1:
                        Map_objects[C].Sprite = card_game_spr;
                        Map_objects[C].Mode = 14;
                break;
                case 5:
                        Map_objects[C].Sprite = money_ship_spr;
                        Map_objects[C].Mode = 12;
                break;
        }
        */

        break;
      };
    };
  };
};
