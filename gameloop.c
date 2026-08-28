#include "gameloop.h"
#include "bosses.h"
#include "bounch.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "compat/gray.h"
#include "control.h"
#include "custom.h"
#include "flying.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "levelset.h"
#include "map.h"
#include "menus.h"
#include "player.h"
#include "render.h"
#include "savegame.h"
#include "scankeys.h"
#include "shells.h"
#include "stringcopy.h"
#include <string.h>

char *Filenames;

char Levelsetfile[9];
struct map_object *Map_objects;
struct map_trigger *Map_triggers;

char Move_map_objects;

void Play_level() {
  short OldFgX = FgX;
  short OldFgY = FgY;

  //	Adjust_renderpoint();
  //	Render();
  Playloop();

  FgX = OldFgX;
  FgY = OldFgY;
}

void Playloop() {
  /*short OldFgX,OldFgY;
  OldFgX=FgX;	OldFgY=FgY;*/

  //	int frames=0;//test
  short C;
  long int D; // test

  // New: V 1.01:
  char PrevLives = SavePlayer.Lives;
  // end of New

  while (!(Exit)) {

#ifdef speedtest
    Fps++; // speed test
#endif

    Adjust_renderpoint();

// Apply frame skipping on TI 92+ and V200 to gain speed
#ifdef PRODUCE_TI92PLUS_CODE

    static short FrameSkip = 0;
    if ((++FrameSkip) ==
        4) { // skipping each n'th frame (don't draw it, still calculated)
      FrameSkip = 0; // gives a good speed increase
    } else {
      Render();
    }

#endif

#ifdef PRODUCE_V200_CODE

    static short FrameSkip = 0;
    if ((++FrameSkip) ==
        4) { // skipping each n'th frame (don't draw it, still calculated)
      FrameSkip = 0; // gives a good speed increase
    } else {
      Render();
    }

#endif
// No need for frame skipping on TI 89
#ifdef PRODUCE_TI89_CODE

    Render();

#endif

    Scankeys();

    /*		if(Keystate.Enter && !Previous_keystate.Enter)
                            Slow = (Slow==0?1:0);*/

#ifdef immortal_mode
    // Immortal mode for debugging
    SavePlayer.Attribs = SavePlayer.Attribs | 0b10000000;
    Player.Immortal = star_immortal_time;
#endif

    for (C = 0; C < nr_of_bounching_tiles; C++) {
      if (Bounching_tiles[C].Active)
        Bounching_tiles[C].Active--; // decrement timer
    };

    Handleplayer();

    Player_handle_fireballs();

    Handle_enemyshots();

    Player_bounching_shell_hadler();

    Handleenemies();

    Handleitems();

    // Handle_objects();

    Handle_flying_platforms();

    Custom();

    if (Leveldata.Boss)
      Handlebosses();

    Previous_keystate = Keystate;

    /*if(Slow)
            for(D=0;D<160000;D++);//slow motion stuff, to study details...*/
  };

  // FgX=OldFgX;	FgY=OldFgY;

  if (Player.Height == 18) {
    Player.Height = Player.Height2 = 27;
  }
  SavePlayer.Attribs =
      SavePlayer.Attribs & 0b01110111; // disable star and p-wing

  // New: V 1.05: Maximum lives = 99. Prevents player from getting game over
  // when lives exceeds 127
  //  Also prevents displaying problems
  if (SavePlayer.Lives > 99) {
    SavePlayer.Lives = 99;
  };

  // New: V 1.01:
  if (PrevLives > SavePlayer.Lives) {
    SavePlayer.Lives =
        PrevLives - 1; // Make sure you never loose more that 1 life at a time.
                       // There are some indications that it occurs rarely...
  }
  // end of New
}

