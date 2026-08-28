

#include "all.h"

short BgX, BgY, FgX, FgY; // screen render points (upper left corner of visual
                          // screen,coordinates in level...)

short Update_FG;

Plane Bg_plane;
AnimatedPlane Fg_plane, Fg_mask, Map_plane;
unsigned char StatBarHeight;
// statusbardata Statusbardata;

void *dBufHPL_G, *dBufHPD_G;

void GrayDBufToggleSync_SetPointers() {

  GrayDBufToggleSync(); // set drawbuffer as active screen

  dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE);
  dBufHPD_G = GrayDBufGetHiddenPlane(DARK_PLANE);
}

void FilledRectEraseDark(short X1, short Y1, short X2, short Y2) {
  FastFilledRect_Erase_R(dBufHPD_G, X1, Y1, X2, Y2);
};

void FilledRectEraseLight(short X1, short Y1, short X2, short Y2) {
  FastFilledRect_Erase_R(dBufHPL_G, X1, Y1, X2, Y2);
};

void FilledRectDark(short X1, short Y1, short X2, short Y2) {
  FastFilledRect_Draw_R(dBufHPD_G, X1, Y1, X2, Y2);
}

void GrayOutlineRect(short X1, short Y1, short X2, short Y2, short Color) {
  GrayFastOutlineRect_R(dBufHPL_G, dBufHPD_G, X1, Y1, X2, Y2, Color);
};

void DrawString(short X, short Y, const char *Str, short Attr, short Font) {
  DrawGrayStrExt2B(X, Y, Str, Attr, Font, dBufHPL_G, dBufHPD_G);
}

void DrawSprite16SMASKR(short X, short Y, short H, const unsigned short *sprt0,
                        const unsigned short *sprt1,
                        const unsigned short *mask) {
  GrayClipSprite16_SMASK_R(X, Y, H, sprt0, sprt1, mask, dBufHPL_G, dBufHPD_G);
};

void DrawSprite(register short X, register short Y, register short H,
                const unsigned short *sprt0, short H2) {

  //	GrayClipSprite16_SMASK_R(X-FgX,Y-FgY,H,sprt0,sprt0+H2,sprt0+2*H2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite16_SMASK_R(X - FgX, Y - FgY, H, sprt0, sprt0 + H2,
                           sprt0 + H2 + H2,
                           /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                           /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
}

/*void DrawSprite_HP(register short x,register short y,register short h,
const unsigned short *sprt0,const unsigned short *sprt1,const unsigned short
*mask);*/

void DrawMarioCursor(short X, short Y) {
  // GrayClipSprite16_SMASK_R(X,Y,16,Mariosprites+2*Marioanimtab[1][0],Mariosprites+2*Marioanimtab[1][0]+16,Mariomasks+Marioanimtab[1][0],dBufHPL_G,dBufHPD_G);
  DrawSprite16SMASKR(X, Y, 16, Mariosprites + 2 * Marioanimtab[1][0],
                     Mariosprites + 2 * Marioanimtab[1][0] + 16,
                     Mariomasks + Marioanimtab[1][0]);
};

void DrawSpriteUpsideDown(register short X, register short Y, register short H,
                          unsigned short *sprt0, short H2) {

  //	UpsideDownGrayClipSprite16_MASK_R(X-FgX,Y-FgY,H,sprt0,sprt0+H2,sprt0+2*H2,sprt0+2*H2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  UpsideDownGrayClipSprite16_MASK_R(
      X - FgX, Y - FgY, H, sprt0, sprt0 + H2, sprt0 + H2 + H2, sprt0 + H2 + H2,
      /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
      /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
}

// UpsideDownGrayClipSprite16_MASK_R(register short x,register short y,register
// short h,unsigned short *sprt0,unsigned short *sprt1,unsigned short
// *mask0,unsigned short *mask1,register void *dest0,register void *dest1)
// __attribute__((__stkparm__));
/*
void DrawSprite( short X , short  Y , short H , const unsigned short *sprt0 ,
short H2){

        GrayClipSprite16_SMASK_R(X-FgX,Y-FgY,H,sprt0,sprt0+H2,sprt0+2*H2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));

}*/

/*
(register short x,register short y,register short h,const unsigned short
*sprt0,const unsigned short *sprt1,const unsigned short *mask,register void
*dest0,register void *dest1) __attribute__((__stkparm__));
*/

void DrawBg(short X, short Y) {
  DrawGrayPlane(X, Y, &Bg_plane, dBufHPL_G, dBufHPD_G, drawmode_bg,
                TM_G16B_ROLL);
};

