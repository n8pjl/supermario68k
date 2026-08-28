

#include "all.h"

short Show_titlescreen() { // New: V 1.01 Changed return type from void to short
  short C, H = 19;

  // New: V 1.01 Added error check
  if (Load_level(Commonfilename, 7)) {
    Exit = 1;
    //		Error = 3;
    return 3;
  };
  // End of new

  Update_FG = Bg_plane.force_update = 1;

  Adjust_renderpoint();

#ifdef PRODUCE_TI89_CODE
  FgY += 8;
#endif

#ifdef PRODUCE_V200_CODE
  /*FgX = */ FgY = 0;
  BgY = 32;
#endif

#ifdef PRODUCE_TI92PLUS_CODE
  /*FgX = */ FgY = 0;
  BgY = 32;
#endif

  StatBarHeight = 0;
  Render();
  GrayDBufToggleSync_SetPointers(); // GrayDBufToggle();//Sync

  StatBarHeight = 8;

  for (C = 0; C < 7; C++) {
    H = 19 + (C == 6);

    // GrayClipSprite16_SMASK_R(23+16*C,17,H,titlescreen_gfx+2*19*C,titlescreen_gfx+2*19*C+H,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
    // GrayClipSprite16_SMASK_R(screen_offset_sg_x+23+16*C,10,H,titlescreen_gfx+3*19*C,titlescreen_gfx+3*19*C+H,titlescreen_gfx+3*19*C+2*H,dBufHPL_G,dBufHPD_G);
    DrawSprite16SMASKR(
        screen_offset_sg_x + 23 + 16 * C, 10, H, titlescreen_gfx + 3 * 19 * C,
        titlescreen_gfx + 3 * 19 * C + H, titlescreen_gfx + 3 * 19 * C + 2 * H);
  }

  for (C = 0; C < 4; C++) {

    // GrayClipSprite16_SMASK_R(46+16*C,44,21,titlescreen_gfx +6*19*2+20*2+
    // 2*21*C,titlescreen_gfx +6*19*2+20*2+
    // +2*21*C+21,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
    // GrayClipSprite16_SMASK_R(screen_offset_sg_x+54+16*C,37,21,titlescreen_gfx
    // +6*19*3+20*3+ 3*21*C,titlescreen_gfx +6*19*3+20*3+
    // +3*21*C+21,titlescreen_gfx +6*19*3+20*3+
    // +3*21*C+2*21,dBufHPL_G,dBufHPD_G);
    DrawSprite16SMASKR(screen_offset_sg_x + 54 + 16 * C, 37, 21,
                       titlescreen_gfx + 6 * 19 * 3 + 20 * 3 + 3 * 21 * C,
                       titlescreen_gfx + 6 * 19 * 3 + 20 * 3 + +3 * 21 * C + 21,
                       titlescreen_gfx + 6 * 19 * 3 + 20 * 3 + +3 * 21 * C +
                           2 * 21);
  }

  GrayDBufToggleSync_SetPointers(); // GrayDBufToggleSync();

  /*while(_rowread(0));//wait until key released
  while(!_rowread(0));//wait until key pressed*/
  WaitKeyPress();

  return 0;
};