void Gameloop() {
  short C;
  // long D;//for test

  while ((!Exit) && (ErrorCode == 0) && (SavePlayer.Lives > 0)) {

    Render_map();

    if (Map_plane.frame % 2)
      Contrast_adj();

    for (C = 0; C < 10000; C++)
      ; // time delay, will probably do it different later...

    // for(D=0;D<80000;D++);

    Previous_keystate = Keystate;

    if (!Player.Offset) {
      Scankeys();
      if (Keystate.Esc) { // mid game menu

        /*char*/ short Res = doMenu(Texts + GameTextData.MidGameMap, 3);

        switch (Res) {
          short Offset;
        case 1: // continue
          //					Render_map();
          //					while(1);
          break;
        case 2: // save
          // Savegame(0);

          /*strcpy(Fg_plane.p.big_vscreen,Texts+GameTextData.Save);//Title
          C = Offset = strlen(Fg_plane.p.big_vscreen)+1;*/
          {
            char *TempBuffer = Fg_plane.p.big_vscreen;
            //							C = Offset =
            // StringCopy(TempBuffer/*Fg_plane.p.big_vscreen*/,Texts+GameTextData.Save);

            Paste_saveslot_text(TempBuffer /*Fg_plane.p.big_vscreen*/,
                                Texts + GameTextData.Save); // Offset);
            Res = doMenu(TempBuffer /*Fg_plane.p.big_vscreen*/, 3);
          }

          WaitKeyReleased(); // while(_rowread(0));//wait until key released
          //						while(!_rowread(0)){//wait
          // until key pressed

          if ((Res >= 1) && (Res <= 3)) {
            //
            short SaveOk = 1;
            if (Levelsetdata.Savegames[Res - 1] != 0) {

              // New: V 1.03 Added confirmation when overwriting a saveslot

              if (doMenu(Texts + GameTextData.OverWrite, 2) == 1)
                SaveOk = 0;
            }
            // End of New
            if (SaveOk)
              Savegame(Res - 1);
          }

          break;
        case 3: // quit
          Exit = 1;
          break;
        case 0: // esc => continue

          //					Keystate.Esc = 0;

          // while( !(_rowread(0)) );
          break;
        };

        WaitKeyReleased(); // while( (_rowread(0)) );
        Keystate.Jump = 0;

      }; // Keystate.Esc
    } else {
      Player.Passified = 1;
    };

    Move_map_objects = 0;

    if (Player.Passified) {
      /*Keystate.Up    = 0;
      Keystate.Left  = 0;
      Keystate.Right = 0;
      Keystate.Down  = 0;
      Keystate.Jump  = 0;
      Keystate.Run   = 0;*/

      memset(&Keystate, 0, sizeof(struct keystate));
    };

    Handle_player_map();

    Player.Passified = 0;

    Handle_map_objects();
  };
};

void New_world_screen() {

  /*FastFilledRect_Erase_R(dBufHPL_G,0,0,239,127);
  FastFilledRect_Draw_R(dBufHPD_G,0,0,239,127);
  FastFilledRect_Erase_R(dBufHPD_G,32,32,128,68);*/

  //	GrayDBufToggleSync_SetPointers();

  Render_map();

  unsigned short *ActiveLight = GrayDBufGetActivePlane(LIGHT_PLANE);
  unsigned short *ActiveDark = GrayDBufGetActivePlane(DARK_PLANE);

  /*	FastFilledRect_Erase_R(ActiveLight,32,32,128,68);
          FastFilledRect_Erase_R(ActiveDark,32,32,128,68);
          GrayFastOutlineRect_R(ActiveLight,ActiveDark,31,31,129,69,COLOR_BLACK);*/

  FastFilledRect_Erase_R(ActiveLight, screen_width / 2 - 40,
                         screen_height / 2 - 20, screen_width / 2 + 40,
                         screen_height / 2 + 20);
  FastFilledRect_Erase_R(ActiveDark, screen_width / 2 - 40,
                         screen_height / 2 - 20, screen_width / 2 + 40,
                         screen_height / 2 + 20);
  //	GrayFastFillRect_R(ActiveLight,ActiveDark,
  // screen_width/2-40,screen_height/2-20,screen_width/2+40,screen_height/2+20,COLOR_WHITE);
  GrayFastOutlineRect_R(ActiveLight, ActiveDark, screen_width / 2 - 41,
                        screen_height / 2 - 21, screen_width / 2 + 41,
                        screen_height / 2 + 21, COLOR_BLACK);

  char String[11] = "WORLD  ";

  String[6] = Levelsetdata.CurrentWorld + '1';

  //	DrawGrayStrExt2B(36,36,String,A_REPLACE|A_SHADOWED,F_6x8,ActiveLight,ActiveDark);
  DrawGrayStrExt2B(screen_width / 2 - 42 / 2, screen_height / 2 - (6 + 8),
                   String, A_REPLACE | A_SHADOWED, F_6x8, ActiveLight,
                   ActiveDark);
  // DrawString(screen_width/2-42/2,screen_height/2-(6+8),String,A_REPLACE|A_SHADOWED,F_6x8);

  StringCopy(String, "MARIO x ");

  char Lives = SavePlayer.Lives;

  if (Lives >= 10) {
    String[8] = '0' + Lives / 10;
  } else {
    String[8] = ' ';
  };

  String[9] = '0' + Lives % 10;

  String[10] = 0;

  DrawGrayStrExt2B(screen_width / 2 - 60 / 2, screen_height / 2 + 6, String,
                   A_REPLACE | A_SHADOWED, F_6x8, ActiveLight, ActiveDark);
  // DrawString(screen_width/2-60/2,screen_height/2+6,String,A_REPLACE|A_SHADOWED,F_6x8);

  //	unsigned short* Sprite = Mariosprites+2*Marioanimtab[0][9];

  //	GrayClipSprite16_SMASK_R(80,50,16,Sprite,Sprite+16,Sprite+32,ActiveLight,ActiveDark);

  //	GrayDBufToggleSync_SetPointers();

  // This screen is drawn into the active plane, which the LCD showed as it
  // was written; the canvas has to be told about it before we go and wait.
  GrayDBufRefresh();

  /*while(_rowread(0));//wait until key released
  while(!_rowread(0));//wait until key pressed*/
  WaitKeyPress();
};

