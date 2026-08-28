

#include "all.h"

char *CardGame;

char *Games;

static short RollX_176(short X) {

  if (X >= 176) { // 352
    return X - 176;
  } else {
    if (X < 0) {
      return X + 176;
    } else {
      return X;
    }
  };
};

static short Find_stop_pos(short X) {
  return ((X - 22) / 44) * 44 + (((X - 22) % 44) > 22 ? 44 : 0) + 22;
}

static short Find_stopping_D(short Stopdist) {
  short D;

  Stopdist = abs(Stopdist);

  if (Stopdist <= 22) {
    D = 1;
  } else {
    if (Stopdist <= 44) {
      D = 2;
    } else {
      if (Stopdist <= 66) {
        D = 3;
      } else {
        D = 4;
      }
    }
  }

  return D;
}

void Enter_game_house() {

  /*char*/ short Playing = 1;
  /*char*/ short State = 0;
  short X1 = 0, X2 = 44, X3 = 88;
  short Stopping = 0;
  short Sprite;
  unsigned char AnimCount = 0;
  static const /*char*/ short Sequence[4] = {0, 2, 0, 1}; //,0,2,0,1};
  //	void *dBufHPL, *dBufHPD;

  while (Playing) { //! Keystate.Esc

    /*
    memset(GrayDBufGetHiddenPlane(LIGHT_PLANE),0x0000,LCD_SIZE);
    memset(GrayDBufGetHiddenPlane(DARK_PLANE),0x0000,LCD_SIZE);
    */
    /*
    FastFilledRect_Erase_R(GrayDBufGetHiddenPlane(LIGHT_PLANE),0,0,239,127);
    FastFilledRect_Erase_R(GrayDBufGetHiddenPlane(DARK_PLANE),0,0,239,127);
    */

    /*dBufHPL = GrayDBufGetHiddenPlane(LIGHT_PLANE);
    dBufHPD = GrayDBufGetHiddenPlane(DARK_PLANE);*/

    GrayClearScreen2B(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                      /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);

    short C, D;
    for (C = 0; C < 4; C++) {
      // 43 +16
      /*
      GrayClipSprite16_SMASK_R(66,30,12,game_mushrom_l+4*38*2,game_mushrom_l+38+4*38*2,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      GrayClipSprite16_SMASK_R(66+16,30,12,game_mushrom_r+4*38*2,game_mushrom_r+38+4*38*2,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      */
      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X1)+screen_offset_sg_x,30,12,game_mushrom_l+4*38*Sequence[C],game_mushrom_l+38+4*38*Sequence[C],blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X1+16)+screen_offset_sg_x,30,12,game_mushrom_r+4*38*Sequence[C],game_mushrom_r+38+4*38*Sequence[C],blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      DrawSprite16SMASKR(RollX_176(C * 44 + X1) + screen_offset_sg_x, 30, 12,
                         game_mushrom_l + 4 * 38 * Sequence[C],
                         game_mushrom_l + 38 + 4 * 38 * Sequence[C],
                         blank_sprite);
      DrawSprite16SMASKR(RollX_176(C * 44 + X1 + 16) + screen_offset_sg_x, 30,
                         12, game_mushrom_r + 4 * 38 * Sequence[C],
                         game_mushrom_r + 38 + 4 * 38 * Sequence[C],
                         blank_sprite);

      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X2)+screen_offset_sg_x,42,16,game_mushrom_l+4*38*Sequence[C]+12,game_mushrom_l+38+4*38*Sequence[C]+12,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X2+16)+screen_offset_sg_x,42,16,game_mushrom_r+4*38*Sequence[C]+12,game_mushrom_r+38+4*38*Sequence[C]+12,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      DrawSprite16SMASKR(RollX_176(C * 44 + X2) + screen_offset_sg_x, 42, 16,
                         game_mushrom_l + 4 * 38 * Sequence[C] + 12,
                         game_mushrom_l + 38 + 4 * 38 * Sequence[C] + 12,
                         blank_sprite);
      DrawSprite16SMASKR(RollX_176(C * 44 + X2 + 16) + screen_offset_sg_x, 42,
                         16, game_mushrom_r + 4 * 38 * Sequence[C] + 12,
                         game_mushrom_r + 38 + 4 * 38 * Sequence[C] + 12,
                         blank_sprite);

      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X3)+screen_offset_sg_x,58,10,game_mushrom_l+4*38*Sequence[C]+28,game_mushrom_l+38+4*38*Sequence[C]+28,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      // GrayClipSprite16_SMASK_R(RollX_176(C*44+X3+16)+screen_offset_sg_x,58,10,game_mushrom_r+4*38*Sequence[C]+28,game_mushrom_r+38+4*38*Sequence[C]+28,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
      DrawSprite16SMASKR(RollX_176(C * 44 + X3) + screen_offset_sg_x, 58, 10,
                         game_mushrom_l + 4 * 38 * Sequence[C] + 28,
                         game_mushrom_l + 38 + 4 * 38 * Sequence[C] + 28,
                         blank_sprite);
      DrawSprite16SMASKR(RollX_176(C * 44 + X3 + 16) + screen_offset_sg_x, 58,
                         10, game_mushrom_r + 4 * 38 * Sequence[C] + 28,
                         game_mushrom_r + 38 + 4 * 38 * Sequence[C] + 28,
                         blank_sprite);
    }

    // FastFilledRect_Draw_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,0,0,239,25);
    // FastFilledRect_Draw_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,0,75,239,127);
    FilledRectDark(0, 0, 239, 25);
    FilledRectDark(0, 75, 239, 127);

    // FastFilledRect_Draw_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,0,25,43+screen_offset_sg_x,75);
    // FastFilledRect_Draw_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,119+screen_offset_sg_x,25,239,75);
    FilledRectDark(0, 25, 43 + screen_offset_sg_x, 75);
    FilledRectDark(119 + screen_offset_sg_x, 25, 239, 75);

    // FastFilledRect_Erase_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,0,25,43+screen_offset_sg_x,75);
    // FastFilledRect_Erase_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,119+screen_offset_sg_x,25,239,75);
    FilledRectEraseLight(0, 25, 43 + screen_offset_sg_x, 75);
    FilledRectEraseLight(119 + screen_offset_sg_x, 25, 239, 75);

    // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,41+screen_offset_sg_x,23,121+screen_offset_sg_x,77,COLOR_WHITE);//COLOR_BLACK
    // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,40+screen_offset_sg_x,22,122+screen_offset_sg_x,78,COLOR_WHITE);//COLOR_BLACK
    GrayOutlineRect(41 + screen_offset_sg_x, 23, 121 + screen_offset_sg_x, 77,
                    COLOR_WHITE);
    GrayOutlineRect(40 + screen_offset_sg_x, 22, 122 + screen_offset_sg_x, 78,
                    COLOR_WHITE);

    if (State == 5) { // match

      if ((AnimCount / 16) % 2) {
        // if((AnimCount>>4)&0b00000001){//this optimization didn't improve
        // anything :-(
        // GrayClipSprite16_SMASK_R(79+screen_offset_sg_x,44,15,up_sprite,up_sprite+15,blank_sprite,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
        DrawSprite16SMASKR(79 + screen_offset_sg_x, 44, 15, up_sprite,
                           up_sprite + 15, blank_sprite);
        GrayClipSprite8_SMASK_R(
            67 + screen_offset_sg_x, 44, 15, Smallsprites + Sprite,
            Smallsprites + Sprite + 15, (unsigned char *)blank_sprite,
            /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
            /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
      };
    }

    if ((State == 5) || (State == 4)) {
      if ((++AnimCount) > 160)
        Playing = 0;
    }

    GrayDBufToggleSync_SetPointers(); // GrayDBufToggleSync();//set drawbuffer
                                      // as active screen

    Previous_keystate = Keystate;

    Scankeys();

    if (Keystate.Run && !(Previous_keystate.Run) && !(Stopping)) {

      short TempX;

      switch (State) {
      case 0:
        Stopping = Find_stop_pos(X1) - X1 + 176;
        break;
      case 1:
        // Stopping = (Find_stop_pos(X2) - X2) - 176;//denne virker nesten: noen
        // få pixler for langt til høyre
        Stopping =
            -(176 + (X2 - Find_stop_pos(X2))); //(Find_stop_pos(X2) - X2) - 176;

        break;
      case 2:
        Stopping = Find_stop_pos(X3) - X3 + 176;
        break;
      }
    }

    if (State == 0) {

      if (Stopping) {

        D = Find_stopping_D(Stopping);
        Stopping -= D;
        if (Stopping == 0) {
          State++;
        }
      } else {
        D = 4;
      };

      X1 += D;
      if (X1 >= 176) {
        X1 = X1 - 176;
      }
    }

    if (State <= 1) {

      if (Stopping && (State == 1)) {
        D = Find_stopping_D(Stopping);

        Stopping += D;
        if (Stopping == 0) {
          State++;
        }
      } else {
        D = 4;
      }
      X2 -= D;
      if (X2 < 0)
        X2 = 176 + X2;
    }

    if (State <= 2) {

      if (Stopping && (State == 2)) {
        D = Find_stopping_D(Stopping);

        Stopping -= D;
        if (Stopping == 0) {
          State++;
        }
      } else {
        D = 4;
      }
      X3 += D;
      if (X3 >= 176) {
        X3 = X3 - 176;
      }
    }

    if (State == 3) { // check if match
      State = 4;
      switch (X1) {

      case 22: // flower

        if ((X2 == 22) && (X3 == 22)) {
          SavePlayer.Lives += 3;
          Sprite = 24 + 2 * 30;
          State = 5;
        }

        break;
      case 110: // star

        if ((X2 == 110) && (X3 == 110)) {
          SavePlayer.Lives += 5;
          Sprite = 24 + 3 * 30;
          State = 5;
        }

        break;
      case 66:
      case 154: // mushrom

        if (((X2 == 66) || (X2 == 154)) && ((X3 == 66) || (X3 == 154))) {
          SavePlayer.Lives += 2;
          Sprite = 24 + 30;
          State = 5;
        }

        break;
      }
    }

    for (C = 0; C < 7500; C++)
      ; // delay
  }
};