void Render() // this function renders all graphics
{
  /*
  //Apply frame skipping on TI 92+ and V200 to gain speed. Now moved to gameloop
          #ifdef PRODUCE_TI92PLUS_CODE

          static short FrameSkip = 0;
          if( (++FrameSkip)==4){//skipping each n'th frame (don't draw it, still
  calculated) FrameSkip = 0;//gives a good speed increase return;
          }

          #endif

          #ifdef PRODUCE_V200_CODE

          static short FrameSkip = 0;
          if( (++FrameSkip)==4){//skipping each n'th frame (don't draw it, still
  calculated) FrameSkip = 0;//gives a good speed increase return;
          }

          #endif
  */
  short C, D;
  /*//Those should be valid from previous call to Render(), Render_map() or
  initialization dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE); dBufHPD_G =
  GrayDBufGetHiddenPlane(DARK_PLANE);
  */
  // Draw background and animated foreground

  // DrawGrayPlane(BgX,BgY,&Bg_plane,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,drawmode_bg/*TM_GRPLC89*/,TM_G16B_ROLL/*TM_G16B*/);
  DrawBg(BgX, BgY);

  if (Player.Behind_fg /*SavePlayer.Attribs & 0b00010000*/) {

    Draw_player();
  };

  if (Update_FG) {
    Fg_plane.p.force_update = 1;
    Fg_mask.p.force_update = 1;
  }
  Update_FG = 0;

  DrawAnimatedPlane(FgX, FgY, &Fg_mask,
                    /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                    drawmode_fg_m /*TM_MASK89*/, TM_A16B);
  // Fg_mask.frame--;//MUST do this !!! important! or else the fg_mask will
  // animate twice as fast as the fg_plane
  DrawAnimatedPlane(FgX, FgY, &Fg_mask,
                    /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G,
                    drawmode_fg_m /*TM_MASK89*/, TM_A16B);
  DrawGrayAnimatedPlane(FgX, FgY, &Fg_plane,
                        /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                        /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G,
                        drawmode_fg /*TM_GOR89*/, TM_GA16B);

  if (!Player.Behind_fg /*!(SavePlayer.Attribs & 0b00010000)*/) {

    Draw_player();
  };

  short Curr_enemy;
  // Draw_enemies(); //put inline for minor speed increase
  for (Curr_enemy = 0; Curr_enemy < Leveldata.Nr_of_enemies; Curr_enemy++) {

    if (Enemies[Curr_enemy].Active &&
        (Enemies[Curr_enemy].Height >
         0)) { // check if current enemy is active,i.e. in the "active" area
               // Fg.X+{-15,255}
      if (Enemies[Curr_enemy].Life > 0) {
        // GrayClipSprite16_MASK_R(Enemies[Curr_enemy].X-FgX,Enemies[Curr_enemy].Y-FgY,Enemies[Curr_enemy].Height,Enemies[Curr_enemy].Sprite,Enemies[Curr_enemy].Sprite+Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Mask,Enemies[Curr_enemy].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
        // //USING CLIPPED SPRITE TO AVOID DRAVING OUTSIDE GFX MEM
        // GrayClipSprite16_MASK_R(Enemies[Curr_enemy].X-FgX,Enemies[Curr_enemy].Y-FgY,Enemies[Curr_enemy].Height,Enemies[Curr_enemy].Sprite,Enemies[Curr_enemy].Sprite+Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Sprite+2*Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Sprite+2*Enemies[Curr_enemy].Height2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
        // //USING CLIPPED SPRITE TO AVOID DRAVING OUTSIDE GFX MEM
        // GrayClipSprite16_SMASK_R(Enemies[Curr_enemy].X-FgX,Enemies[Curr_enemy].Y-FgY,Enemies[Curr_enemy].Height,Enemies[Curr_enemy].Sprite,Enemies[Curr_enemy].Sprite+Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Sprite+2*Enemies[Curr_enemy].Height2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
        // //USING CLIPPED SPRITE TO AVOID DRAVING OUTSIDE GFX MEM
        DrawSprite(Enemies[Curr_enemy].X, Enemies[Curr_enemy].Y,
                   Enemies[Curr_enemy].Height, Enemies[Curr_enemy].Sprite,
                   Enemies[Curr_enemy].Height2);
      } else {

        if (Enemies[Curr_enemy].Life == -1)
          // UpsideDownGrayClipSprite16_MASK_R(Enemies[Curr_enemy].X-FgX,Enemies[Curr_enemy].Y-FgY,Enemies[Curr_enemy].Height,Enemies[Curr_enemy].Sprite,Enemies[Curr_enemy].Sprite+Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Mask,Enemies[Curr_enemy].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
          // UpsideDownGrayClipSprite16_MASK_R(Enemies[Curr_enemy].X-FgX,Enemies[Curr_enemy].Y-FgY,Enemies[Curr_enemy].Height,Enemies[Curr_enemy].Sprite,Enemies[Curr_enemy].Sprite+Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Sprite+2*Enemies[Curr_enemy].Height2,Enemies[Curr_enemy].Sprite+2*Enemies[Curr_enemy].Height2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
          DrawSpriteUpsideDown(Enemies[Curr_enemy].X, Enemies[Curr_enemy].Y,
                               Enemies[Curr_enemy].Height,
                               Enemies[Curr_enemy].Sprite,
                               Enemies[Curr_enemy].Height2);
      };
    };
  };

  // Draw_shells(); //put inline for minor speed increase
  for (C = 0; C < max_nr_of_shells; C++) { //
    if (Shells[C].Active) {                // if valid shell
      // GrayClipSprite16_MASK_R(Shells[C].X-FgX,Shells[C].Y-FgY,16,(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite),(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite)+16,(Shells[C].Active==2?beetle_shell_mask:shell_0_mask),(Shells[C].Active==2?beetle_shell_mask:shell_0_mask),GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_MASK_R(Shells[C].X-FgX,Shells[C].Y-FgY,16,(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite),(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite)+16,(Shells[C].Active==2?(beetle_shell_sprite+16*2):(shell_0_sprite+16*2)),(Shells[C].Active==2?(beetle_shell_sprite+16*2):(shell_0_sprite+16*2)),GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Shells[C].X-FgX,Shells[C].Y-FgY,16,(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite),(Shells[C].Active==2?beetle_shell_sprite:shell_0_sprite)+16,(Shells[C].Active==2?(beetle_shell_sprite+16*2):(shell_0_sprite+16*2)),GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      DrawSprite(Shells[C].X, Shells[C].Y, 16,
                 (Shells[C].Active == 2 ? beetle_shell_sprite : shell_0_sprite),
                 16);
    };
  };

  // draw player fireballs
  for (C = 0; C < nr_of_player_smallshots; C++) {
    if (Player_smallshots[C].Mode)
      // GrayClipSprite16_MASK_R(Player_smallshots[C].X-FgX,Player_smallshots[C].Y-FgY,8,fireball_anim_16_spr+16*Fg_plane.step,fireball_anim_16_spr+8+16*Fg_plane.step,fireball_anim_16_msk+8*Fg_plane.step,fireball_anim_16_msk+8*Fg_plane.step,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_MASK_R(Player_smallshots[C].X-FgX,Player_smallshots[C].Y-FgY,8,fireball_anim_16_spr+24*Fg_plane.step,fireball_anim_16_spr+8+24*Fg_plane.step,fireball_anim_16_spr+2*8+24*Fg_plane.step,fireball_anim_16_spr+2*8+24*Fg_plane.step,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Player_smallshots[C].X-FgX,Player_smallshots[C].Y-FgY,8,fireball_anim_16_spr+24*Fg_plane.step,fireball_anim_16_spr+8+24*Fg_plane.step,fireball_anim_16_spr+2*8+24*Fg_plane.step,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      DrawSprite(Player_smallshots[C].X, Player_smallshots[C].Y, 8,
                 fireball_anim_16_spr + 24 * Fg_plane.step, 8);
    // GrayClipSprite16_MASK_R(Player_smallshots[C].X-FgX,Player_smallshots[C].Y-FgY,8,fireball_anim_16_spr+16*0,fireball_anim_16_spr+8+16*0,fireball_anim_16_spr+2*8+16*0,fireball_anim_16_spr+2*8+16*0,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  };

  for (C = 0; C < nr_of_enemyshots; C++) {
    if (Enemyshots[C].Mode)
      // GrayClipSprite16_MASK_R(Enemyshots[C].X-FgX,Enemyshots[C].Y-FgY,Enemyshots[C].Height,Enemyshots[C].Sprite,Enemyshots[C].Sprite+Enemyshots[C].Height,Enemyshots[C].Mask,Enemyshots[C].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_MASK_R(Enemyshots[C].X-FgX,Enemyshots[C].Y-FgY,Enemyshots[C].Height,Enemyshots[C].Sprite,Enemyshots[C].Sprite+Enemyshots[C].Height,Enemyshots[C].Sprite+2*Enemyshots[C].Height,Enemyshots[C].Sprite+2*Enemyshots[C].Height,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Enemyshots[C].X-FgX,Enemyshots[C].Y-FgY,Enemyshots[C].Height,Enemyshots[C].Sprite,Enemyshots[C].Sprite+Enemyshots[C].Height,Enemyshots[C].Sprite+2*Enemyshots[C].Height,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      DrawSprite(Enemyshots[C].X, Enemyshots[C].Y, Enemyshots[C].Height,
                 Enemyshots[C].Sprite, Enemyshots[C].Height);
  };

  // Draw_objects();
  for (C = 0; C < nr_of_objects; C++) {
    if (Objects[C].Active)
      Objects[C].Draw(&Objects[C]);
  };

  // Draw_items();
  for (C = 0; C < nr_of_items; C++) {
    if (Items[C].Active) { // if existing

      // GrayClipSprite16_MASK_R(Items[C].X-FgX,Items[C].Y-FgY,Items[C].Height,Items[C].Sprite,Items[C].Sprite+Items[C].Height2,Items[C].Mask,Items[C].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Items[C].X-FgX,Items[C].Y-FgY,Items[C].Height,Items[C].Sprite,Items[C].Sprite+Items[C].Height2,Items[C].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      DrawSprite(Items[C].X, Items[C].Y, Items[C].Height, Items[C].Sprite,
                 Items[C].Height2);
    };
  };

  // draw flying platforms
  for (C = 0; C < Leveldata.Nr_of_flying_platforms; C++) {

    if (Flying_platforms[C].Active) {

      for (D = 0; D <= (Flying_platforms[C].Width - 1); D++) {
        short Temp = 0;

        if (D == 1) {
          Temp = 32;
        };

        if (D == (Flying_platforms[C].Width - 1)) {
          Temp = 64;
        };

        //			if(!Flying_platforms[C].DrawMode){
        if (Flying_platforms[C].Sprite == 60000) { // TEMP, THIS WILL CHANGE!!
          DrawSprite(Flying_platforms[C].X, Flying_platforms[C].Y, 16,
                     falling_block_spr, 16);
        } else {
          GrayClipISprite16_RPLC_R(
              Flying_platforms[C].X - FgX + (D) * 16,
              Flying_platforms[C].Y - FgY, 16,
              /*Flying_platforms[C].Sprite+Temp*/
              (unsigned short *)Fg_plane.p.sprites +
                  Flying_platforms[C].Sprite * 32 + Temp,
              /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
              /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
        }
      };
    };
  };

  if (Leveldata.Boss) {

    C = LIGHT_PLANE;
    D = DARK_PLANE;
    if (BossG->Mode) {
      if (Fg_plane.step % 2) {
        C = DARK_PLANE;
        D = LIGHT_PLANE;
      };
    };

    // GrayClipSprite16_MASK_R(Boss->X-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite,Boss->Sprite+Boss->Height2,Boss->Mask,Boss->Mask,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));
    // GrayClipSprite16_MASK_R(Boss->X+16-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite+2*Boss->Height2,Boss->Sprite+3*Boss->Height2,Boss->Mask+Boss->Height2,Boss->Mask+Boss->Height2,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));

    // GrayClipSprite16_MASK_R(Boss->X-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite,Boss->Sprite+Boss->Height2,Boss->Sprite+2*Boss->Height2,Boss->Sprite+2*Boss->Height2,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));
    // GrayClipSprite16_MASK_R(Boss->X+16-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite+3*Boss->Height2,Boss->Sprite+4*Boss->Height2,Boss->Sprite+5*Boss->Height2,Boss->Sprite+5*Boss->Height2,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));
    // GrayClipSprite16_SMASK_R(Boss->X-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite,Boss->Sprite+Boss->Height2,Boss->Sprite+2*Boss->Height2,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));
    // GrayClipSprite16_SMASK_R(Boss->X+16-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite+3*Boss->Height2,Boss->Sprite+4*Boss->Height2,Boss->Sprite+5*Boss->Height2,GrayDBufGetHiddenPlane(C),GrayDBufGetHiddenPlane(D));
    DrawSprite(BossG->X, BossG->Y, BossG->Height, BossG->Sprite,
               BossG->Height2);
    DrawSprite(BossG->X + 16, BossG->Y, BossG->Height,
               BossG->Sprite + 3 * BossG->Height2, BossG->Height2);
    // GrayClipSprite32_MASK_R(Boss->X-FgX,Boss->Y-FgY,Boss->Height,Boss->Sprite,Boss->Sprite+Boss->Height2,Boss->Mask,Boss->Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  };

  if (StatBarHeight) {
    Update_statusbar();
  }
  /*
  GrayDBufToggleSync(); 	//set drawbuffer as active screen

  dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE);
  dBufHPD_G = GrayDBufGetHiddenPlane(DARK_PLANE);
  */
  GrayDBufToggleSync_SetPointers();
}

void Adjust_renderpoint() { // adjusts the FG renderpoint
  if (((Player.X - FgX) < scroll_limit_left)) {
    FgX = FgX - (scroll_limit_left + FgX - (Player.X));

  } else {
    if((Player.X-FgX) > scroll_limit_right /*&&  (Leveldata.Border_right > (FgX+screen_width))*/ ){
      FgX = FgX + (Player.X - FgX - scroll_limit_right);
    };
  };

  if (Leveldata.Border_right < (FgX + screen_width))
    FgX = Leveldata.Border_right - screen_width;
  if (FgX < Leveldata.Border_left)
    FgX = Leveldata.Border_left;

  if (((Player.Y + Player.Height) > (FgY + scroll_limit_down)) &&
      ((FgY + screen_height) < (Leveldata.Height * 16))) {
    FgY = FgY + ((Player.Y + Player.Height) - (FgY + scroll_limit_down));
  } else {
    if (Player.Y < (FgY + scroll_limit_up)) {
      FgY = FgY - ((FgY + scroll_limit_up) - Player.Y);
      if (FgY < 0)
        FgY = 0;
    };
  };

  if (((FgY + screen_height) > (Leveldata.Height * 16))) {
    FgY = Leveldata.Height * 16 - screen_height;
  };

  BgX = FgX / Leveldata.Bg_scrollrate;

  /*if(BgX>(Leveldata.Bg_width*16-240))
          BgX = BgX%((Leveldata.Bg_width)*16);//-15*/
  /*if(BgX>(Bg_plane.width*16-240))
          BgX = BgX%((Bg_plane.width)*16);//-15*/

  BgY = max(FgY / Leveldata.Bg_scrollrate + Leveldata.Bg_offset, 0);

  if ((BgY + screen_height) > (Leveldata.Bg_height * 16))
    BgY = Leveldata.Bg_height * 16 -
          screen_height; // prevents that garbage is drawn on the bottom of the
                         // BG plane
}

void Draw_brick_fragments(object *Object) {
  /*
  GrayClipSprite8_MASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_MASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_MASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_MASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  */
  /*
  GrayClipSprite8_SMASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_SMASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_SMASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipSprite8_SMASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  */
  if ((++Object->Active) == 36) // Xpos
    Object->Active = 0;

  if (Object->Active >= 10) {
    Object->Data0--; // low y
  } else {
    Object->Data0++; // low y
  };
  if (Object->Active < 15) {
    Object->Data1 += 1; // high y
  } else {
    Object->Data1--; // high y
  };

  unsigned char *Sprite = brick_fragment_sprite;
  /*
  GrayClipSprite8_SMASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_sprite+16,dBufHPL_G,dBufHPD_G);
  GrayClipSprite8_SMASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data0)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_sprite+16,dBufHPL_G,dBufHPD_G);
  GrayClipSprite8_SMASK_R((Object->X-Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_sprite+16,dBufHPL_G,dBufHPD_G);
  GrayClipSprite8_SMASK_R((Object->X+Object->Active)-FgX,(Object->Y-8*Object->Data1)-FgY,8,brick_fragment_sprite,brick_fragment_sprite+8,brick_fragment_sprite+16,dBufHPL_G,dBufHPD_G);
  */

  short X1 = (Object->X - Object->Active) - FgX;
  short X2 = (Object->X + Object->Active) - FgX;
  short Y1 = (Object->Y - 8 * Object->Data0) - FgY;
  short Y2 = (Object->Y - 8 * Object->Data1) - FgY;

  GrayClipSprite8_SMASK_R(/*(Object->X-Object->Active)-FgX*/ X1,
                          /*(Object->Y-8*Object->Data0)-FgY*/ Y1, 8, Sprite,
                          Sprite + 8, Sprite + 16, dBufHPL_G, dBufHPD_G);
  GrayClipSprite8_SMASK_R(/*(Object->X+Object->Active)-FgX*/ X2,
                          /*(Object->Y-8*Object->Data0)-FgY*/ Y1, 8, Sprite,
                          Sprite + 8, Sprite + 16, dBufHPL_G, dBufHPD_G);
  GrayClipSprite8_SMASK_R(/*(Object->X-Object->Active)-FgX*/ X1,
                          /*(Object->Y-8*Object->Data1)-FgY*/ Y2, 8, Sprite,
                          Sprite + 8, Sprite + 16, dBufHPL_G, dBufHPD_G);
  GrayClipSprite8_SMASK_R(/*(Object->X+Object->Active)-FgX*/ X2,
                          /*(Object->Y-8*Object->Data1)-FgY*/ Y2, 8, Sprite,
                          Sprite + 8, Sprite + 16, dBufHPL_G, dBufHPD_G);
};

void Draw_killed_fireballs(object *Object) {

  Object->Active++;

  short Data0 = Object->Data0;

  if (Object->Active == 4) {
    /*Object->*/ Data0 = 16;
  } else {
    if (Object->Active == 7) {
      /*Object->*/ Data0 = 32;
    } else {
      if (Object->Active == 10)
        Object->Active = 0;
    };
  };

  Object->Data0 = Data0;

  // GrayClipSprite16_MASK_R(Object->X-FgX,Object->Y-FgY,16,blank_sprite,blank_sprite,fireball_hit_anim
  // + Object->Data0,fireball_hit_anim +
  // Object->Data0,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_SMASK_R(Object->X-FgX,Object->Y-FgY,16,blank_sprite,blank_sprite,fireball_hit_anim
  // +
  // /*Object->*/Data0,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
  DrawSprite16SMASKR(Object->X - FgX, Object->Y - FgY, 16, blank_sprite,
                     blank_sprite, fireball_hit_anim + Data0);
};

void Draw_elastic_tiles(
    object *Object) { // data2: b7:up b6:down b5:left b4:right

  if ((--Object->Active) == 0) {
    //*(
    //(char*)(Fg_plane.p.matrix+(Object->Y/16)*Fg_plane.p.width+(Object->X/16))
    //) = Object->Data0;
    if ((Get_tile(Object->X, Object->Y) != coin)) {
      Put_tile(Object->X, Object->Y, Object->Data0);
      Update_FG = 1; // Fg_plane.p.force_update=1;Fg_mask.p.force_update=1;
    };
  };

  short Xoffset = 0, Yoffset = 0;

  if ((Object->Data2 & 0b10000000)) {
    Yoffset = -8;
  } else {
    if ((Object->Data2 & 0b01000000))
      Yoffset = 8;
  };
  if ((Object->Data2 & 0b00100000)) {
    Xoffset = -8;
  } else {
    if ((Object->Data2 & 0b00010000))
      Xoffset = 8;
  };

  // GrayClipSprite16_MASK_R(Object->X-FgX+Xoffset,Object->Y-FgY+Yoffset,16,Sprites+Object->Data1*32,Sprites+Object->Data1*32+16,bbox_note_mask,bbox_note_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  GrayClipISprite16_RPLC_R(
      Object->X - FgX + Xoffset, Object->Y - FgY + Yoffset, 16,
      (unsigned short *)Fg_plane.p.sprites + Object->Data1 * 32,
      /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
      /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
};

void Draw_bb_coin(object *Object) {
  // GrayClipSprite16_MASK_R(Object->X-FgX,Object->Y-FgY-8,Object->Data1,bb_coin_anim_sprite+(2*Object->Data0),bb_coin_anim_sprite+(2*Object->Data0)+Object->Data1,bb_coin_anim_mask+Object->Data0,bb_coin_anim_mask+Object->Data0,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_SMASK_R(Object->X-FgX,Object->Y-FgY-8,Object->Data1,bb_coin_anim_sprite+(2*Object->Data0),bb_coin_anim_sprite+(2*Object->Data0)+Object->Data1,bb_coin_anim_sprite+(2*Object->Data0)+2*Object->Data1,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  Object->Active--;

  short Y = Object->Y;

  if (Object->Active > 15) {
    /*Object->*/ Y -= 5;
  }
  if ((Object->Active <= 15) && Object->Active > 6) {
    /*Object->*/ Y += 5;
  };

  Object->Y = Y;

  short Data0 = Object->Data0;

  if (Object->Active <= 6) {
    if (Object->Active > 3) {
      /*Object->*/ Data0 = 24 * 4; // 16*4
    } else {
      /*Object->*/ Data0 = 24 * 5; // 16*5
      Object->Data1 = 20;
    };
  } else {
    /*Object->*/ Data0 = 24 * Fg_plane.step; // 16
  };
  Object->Data0 = Data0;
  DrawSprite(Object->X, Object->Y - 8, Object->Data1,
             bb_coin_anim_sprite + (2 * Object->Data0), Object->Data1);

  // bb_coin_anim_mask+Object->Data0
};

void Draw_score(object *Object) {
  // GrayClipSprite16_MASK_R(Object->X-FgX,Object->Y-FgY,8,score_sprites+(2*Object->Data0),score_sprites+(2*Object->Data0)+8,score_masks+Object->Data0*8,score_masks+Object->Data0*8,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_SMASK_R(Object->X-FgX,Object->Y-FgY,8,score_sprites+(Object->Data0),score_sprites+(Object->Data0)+8,score_sprites+(Object->Data0)+16,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));

  Object->Active--;
  Object->Y--;

  DrawSprite(Object->X, Object->Y, 8, score_sprites + (Object->Data0), 8);
  // score_masks+Object->Data0*8
};

void Draw_collided_shells(object *Object) {
  // Do you see a bug? Hint: Active!
  Object->Active++;

  if (Object->Active < 3) {
    Object->Y -= 4;
  } else {
    if (Object->Active > 6)
      Object->Y += 4;
  };

  Object->X += Object->Data0; // direction

  if (Object->Active > 28) // with speed 4 in y dir this nr ensures that none is
                           // removed before they have left the screen
    Object->Active = 0;    // Enemy_remove(Enemy);

  // UpsideDownGrayClipSprite16_MASK_R(Object->X-FgX,Object->Y-FgY,16,shell_0_sprite,shell_0_sprite+16,shell_0_mask,shell_0_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // UpsideDownGrayClipSprite16_MASK_R(Object->X-FgX,Object->Y-FgY,16,shell_0_sprite,shell_0_sprite+16,shell_0_sprite+16*2,shell_0_sprite+16*2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // DrawSpriteUpsideDown(Object->X, Object->Y, 16, shell_0_sprite, 16);
  DrawSpriteUpsideDown(
      Object->X, Object->Y, 16,
      (Object->Active == 2 ? beetle_shell_sprite : shell_0_sprite), 16);
};

void Draw_climbing_flower(object *Object) {

  if (Object->Data0 < 16) { // coming out of box
    Object->Data0 += 2;
    Object->Y -= 2;
  } else {

    if (Free_up(Object->X, Object->Y)) {
      Object->Y -= 2; // grow

      Put_tile(Object->X, Object->Y + Object->Data0 - 1, ladder);
      if (!(Object->Y % 16)) {
        // if(!(Object->Y&0x000f)){
        Update_FG = 1; // Fg_plane.p.force_update=1;Fg_mask.p.force_update=1;
      };
    } else {
      Object->Active = 0; // suicide
    };
  }

  //	short Animoffset = (Fg_plane.step%2?0:32);

  DrawSprite(Object->X, Object->Y, Object->Data0,
             flower1_sprite + 3 * (Fg_plane.step % 2 ? 0 : 32) /*Animoffset*/,
             32);
};

void Draw_killed_cannonballs(object *Object) {

  if ((Object->Y += 3) >= FgY + screen_height) {
    Object->Active = 0;
  };
  // GrayClipSprite16_SMASK_R(Object->X-FgX,Object->Y-FgY,Object->Data1,cannonball_horiz_left_spr+Object->Data0,cannonball_horiz_left_spr+Object->Data0+Object->Data1,cannonball_horiz_left_spr+Object->Data0+2*Object->Data1,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  DrawSprite(Object->X, Object->Y, Object->Data1,
             cannonball_horiz_left_spr + Object->Data0, Object->Data1);
};