short RunGame(char Saveslot) {
  //	SYM_ENTRY *Levelsetfile_sym;
  HANDLE Temp;
  short C;

  // memcpy( Filenames, HeapDeref (Temp)+2+sizeof(levelsetdata)+21,
  // 9*Levelsetdata.Nr_of_files );
  Playerinit();

  if (Saveslot >= 0) { // load game
    Player.Offset = 0;
    Loadgame(Saveslot /*,HeapDeref (Temp)+2*/);

  } else { // new game
    Levelsetdata.CurrentWorld = 0;
  }

  //	HeapUnlock(	Levelsetfile_sym->handle );
  // DrawGrayStrExt2B(142,88,Buffer,A_REPLACE,F_4x6,GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE));

  // DrawStr (20, 20, buffer, A_NORMAL);//test

  // for(C=0;C<Levelsetdata.Nr_of_files;C++){

  char Warp = 0;
  do {

    Keystate.Esc = Exit = 0;

    // memcpy( &Levelfilename, HeapDeref (Temp)+2+sizeof(levelsetdata)+21+9*C, 9
    // ); memcpy( &Levelfilename, HeapDeref
    // (Temp)+2+sizeof(levelsetdata)+21+9*Levelsetdata.CurrentWorld, 9 );

    StringCopy(Levelfilename, Filenames + 9 * Levelsetdata.CurrentWorld);
    // memcpy(Levelfilename,Filenames+9*Levelsetdata.CurrentWorld,9);
    /*
    DrawGrayStrExt2B(17,30,Levelfilename,A_REPLACE,F_6x8,GrayDBufGetActivePlane(LIGHT_PLANE),GrayDBufGetActivePlane(DARK_PLANE));
    while(_rowread(0));//wait until key released
    while(!_rowread(0));//wait until key pressed
    */
    //		Draw_statusbar();

    // if(Levelsetdata.Mode==1){//map mode

    if (Saveslot < 0)

      Load_map(Levelfilename);
    New_world_screen();
    /*		if(Warp)
                            Load_map(Commonfilename);

                    Warp=0;*/

    Saveslot = -1;

    Gameloop();

    // New: V 1.01: Changed ==0 to <=0
    if (SavePlayer.Lives <= 0) { //==0){//BUG!
      // Game over
      // menu: Game Over, Continue, Quit
      short Res = doMenu(Texts + GameTextData.GameOver, 2);

      if (Res == 1) {

        Exit = 0;
        Playerinit();
        Player.Offset = 1;
      } else {
        Exit = 9;
      }
    };

    if (Exit == 2) {
      Levelsetdata.CurrentWorld++;
      Exit = 0;
    };
#ifdef debug
    if (Exit == 1) { // temp,debug
      Levelsetdata.CurrentWorld++;
      Exit = 0;
    };
#endif
    if (Exit == 3) {
      Exit = 0;
    };

    /*		if(Exit==100){//Warp Zone
                            Warp = 1;
                            Exit=0;
                    }*/

  } while ((Levelsetdata.CurrentWorld < Levelsetdata.Nr_of_files) &&
           (ErrorCode == 0) && (Exit == 0));

  //	free(Filenames);

  return 1;
}