void New_card_game() {
  short C;
  /*
          static const char Games[5][18]={
                  {-3,-2,-4,-3,-4,-5,
                   -1,-5,-3,-2,-6,-1,
                   -2,-1,-6,-1,-3,-2,
                  },
                  {-1,-4,-5,-3,-2,-1,
                   -3,-2,-2,-4,-1,-6,
                   -6,-1,-3,-5,-3,-2,
                  },*/
  /*{
   -1,-3,-6,-2,-5,-3,
   -4,-6,-5,-1,-4,-2,
   -1,-2,-1,-2,-3,-3,
  },*/
  /*		{
                   -3,-6,-1,-2,-4,-3,
                   -4,-3,-5,-1,-6,-2,
                   -1,-2,-5,-3,-1,-2,
                  },
                  {
                   -5,-1,-1,-4,-2,-6,
                   -1,-2,-1,-6,-3,-1,
                   -2,-4,-3,-1,-5,-2,
                  },
                  {
                   -2,-1,-5,-4,-1,-3,
                   -3,-2,-1,-2,-5,-6,
                   -6,-2,-3,-1,-3,-4,
                  },
          };
          */
  //	memcpy(CardGame,Games[Map_plane.step],18);
  /*
  //debug
          if( (Map_plane.step>4) || (Map_plane.step<0) )
                  while(1);
          */
  memcpy(CardGame, Games + 18 * Map_plane.step, 18);
}

