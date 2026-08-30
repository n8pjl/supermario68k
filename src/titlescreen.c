

#include "titlescreen.h"
#include "control.h"
#include "gfx.h"
#include "level.h"
#include "render.h"
#include "scankeys.h"
#include "version.h"
#include <stdint.h>

int16_t Show_titlescreen()
{ // New: V 1.01 Changed return type from void to short
	int16_t C, H = 19;

	// New: V 1.01 Added error check
	if (Load_level(Commonfilename, 7)) {
		Exit = 1;
		//		Error = 3;
		return 3;
	};
	// End of new

	Update_FG = Bg_plane.force_update = 1;

	Adjust_renderpoint();

	if (ti89_mode) {
		FgY += 8;
	} else {
		FgY = 0;
		BgY = 32;
	}

	StatBarHeight = 0;
	Render();
	GrayDBufToggleSync_SetPointers(); // GrayDBufToggle();//Sync

	StatBarHeight = 8;

	for (C = 0; C < 7; C++) {
		H = 19 + (C == 6);

		// GrayClipSprite16_SMASK_R(23+16*C,17,H,titlescreen_gfx+2*19*C,titlescreen_gfx+2*19*C+H,blank_sprite,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
		// GrayClipSprite16_SMASK_R(screen_offset_sg_x+23+16*C,10,H,titlescreen_gfx+3*19*C,titlescreen_gfx+3*19*C+H,titlescreen_gfx+3*19*C+2*H,dBufHPL_G,dBufHPD_G);
		DrawSprite16SMASKR(screen_offset_sg_x + 23 + 16 * C, 10, H,
				   titlescreen_gfx + 3 * 19 * C,
				   titlescreen_gfx + 3 * 19 * C + H,
				   titlescreen_gfx + 3 * 19 * C + 2 * H);
	}

	for (C = 0; C < 4; C++) {
		DrawSprite16SMASKR(screen_offset_sg_x + 54 + 16 * C, 37, 21,
				   titlescreen_gfx + 6 * 19 * 3 + 20 * 3 +
					   3 * 21 * C,
				   titlescreen_gfx + 6 * 19 * 3 + 20 * 3 +
					   +3 * 21 * C + 21,
				   titlescreen_gfx + 6 * 19 * 3 + 20 * 3 +
					   +3 * 21 * C + 2 * 21);
	}

	GrayDBufToggleSync_SetPointers();

	WaitKeyPress();

	return 0;
};