void Draw_splash(object *Object) {

  short Data0 = Object->Data0;
  short Data1 = Object->Data1;
  short Y = Object->Y;

  switch (Object->Active) {
  case 20:
    /*Object->*/ Data0 = 14;
    /*Object->*/ Data1 += 7 * 2;
    /*Object->*/ Y -= 7;
    break;
  case 15:
    /*Object->*/ Data0 = 9;
    /*Object->*/ Data1 += 14 * 2;
    /*Object->*/ Y += 2;
    break;
  case 10:
    /*Object->*/ Data0 = 4;
    /*Object->*/ Data1 += 9 * 2;

    break;
  case 5:
    /*Object->*/ Data0 = 9;
    /*Object->*/ Data1 += 4 * 2;
    /*Object->*/ Y += 3;
    break;
  }

  Object->Data0 = Data0;
  Object->Data1 = Data1;
  Object->Y = Y;

  // GrayClipSprite16_SMASK_R(Object->X-FgX,Object->Y-FgY,Object->Data0,blank_sprite,splash_gfx+Object->Data1,splash_gfx+Object->Data1+Object->Data0,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
  DrawSprite16SMASKR(Object->X - FgX, Object->Y - FgY, Object->Data0,
                     blank_sprite, splash_gfx + Object->Data1,
                     splash_gfx + Object->Data1 + Object->Data0);
  // DrawSprite(Object->X, Object->Y,Object->Data0,splash_gfx+Object->Data1,0);

  Object->Active--;
}