void SetPixel(
    short X,
    short Y) { // used in cardgame. Slower, but gives size optimization.
  EXT_SETPIX(dBufHPL_G, X, Y);
  EXT_SETPIX(dBufHPD_G, X, Y);
}

void Play_card_game() {

  short CX = 0, CY = 0, PX = -1, PY = -1;
  unsigned short *Sprite;
  short C, D = 0;
  //	void *dBufHPL, *dBufHPD;

  for (C = 0; C < 18; C++) {
    if (*(CardGame + C) >= 0) {
      D++;
    }
  }

  if (D == 18) {
    New_card_game();
  }

  short Miss = 0, Wait = 0, Flash = 0, Turn = 0;
  while (Miss < 2) {

    /*
    memset(GrayDBufGetHiddenPlane(LIGHT_PLANE),0xffff,LCD_SIZE);
    memset(GrayDBufGetHiddenPlane(DARK_PLANE),0x0000,LCD_SIZE);
    */

    /*dBufHPL = GrayDBufGetHiddenPlane(LIGHT_PLANE);
    dBufHPD = GrayDBufGetHiddenPlane(DARK_PLANE);*/

    FastFilledRect_Draw_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G, 0,
                          0, 239, 127); //,0,25,43,75);
    // FastFilledRect_Erase_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,0,0,239,127);
    FilledRectEraseDark(0, 0, 239, 127);

    // Draw cursor
    // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,2+screen_offset_sg_x+26*CX,1+screen_offset_sg_y+32*CY,27+screen_offset_sg_x+26*CX,34+screen_offset_sg_y+32*CY,COLOR_BLACK);//COLOR_DARKGRAY
    // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,3+screen_offset_sg_x+26*CX,2+screen_offset_sg_y+32*CY,26+screen_offset_sg_x+26*CX,33+screen_offset_sg_y+32*CY,COLOR_BLACK);//COLOR_BLACK
    GrayOutlineRect(2 + screen_offset_sg_x + 26 * CX,
                    1 + screen_offset_sg_y + 32 * CY,
                    27 + screen_offset_sg_x + 26 * CX,
                    34 + screen_offset_sg_y + 32 * CY, COLOR_BLACK);
    GrayOutlineRect(3 + screen_offset_sg_x + 26 * CX,
                    2 + screen_offset_sg_y + 32 * CY,
                    26 + screen_offset_sg_x + 26 * CX,
                    33 + screen_offset_sg_y + 32 * CY, COLOR_BLACK);

    // draw cards
    for (C = 0; C < 6; C++) {
      for (D = 0; D < 3; D++) {

        // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,4+screen_offset_sg_x+26*C,3+screen_offset_sg_y+32*D,23+screen_offset_sg_x+26*C,30+screen_offset_sg_y+32*D,COLOR_BLACK);//COLOR_DARKGRAY
        // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,24+screen_offset_sg_x+26*C,6+screen_offset_sg_y+32*D,25+screen_offset_sg_x+26*C,32+screen_offset_sg_y+32*D,COLOR_DARKGRAY);//COLOR_DARKGRAY
        // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,7+screen_offset_sg_x+26*C,31+screen_offset_sg_y+32*D,23+screen_offset_sg_x+26*C,32+screen_offset_sg_y+32*D,COLOR_DARKGRAY);//COLOR_DARKGRAY
        GrayOutlineRect(4 + screen_offset_sg_x + 26 * C,
                        3 + screen_offset_sg_y + 32 * D,
                        23 + screen_offset_sg_x + 26 * C,
                        30 + screen_offset_sg_y + 32 * D, COLOR_BLACK);
        GrayOutlineRect(24 + screen_offset_sg_x + 26 * C,
                        6 + screen_offset_sg_y + 32 * D,
                        25 + screen_offset_sg_x + 26 * C,
                        32 + screen_offset_sg_y + 32 * D, COLOR_DARKGRAY);
        GrayOutlineRect(7 + screen_offset_sg_x + 26 * C,
                        31 + screen_offset_sg_y + 32 * D,
                        23 + screen_offset_sg_x + 26 * C,
                        32 + screen_offset_sg_y + 32 * D, COLOR_DARKGRAY);

        //				EXT_SETPIX(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,5+26*C+screen_offset_sg_x,4+screen_offset_sg_y+32*D);EXT_SETPIX(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,5+screen_offset_sg_x+26*C,4+screen_offset_sg_y+32*D);
        //				EXT_SETPIX(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,22+26*C+screen_offset_sg_x,4+screen_offset_sg_y+32*D);EXT_SETPIX(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,22+screen_offset_sg_x+26*C,4+screen_offset_sg_y+32*D);
        //				EXT_SETPIX(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,22+26*C+screen_offset_sg_x,29+screen_offset_sg_y+32*D);EXT_SETPIX(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,22+screen_offset_sg_x+26*C,29+screen_offset_sg_y+32*D);
        //				EXT_SETPIX(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,5+26*C+screen_offset_sg_x,29+screen_offset_sg_y+32*D);EXT_SETPIX(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,5+screen_offset_sg_x+26*C,29+screen_offset_sg_y+32*D);

        SetPixel(5 + 26 * C + screen_offset_sg_x,
                 4 + screen_offset_sg_y + 32 * D); // same as above, but smaller
        SetPixel(22 + 26 * C + screen_offset_sg_x,
                 4 + screen_offset_sg_y + 32 * D);
        SetPixel(22 + 26 * C + screen_offset_sg_x,
                 29 + screen_offset_sg_y + 32 * D);
        SetPixel(5 + 26 * C + screen_offset_sg_x,
                 29 + screen_offset_sg_y + 32 * D);

        // EXT_CLRPIX
        // EXT_SETPIX(plane,x,y)

        if ((CardGame[C + 6 * D] < 0)) {
          // GrayClipSprite16_MASK_R(6+26*C,6+32*D,11,card_top,card_top+22,blank_sprite,blank_sprite,GrayDBufGetHiddenPlane(DARK_PLANE),GrayDBufGetHiddenPlane(LIGHT_PLANE));
          // GrayClipSprite16_MASK_R(6+26*C,6+11+32*D,11,card_top+11,card_top+22+11,blank_sprite,blank_sprite,GrayDBufGetHiddenPlane(DARK_PLANE),GrayDBufGetHiddenPlane(LIGHT_PLANE));
          //					GrayClipSprite16_SMASK_R(6+screen_offset_sg_x+26*C,6+screen_offset_sg_y+32*D,11,card_top,card_top+22,blank_sprite,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL);
          //					GrayClipSprite16_SMASK_R(6+screen_offset_sg_x+26*C,6+screen_offset_sg_y+11+32*D,11,card_top+11,card_top+22+11,blank_sprite,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL);

          GrayClipSprite16_SMASK_R(
              6 + screen_offset_sg_x + 26 * C, 6 + screen_offset_sg_y + 32 * D,
              22, card_top, card_top + 22, blank_sprite, dBufHPD_G, dBufHPL_G);
          // DrawSprite16SMASKR(6+screen_offset_sg_x+26*C,6+screen_offset_sg_y+32*D,22,card_top,card_top+22,blank_sprite);

        } else {

          // draw card top or  content if not flashing
          if ((!Flash) || ((Flash % 2) || (((C != PX) || (D != PY)) &&
                                           ((C != CX) || (D != CY))))) {

            if (CardGame[C + 6 * D] <= 2) {
              Sprite =
                  (CardGame[C + 6 * D] == 1 ? pu_mushrom_sprite : (star_anim));
              // GrayClipSprite16_MASK_R(6+26*C,9+32*D,16,Sprite,Sprite+16,blank_sprite,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
              // GrayClipSprite16_SMASK_R(6+26*C,9+32*D,16,Sprite,Sprite+16,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
              // GrayClipSprite16_SMASK_R(6+screen_offset_sg_x+26*C,9+screen_offset_sg_y+32*D,16,Sprite,Sprite+16,Sprite+32,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
              DrawSprite16SMASKR(6 + screen_offset_sg_x + 26 * C,
                                 9 + screen_offset_sg_y + 32 * D, 16, Sprite,
                                 Sprite + 16, Sprite + 32);
            } else {

              char String[5] = "";

              Sprite = (unsigned short *)Fg_plane.p.sprites;

              switch (CardGame[C + 6 * D]) {

              case 3: // flower
                Sprite +=
                    63 *
                    32; // Sprite = (unsigned short*)Fg_plane.p.sprites + 63*32;
                break;
              case 4: // Up
                Sprite += 180 * 32;
                ; // Sprite = (unsigned short*)Fg_plane.p.sprites + 180*32;
                StringCopy(String, "1 UP");
                break;
              case 5: // 10 Coins
                Sprite +=
                    4 *
                    32; // Sprite = (unsigned short*)Fg_plane.p.sprites + 4*32;
                StringCopy(String, "10");
                break;
              case 6: // 20 Coins
                Sprite +=
                    4 *
                    32; // Sprite = (unsigned short*)Fg_plane.p.sprites + 4*32;
                StringCopy(String, "20");
                break;

              default:
              };

              // DrawGrayStrExt2B(8+screen_offset_sg_x+26*C,9+screen_offset_sg_y+15+32*D,String,A_NORMAL,F_4x6,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD);
              DrawString(8 + screen_offset_sg_x + 26 * C,
                         9 + screen_offset_sg_y + 15 + 32 * D, String, A_NORMAL,
                         F_4x6);
              // A_REPLACE|A_SHADOWED

              GrayClipISprite16_RPLC_R(
                  6 + screen_offset_sg_x + 26 * C,
                  7 + screen_offset_sg_y + 32 * D, 16, Sprite,
                  /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                  /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
            };
          };
        };
      };
    };

    GrayDBufToggleSync_SetPointers(); // GrayDBufToggleSync();//set drawbuffer
                                      // as active screen

    if (Wait) {

      if ((--Wait) == 0) {
        if (Turn) { // turn cards back after mismatch and wiev
          CardGame[CX + 6 * CY] = -CardGame[CX + 6 * CY];
          CardGame[PX + 6 * PY] = -CardGame[PX + 6 * PY];
          Turn = 0;
          Miss++;
          PX = PY = -1;
        } else {

          for (C = 0; C < 18; C++) { // scan through all cards...
            if (CardGame[C] < 0)
              break; // until an unmatched card is found
          };
          if (C == 18) { // if all cards were checked, i.e. no unmatched cards
                         // on the table...
            Miss = 2;    // exit
            //		New_card_game();
          }
        };
      };
      for (C = 0; C < 10; C++) { // delay
        for (D = 0; D < 30000; D++)
          ;
      };

    } else {
      /*while(_rowread(0));//wait until key released
      while(!_rowread(0));//wait until key pressed*/
      WaitKeyPress();
    };
    if (Flash) {
      if ((--Flash) == 0) {
        PX = PY = -1;
      };
    };

    Scankeys();

    if (!Wait) {

      if (Keystate.Left) {
        if (CX > 0) {
          CX--;
        };
      } else {
        if (Keystate.Right) {
          if (CX < 5) {
            CX++;
          };
        };
      };

      if (Keystate.Up) {
        if (CY > 0) {
          CY--;
        };
      } else {
        if (Keystate.Down) {
          if (CY < 2) {
            CY++;
          };
        };
      };

      if (Keystate.Jump) {
        if (CardGame[CX + 6 * CY] < 0) { // unturned
          CardGame[CX + 6 * CY] = -CardGame[CX + 6 * CY];

          if (PX >= 0) { // already turned a card (another card is open)

            if (CardGame[CX + 6 * CY] == CardGame[PX + 6 * PY]) { // MATCH

              char Coins = SavePlayer.Coins;

              switch (CardGame[CX + 6 * CY]) {
              case 1: // mushrom
                Itemlist_add(1);
                break;
              case 2: // star
                Itemlist_add(4);
                break;
              case 3: // flower
                Itemlist_add(2);
                break;
              case 4: // 1 up
                SavePlayer.Lives += 1;

                break;
              case 6: // 20 coins
                /*SavePlayer.*/ Coins += 10;
              case 5: // 10 coins
                SavePlayer.Coins += 10;
                if (/*SavePlayer.*/ Coins >= 100) {
                  /*SavePlayer.*/ Coins -= 100;
                  SavePlayer.Lives += 1;
                }

                break;
              }

              SavePlayer.Coins = Coins;
              Wait = 8;
              Flash = 8;
            } else { // mismatch
              // wait,turn both cards
              Turn = 1;
              Wait = 4;
            }

          } else {
            PX = CX;
            PY = CY;
          };
        };
      };
    };
  };
}