void Update_statusbar() { // one dummy call neccesarry with all the
                          // Player.Involved_variables set to 0 in init
  char String[8];
  String[2] = ' ';
  String[3] = 0;
  static char Oldcoins = 101;
  static short Oldlives = 0;
  static unsigned int OldScore = UINT_MAX;
  short C;

  if (Oldlives != SavePlayer.Lives) { // Statusbardata.Update_lives
    Oldlives = SavePlayer.Lives;
    String[1] = '0' + SavePlayer.Lives % 10;
    if (SavePlayer.Lives >= 10) {
      String[0] = '0' + SavePlayer.Lives / 10;
    } else {
      String[0] = ' ';
    };
    DrawGrayStrExt2B(28, 1, String, A_REPLACE, F_4x6, Statusbar,
                     Statusbar + 240);
  };

  if (Oldcoins != SavePlayer.Coins) {
    Oldcoins = SavePlayer.Coins;
    String[1] = '0' + SavePlayer.Coins % 10;
    if (SavePlayer.Coins >= 10) {
      String[0] = '0' + SavePlayer.Coins / 10;
    } else {
      String[0] = ' ';
    };
    DrawGrayStrExt2B(62, 1, String, A_REPLACE, F_4x6, Statusbar,
                     Statusbar + 240);
  };

#ifdef speedtest
  if (Update_frame_counter) {

    sprintf(String, " %d",
            Update_frame_counter); // Update_frame_counter);Fps);//
    DrawGrayStrExt2B(145, 1, "        ", A_REPLACE, F_4x6, Statusbar,
                     Statusbar + 240);
    DrawGrayStrExt2B(145, 1, String, A_REPLACE, F_4x6, Statusbar,
                     Statusbar + 240);

    // SavePlayer.Coins = Fps;
    Fps = 0;

    Update_frame_counter = 0;
  }
#endif

#ifndef speedtest
  if (OldScore != SavePlayer.Score) {
    OldScore = SavePlayer.Score;
    short S = SavePlayer.Score;
    short D = 0;
    short F = (10000);

    while (F >= 1) {

      char n = S / F;
      S = S - (n * F);

      if (n) {
        String[D] = '0' + n;

      } else {
        String[D] = '0';
      }
      D++;
      F /= 10; // F = F*0.1;//
    }

    String[4 + 1] = String[4 + 2] = '0';
    String[4 + 3] = 0;

    DrawGrayStrExt2B(screen_width - (32), 1, String, A_REPLACE, F_4x6,
                     Statusbar, Statusbar + 240);
  }
#endif

  // draw some triangles that get filled when mario runs and a P flashing when
  // he is able to fly
  for (C = 0; C < 4; C++) {
    // GrayClipSprite8_MASK_R(78+8*C,0,8,(Player.Runcount>=((flycondition/4)*(C+1))?triangle_f_spr:triangle_e_spr),(Player.Runcount>=((flycondition/4)*(C+1))?triangle_f_spr:triangle_e_spr)+8,triangle_msk,triangle_msk,Statusbar,Statusbar+240);
    // GrayClipSprite8_SMASK_R(78+8*C,0,8,(Player.Runcount>=((flycondition/4)*(C+1))?triangle_f_spr:triangle_e_spr),(Player.Runcount>=((flycondition/4)*(C+1))?triangle_f_spr:triangle_e_spr)+8,triangle_msk,Statusbar,Statusbar+240);
    GrayClipSprite8_SMASK_R(
        78 + 8 * C, 0, 8,
        (Player.Runcount >= ((flycondition / 4) * (C + 1)) ? triangle_f_spr
                                                           : triangle_e_spr),
        (Player.Runcount >= ((flycondition / 4) * (C + 1)) ? triangle_f_spr
                                                           : triangle_e_spr) +
            8,
        triangle_msk, Statusbar, Statusbar + 240);
  };

  // GrayClipSprite16_MASK_R(110,0,8,( ((Player.Runcount>=flycondition) &&
  // (Fg_plane.step%2))?p_dark_spr:p_bright_spr),(((Player.Runcount>=flycondition)
  // &&
  // (Fg_plane.step%2))?p_dark_spr:p_bright_spr)+8,p_msk,p_msk,Statusbar,Statusbar+240);
  GrayClipSprite16_SMASK_R(
      110, 0, 8,
      (((Player.Runcount >= flycondition) && (Fg_plane.step % 2))
           ? p_dark_spr
           : p_bright_spr),
      (((Player.Runcount >= flycondition) && (Fg_plane.step % 2))
           ? p_dark_spr
           : p_bright_spr) +
          8,
      p_msk, Statusbar, Statusbar + 240);

  // debug stuff
  //	sprintf (String, " %d",Enemies[12].Mode/*Player.Test*/);//Triggers[0].X
  // Player.Test  Enemies[2].Mode  Leveldata.Bg_offset Nr_of_enemies TCSize
  // Enemies[1].Y 	sprintf (String, "
  //%d",((enemy*)Enemies[16].Die==(enemy*)Enemy_die_11?99:11)/*Player.Test*/);//Triggers[0].X
  // Player.Test  Enemies[2].Mode  Leveldata.Bg_offset Nr_of_enemies TCSize
  // Enemies[1].Y

  //	DrawGrayStrExt2B(145,1,String,A_REPLACE,F_4x6,Statusbar,Statusbar+240);

  memcpy(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G + statusbar_offset,
         Statusbar, 240); // 2760
  memcpy(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G + statusbar_offset,
         Statusbar + 240, 240);
};

void Draw_statusbar() {
  short C;

  memset(Statusbar, 0, 480);
  /*for(C=0;C<480;C++)
   *(((char*)Statusbar)+C) = 0;//size optimizaton      NOT
   */

  DrawGrayStrExt2B(1, 1, "MARIO x ", A_REPLACE, F_4x6, Statusbar,
                   Statusbar + 240);

  DrawGrayStrExt2B(40, 1, "COINS: ", A_REPLACE, F_4x6, Statusbar,
                   Statusbar + 240);

  Update_statusbar();
};

void Draw_player() {
  short C;

  short Dest1, Dest2;

  // the following stuff is animating mario star-mario and wounded mario
  // (flashing) tricking with the grayscale
  if (Player.Immortal && (Fg_plane.step % 2)) {

    if (SavePlayer.Attribs &
        0b10000000) { // star, invert grays for short periods by interchanging
                      // light and dark data
      Dest1 = DARK_PLANE;
      Dest2 = LIGHT_PLANE;
    } else { // wounded, draw mario brighter and partially transparent for short
             // periods by writing only to lightplane
      Dest1 = LIGHT_PLANE;
      Dest2 = LIGHT_PLANE;
    };

  } else {               // normal
    Dest1 = LIGHT_PLANE; // DARK_PLANE;
    Dest2 = DARK_PLANE;  // LIGHT_PLANE;
    /*Dest1=DARK_PLANE;
    Dest2=DARK_PLANE;*/
    /*Dest1=DARK_PLANE;
    Dest2=LIGHT_PLANE;*/
  };

  // Dest1=DARK_PLANE;//LIGHT_PLANE;//
  // Dest2=DARK_PLANE;//LIGHT_PLANE;//

  if ((SavePlayer.Attribs & 0b00100000) &&
      (SavePlayer.Spritenr != 9)) { // draw racoon suit tale
    C = 9 *
        Marioanimtab
            [4 + SavePlayer.Spritebase]
            [SavePlayer
                 .Spritenr]; // 9*Marioanimtab+(4+Player.Spritebase)*8+Player.Spritenr;

    if (Player.RacoonTaleAnim) {
      C = 9 *
          Marioanimtab[4 + SavePlayer.Spritebase][(Fg_plane.step % 2 ? 0 : 2)];
      Player.RacoonTaleAnim = 0;
    }

    //		if(Player.Jumpspeed==flyspeed)

    // GrayClipSprite16_MASK_R(Player.X-FgX+(Player.Face==1?-14:14),Player.Y-FgY+(Player.Spritenr==10?13:17),9,racoon_tale_spr+2*C,racoon_tale_spr+9+2*C,racoon_tale_msk+C,racoon_tale_msk+C,GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));

    // GrayClipSprite16_MASK_R(Player.X-FgX-(Player.Face*14),Player.Y-FgY+(Player.Spritenr==10?13:17),9,racoon_tale_spr+2*C,racoon_tale_spr+9+2*C,racoon_tale_msk+C,racoon_tale_msk+C,GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));
    if (SavePlayer.Spritenr != 9) {
      GrayClipSprite16_SMASK_R(
          Player.X - FgX - (Player.Face * 14),
          Player.Y - FgY + (SavePlayer.Spritenr == 10 ? 13 : 17), 9,
          racoon_tale_spr + 2 * C, racoon_tale_spr + 9 + 2 * C,
          racoon_tale_msk + C, GrayDBufGetHiddenPlane(Dest1),
          GrayDBufGetHiddenPlane(Dest2));
    };

    //	GrayClipSprite16_MASK_R(Player.X-FgX+(Player.Face==1?-14:14),Player.Y-FgY+17,9,racoon_tale_spr+18*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_tale_spr+9+18*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_tale_msk+9*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_tale_msk+9*Marioanimtab[4+Player.Spritebase][Player.Spritenr],GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  };

  // GrayClipSprite16_MASK_R(Player.X-FgX,Player.Y-FgY,Player.Height,Mariosprites+2*Marioanimtab[Player.Spritebase][Player.Spritenr],Mariosprites+2*Marioanimtab[Player.Spritebase][Player.Spritenr]+Player.Height,Mariomasks+Marioanimtab[Player.Maskbase][Player.Spritenr],Mariomasks+Marioanimtab[Player.Maskbase][Player.Spritenr],GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));
  // GrayClipSprite16_SMASK_R(Player.X-FgX,Player.Y-FgY,Player.Height,Mariosprites+2*Marioanimtab[Player.Spritebase][Player.Spritenr]+Player.SpriteOffset,Mariosprites+2*Marioanimtab[Player.Spritebase][Player.Spritenr]+Player.SpriteOffset+Player.Height2,Mariomasks+Marioanimtab[Player.Maskbase][Player.Spritenr]+Player.SpriteOffset,GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));

  GrayClipSprite16_SMASKBLIT_R(
      Player.X - FgX, Player.Y - FgY, Player.Height,
      Mariosprites +
          2 * Marioanimtab[SavePlayer.Spritebase][SavePlayer.Spritenr] +
          Player.SpriteOffset,
      Mariosprites +
          2 * Marioanimtab[SavePlayer.Spritebase][SavePlayer.Spritenr] +
          Player.SpriteOffset + Player.Height2,
      Mariomasks + Marioanimtab[SavePlayer.Maskbase][SavePlayer.Spritenr] +
          Player.SpriteOffset,
      Player.Blit, GrayDBufGetHiddenPlane(Dest1),
      GrayDBufGetHiddenPlane(Dest2));

  if (SavePlayer.Attribs & 0b00100000) { // draw racoon suit cap

    C = 5 *
        Marioanimtab
            [4 + SavePlayer.Spritebase]
            [SavePlayer.Spritenr]; //((4+Player.Spritebase)*8+Player.Spritenr);
    // GrayClipSprite16_MASK_R(Player.X-FgX,Player.Y-FgY+(Player.Spritenr==4?4:-1),5,racoon_cap_spr+2*C,racoon_cap_spr+5+2*C,racoon_cap_msk+C,racoon_cap_msk+C,GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));
    if (Player.SpriteOffset < 5) {
      // GrayClipSprite16_SMASK_R(Player.X-FgX,Player.Y-FgY+(Player.Spritenr==4?4:-1),5-Player.SpriteOffset,racoon_cap_spr+2*C+Player.SpriteOffset,racoon_cap_spr+5+2*C+Player.SpriteOffset,racoon_cap_msk+C+Player.SpriteOffset,GrayDBufGetHiddenPlane(Dest1),GrayDBufGetHiddenPlane(Dest2));
      GrayClipSprite16_SMASKBLIT_R(
          Player.X - FgX, Player.Y - FgY + (SavePlayer.Spritenr == 4 ? 4 : -1),
          5 - Player.SpriteOffset, racoon_cap_spr + 2 * C + Player.SpriteOffset,
          racoon_cap_spr + 5 + 2 * C + Player.SpriteOffset,
          racoon_cap_msk + C + Player.SpriteOffset, Player.Blit,
          GrayDBufGetHiddenPlane(Dest1), GrayDBufGetHiddenPlane(Dest2));
    };

    // GrayClipSprite16_MASK_R(Player.X-FgX,Player.Y-FgY-1,5,racoon_cap_spr+10*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_cap_spr+5+10*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_cap_msk+5*Marioanimtab[4+Player.Spritebase][Player.Spritenr],racoon_cap_msk+5*Marioanimtab[4+Player.Spritebase][Player.Spritenr],GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  };
};

void Render_map() {
  short C, Left, Right, Height;
  //	char String[4];
  unsigned short *Sprite, *Mask;

  /*	String[0]='x';
          String[2]=' ';
          String[3]=0;*/

  /*//Those should be valid from previous call to Render(), Render_map() or
  initialization dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE); dBufHPD_G =
  GrayDBufGetHiddenPlane(DARK_PLANE);
  */
  // draw map plane
  //	FgX=0;FgY=0;
  DrawGrayAnimatedPlane(FgX, FgY, &Map_plane,
                        /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                        /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G,
                        drawmode_bg /*TM_GRPLC89*/, TM_GA16B); //

  if (Levelsetdata.CurrentWorld ==
      Levelsetdata.Commonfile) { // Warp zone: Find pipe pos and draw numbers
    short C;
    for (C = 0; C < Map_data.Nr_of_triggers; C++) {

      short NewMap = Map_triggers[C].NewMap + 1; // + 26   + C;;
      char String[3];

      String[0] = (NewMap < 10 ? ' ' : '0' + NewMap / 10);
      String[1] = '0' + NewMap % 10;
      String[2] = 0;

      /*short X = Map_triggers[C].X-FgX+1;
      short Y = Map_triggers[C].Y-FgY-8;*/

      if (((Map_triggers[C].X + 1) >= FgX) &&
          (((Map_triggers[C].X + 1) <= (FgX + screen_width))) &&
          ((Map_triggers[C].Y - 8) >= FgY) &&
          ((Map_triggers[C].Y - 8) <= (FgY + screen_height))) {
        // DrawGrayStrExt2B(Map_triggers[C].X-FgX+1,Map_triggers[C].Y-FgY-8,String,A_NORMAL,F_6x8,dBufHPL_G,dBufHPD_G);
        DrawString(Map_triggers[C].X - FgX + 1, Map_triggers[C].Y - FgY - 8,
                   String, A_NORMAL, F_6x8);
      }
    }
  }

  if (Player.Face == 1) {
    Left = 1;
  } else {
    Left = 0;
  };
  if (Player.Face == 2) {
    Right = 4;
  } else {
    if (Player.Face == -2) {
      Right = 9;
    } else {
      Right = 0;
    };
  };

  if (SavePlayer.IsClouded) {
    Sprite = cloud_item_spr;
    Mask = cloud_item_msk;
  } else {
    Sprite = Mariosprites + 2 * Marioanimtab[Left][Right];
    Mask = Mariomasks + Marioanimtab[Left][Right];
  }
  // GrayClipSprite16_MASK_R(Player.MapX-FgX,Player.MapY-FgY,16,Mariosprites+2*Marioanimtab[Left][Right],Mariosprites+2*Marioanimtab[Left][Right]+16,Mariomasks+Marioanimtab[Left][Right],Mariomasks+Marioanimtab[Left][Right],GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_SMASK_R(Player.MapX-FgX,Player.MapY-FgY,16,Mariosprites+2*Marioanimtab[Left][Right],Mariosprites+2*Marioanimtab[Left][Right]+16,Mariomasks+Marioanimtab[Left][Right],GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_SMASK_R(SavePlayer.MapX-FgX,SavePlayer.MapY-FgY,16,Sprite,Sprite+16,Mask,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
  DrawSprite16SMASKR(SavePlayer.MapX - FgX, SavePlayer.MapY - FgY, 16, Sprite,
                     Sprite + 16, Mask);

  // GrayClipSprite16_MASK_R(Player.MapX-FgX,Player.MapY-FgY,16,Sprite,Sprite+16,Mask,Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
  // GrayClipSprite16_MASK_R(Player.MapX-FgX,Player.MapY-FgY,16,Mariosprites,Mariosprites+16,Mariomasks,Mariomasks,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));

  // GrayClipSprite16_MASK_R(Player.MapX-FgX,Player.MapY-FgY,27,Mariosprites+2*Marioanimtab[1][9],Mariosprites+2*Marioanimtab[1][9]+Player.Height,Mariomasks+Marioanimtab[1][9],Mariomasks+Marioanimtab[1][9],GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));

  for (C = 0; C < Map_data.Nr_of_objects; C++) {

    if (Map_objects[C].Mode) {

      // GrayClipSprite16_MASK_R(Map_objects[C].X-FgX,Map_objects[C].Y-FgY,16,Map_objects[C].Sprite,Map_objects[C].Sprite+16,Map_objects[C].Mask,Map_objects[C].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Map_objects[C].X-FgX,Map_objects[C].Y-FgY,16,Map_objects[C].Sprite,Map_objects[C].Sprite+16,Map_objects[C].Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(Map_objects[C].X-FgX,Map_objects[C].Y-FgY,16,Map_objects[C].Sprite,Map_objects[C].Sprite+16,Map_objects[C].Sprite+16*2,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      DrawSprite(Map_objects[C].X, Map_objects[C].Y, 16, Map_objects[C].Sprite,
                 16);
    };
  };

  // draw map statusbar here
  // clear 16 lowest lines
  /*
  memset(GrayDBufGetHiddenPlane(LIGHT_PLANE)+30*(screen_height-16),0,30*16);
  memset(GrayDBufGetHiddenPlane(DARK_PLANE)+30*(screen_height-16),0,30*16);
  */
  /*
  for(C=0;C<30*16;C++)
          *(((char*)GrayDBufGetHiddenPlane(LIGHT_PLANE)+30*(screen_height-16))+C)
  = 0;//size optimizaton

  for(C=0;C<30*16;C++)
          *(((char*)GrayDBufGetHiddenPlane(DARK_PLANE)+30*(screen_height-16))+C)
  = 0;//size optimizaton
  */
  // Optimized further
  // FastFilledRect_Erase_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,0,screen_height-16,240,screen_height);
  FilledRectEraseLight(0, screen_height - 16, 240, screen_height);
  // FastFilledRect_Erase_R(/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,0,screen_height-16,240,screen_height);
  FilledRectEraseDark(0, screen_height - 16, 240, screen_height);

  if (Map_statusbar == status) {

    for (C = 1; C <= 3; C++) {
      // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,screen_width-18*C,screen_height-16,screen_width-18*C+17,screen_height-1,COLOR_DARKGRAY);
      GrayOutlineRect(screen_width - 18 * C, screen_height - 16,
                      screen_width - 18 * C + 17, screen_height - 1,
                      COLOR_DARKGRAY);
    };
    for (C = 0; C <= (SavePlayer.CurrCard - 1); C++) {
      GrayClipISprite16_RPLC_R(
          screen_width - 18 * 3 + 1 + 18 * C, screen_height - 16, 16,
          (unsigned short *)Fg_plane.p.sprites +
              (79 + SavePlayer.Cards[C]) * 32,
          /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
          /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);
    };

    char String[4];

    String[0] = 'x';
    String[2] = ' ';
    String[3] = 0;

    String[2] = '0' + SavePlayer.Lives % 10;
    if (SavePlayer.Lives >= 10) {
      String[1] = '0' + SavePlayer.Lives / 10;
    } else {
      String[1] = ' ';
    };

    // DrawGrayStrExt2B(18,screen_height-16+2,String,A_REPLACE|A_SHADOWED,F_8x10,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
    DrawString(18, screen_height - 16 + 2, String, A_REPLACE | A_SHADOWED,
               F_8x10);

    String[2] = '0' + SavePlayer.Coins % 10;
    if (SavePlayer.Coins >= 10) {
      String[1] = '0' + SavePlayer.Coins / 10;
    } else {
      String[1] = ' ';
    };

    // DrawGrayStrExt2B(66,screen_height-16+2,String,A_REPLACE|A_SHADOWED,F_8x10,dBufHPL_G,dBufHPD_G);
    DrawString(66, screen_height - 16 + 2, String, A_REPLACE | A_SHADOWED,
               F_8x10);

    //		String[0]=String[1]=String[2]=String[3]=0;
    //		sprintf (String, " %d", SavePlayer.IsOnMapBoat );//Grevling
    // Get_map_tile(3*16-1,3*16-1) Map_plane.nb_anim SavePlayer.MapX

    //		DrawGrayStrExt2B(18,screen_height-16+2,String,A_REPLACE|A_SHADOWED,F_8x10,dBufHPL_G,dBufHPD_G);

    if (SavePlayer.Attribs & 0b00100000) { // draw racoon suit cap
      C = 5 * Marioanimtab[7][9];
      // GrayClipSprite16_MASK_R(2,screen_height-16,5,racoon_cap_spr+2*C,racoon_cap_spr+5+2*C,racoon_cap_msk+C,racoon_cap_msk+C,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(2,screen_height-16,5,racoon_cap_spr+2*C,racoon_cap_spr+5+2*C,racoon_cap_msk+C,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
      DrawSprite16SMASKR(2, screen_height - 16, 5, racoon_cap_spr + 2 * C,
                         racoon_cap_spr + 5 + 2 * C, racoon_cap_msk + C);
    };

    if ((SavePlayer.Attribs & 0b10000000) && (Map_plane.step % 2)) {
      Left = DARK_PLANE;
      Right = LIGHT_PLANE;
    } else {
      Left = LIGHT_PLANE;
      Right = DARK_PLANE;
    };

    // GrayClipSprite16_MASK_R(2,screen_height-16+1,14,Mariosprites+2*Marioanimtab[Player.Spritebase/*2*/][9],Mariosprites+2*Marioanimtab[Player.Spritebase/*2*/][9]+/*27*/Player.Height,Mariomasks+Marioanimtab[Player.Spritebase/*2*/][9],Mariomasks+Marioanimtab[Player.Spritebase/*2*/][9],GrayDBufGetHiddenPlane(Left/*LIGHT_PLANE*/),GrayDBufGetHiddenPlane(Right/*DARK_PLANE*/));
    GrayClipSprite16_SMASK_R(
        2, screen_height - 16 + 1, 14,
        Mariosprites + 2 * Marioanimtab[SavePlayer.Spritebase /*2*/][9],
        Mariosprites + 2 * Marioanimtab[SavePlayer.Spritebase /*2*/][9] +
            /*27*/ Player.Height2,
        Mariomasks + Marioanimtab[SavePlayer.Spritebase /*2*/][9],
        GrayDBufGetHiddenPlane(Left /*LIGHT_PLANE*/),
        GrayDBufGetHiddenPlane(Right /*DARK_PLANE*/));

    GrayClipISprite16_RPLC_R(50, screen_height - 16, 16,
                             (unsigned short *)Fg_plane.p.sprites + (4) * 32,
                             /*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/ dBufHPL_G,
                             /*GrayDBufGetHiddenPlane(DARK_PLANE)*/ dBufHPD_G);

    //	GrayDBufToggleSync_SetPointers();
    //	while(1);
  } else { // Map_statusbar==itemlist

    Right = min(max(Player.Curr_item + (short)(screen_width / 36),
                    (short)(screen_width / 18)),
                itemlist_length);
    Left = Right - (short)(screen_width / 18);

    for (C = Left; C < Right; C++) {
      /*
      Height = 16;

      switch(SavePlayer.Itemlist[C]){

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
              */
      /*
      case 5:{
              Sprite = ;
              Mask = ;
              break;
      };


      default:{
              Sprite = blank_sprite;
              //Mask = blank_sprite;
              //Height = 16;
      };
};
*/
      // draw items
      // GrayClipSprite16_MASK_R(10+18*(C-Left),screen_height-16,Height,Sprite,Sprite+Height,Mask,Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
      // GrayClipSprite16_SMASK_R(10+18*(C-Left),screen_height-16,Height,Sprite,Sprite+Height,Mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));

      //			GrayClipSprite16_SMASK_R(10+18*(C-Left),screen_height-16,Height,Sprite,Sprite+Height,Sprite+2*Height,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);

      DrawItem(10 + 18 * (C - Left), screen_height - 16,
               SavePlayer.Itemlist[C]);

      if (C == Player.Curr_item) { // draw cursor
        // GrayFastOutlineRect_R(/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,9+18*(C-Left),screen_height-16,10+18*(C-Left)+16,screen_height-1,COLOR_BLACK/*COLOR_DARKGRAY*/);
        GrayOutlineRect(9 + 18 * (C - Left), screen_height - 16,
                        10 + 18 * (C - Left) + 16, screen_height - 1,
                        COLOR_BLACK);
      };
    };
  };
  /*
  GrayDBufToggleSync(); 	//set drawbuffer as active screen

  dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE);
  dBufHPD_G = GrayDBufGetHiddenPlane(DARK_PLANE);
  */
  GrayDBufToggleSync_SetPointers();
};

void DrawItem(short X, short Y, short Item) {
  short Height = 16;
  unsigned short *Sprite;

  switch (Item) {
  /*case 0:{
          Sprite = blank_sprite;
          Mask = blank_sprite;
          Height = 0;
          break;
  };*/
  case 1: {
    Sprite = pu_mushrom_sprite;
    // Mask = mushrom_mask;
    // Height = 16;
    break;
  };
  case 2: {
    Sprite = flower_spr;
    // Mask = flower_msk;
    // Height = 16;
    break;
  };
  case 3: {
    Sprite = leaf_r_sprite;
    // Mask = leaf_r_mask;
    Height = 14;
    break;
  };
  case 4: {
    Sprite = star_anim;
    // Mask = star_mask;
    break;
  };
  case 5: {
    Sprite = whistle_spr;
    // Mask = whistle_msk;
    break;
  };
  case 6: {
    Sprite = hammer_i_spr;
    // Mask = hammer_i_msk;
    break;
  };

  case 7: {
    Sprite = p_wing_spr;
    // Mask = p_wing_msk;
    break;
  };
  case 8: {
    Sprite = cloud_item_spr;
    // Mask = cloud_item_msk;
    break;
  };
  case 9: {
    Sprite = anchor_spr;
    // Mask = anchor_msk;
    break;
  };

  default: {
    Sprite = blank_sprite;
    // Mask = blank_sprite;
    // Height = 16;
  };
  };

  // GrayClipSprite16_SMASK_R(X,Y,Height,Sprite,Sprite+Height,Sprite+2*Height,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
  DrawSprite16SMASKR(X, Y, Height, Sprite, Sprite + Height,
                     Sprite + 2 * Height);
}