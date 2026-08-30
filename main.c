////////////////////////////////////////////////////////////////////////////////
//------------------------ Super Mario 68K -----------------------------------//
// Campatible with TI 89, TI 89 Titanium, TI 92+ and Voyage 200
// //
//							|
////
// Author: Lachprog
// // Email: lachprog@hotmail.com
// // I can also be reached at tiFreakware (http://tifreakware.net/)
// //
// 																																						//
// Project page: http://tifreakware.net/lachprog/
// // Version: 1.03
// // Release date: 20.01.08
// //
//																																						//
// Note: If you find any bugs or optimizations, please contact me!
// //
////////////////////////////////////////////////////////////////////////////////

// switch that gives warnings when inlineing fails: -Winline

#include "compat/alloc.h"
#include "compat/gray.h"
#include "control.h"
#include "custom.h"
#include "error.h"
#include "gameloop.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "menus.h"
#include "render.h"
#include "shells.h"
#include "smallgames.h"
#include "version.h"
#include <emscripten/em_asm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define Life_the_universe_and_everything 42

// #define OPTIMIZE_ROM_CALLS // Use ROM Call Optimization

void *Block;
void *Buffer; // pointer to video memory (gray doublebuffering mem and
// backbuffers for tilemap engine)

uint16_t *Mariosprites;
uint16_t *Mariomasks;
int16_t (*Marioanimtab)[11];

uint16_t *Enemysprites;

uint8_t *Smallsprites;

uint16_t *Sprites;

uint16_t *Itemsprites;

uint16_t *Boss_sprites;

char *Level, *Map;

void *Statusbar;

char Exit;
char ErrorCode;
bool Skip_anim;

char Map_statusbar;
uint8_t Treasure;

bool ti89_mode = false;

#ifdef speedtest
// Speedtest:
// Note: This does NOT give an accurate FPS, probably not even close to.
// It measures the number of frames within 255 ticks of AUTO_INT_1 (when
// grayscale is enabled.) It was used to give an indication of how effective
// frame skipping and increasing animation length was as a speed optimization.
// Those optimizations were aimed on Voyage 200 and Ti-92+ because the game run
// slower on those calcs.
volatile int16_t Fps;
DEFINE_INT_HANDLER(TESTCOUNTER)
{ // speed test
	counter++;
	if (counter == 0) {
		Update_frame_counter = Fps;
		Fps = 0;
	}
}
#endif
// short Slow;//used in debug only

// Main Function
int main(void)
{
	ErrorCode = 0;
	Skip_anim = 1;

	Map_statusbar = status;
	StatBarHeight = 8;

	// INT_HANDLER ai1,/*ai4,*/ai5;
	// initializations

	// ai1 = GetIntVec(AUTO_INT_1);
	//	ai4 = GetIntVec(AUTO_INT_4);
	// ai5 = GetIntVec(AUTO_INT_5);
	// SetIntVec(AUTO_INT_5,DUMMY_HANDLER);//speed test
#ifdef speedtest
// SetIntVec(AUTO_INT_1,TESTCOUNTER);//DUMMY_HANDLER);
#endif
#ifndef speedtest
// SetIntVec(AUTO_INT_1,DUMMY_HANDLER);//);
#endif
	//	SetIntVec(AUTO_INT_4,DUMMY_HANDLER);

	ti89_mode = EM_ASM_INT({
		// clang-format off
		return Module.ti89Mode ?? false;
		// clang-format on
	});

	init_calc_screen_constants(ti89_mode);

	// Error: Memory possible
	Block = malloc(
		(uint32_t)GRAYDBUFFER_SIZE + 2 * GRAY_BIG_VSCREEN_SIZE +
		BIG_VSCREEN_SIZE +
		480); // allocate 1 drawbuffer and 2 big_vsceens(1 for each plane)

	if (Block == NULL) {
		ErrorCode = 1;
		goto Quit; // OUT OF MEMORY, terminate
	}

	Buffer = Block;
	GrayDBufInit(Buffer); // initialize double buffering mode

	dBufHPL_G = GrayDBufGetHiddenPlane(LIGHT_PLANE);
	dBufHPD_G = GrayDBufGetHiddenPlane(DARK_PLANE);

	//
	// Error: Memory possible
	Items = (struct item *)malloc(sizeof(struct item) * nr_of_items +
				      sizeof(struct shell) * max_nr_of_shells +
				      sizeof(struct object) * nr_of_objects +
				      18); // 18 is for the card game

	if (Items == NULL) {
		ErrorCode = 1;
		goto Quit; // OUT OF MEMORY, terminate
	}

	Shells = (struct shell *)(Items + nr_of_items);
	Objects = (struct object *)(Shells + max_nr_of_shells);
	CardGame = (char *)(Objects + nr_of_objects);

	// Error: File(s) might be missing
	if ((ErrorCode = Load_gfx_from_file())) {
		goto Quit; // terminate if gfx files not found
	}

	//	Map_plane.tabanim = Map_animations;
	Map_plane.nb_anim = 110; // 105;
	Map_plane.nb_step = 4;
	Map_plane.step = 0;
	Map_plane.step_length = 6;
	Map_plane.frame = 0;
	//	Map_plane.p.sprites = Map_tiles;
	Map_plane.p.big_vscreen =
		Block + GRAYDBUFFER_SIZE; // use same drawbuffer as Fg_plane
	//	Map_plane.p.force_update = 1;

	//	Fg_plane.tabanim = Fg_animations;
	Fg_plane.nb_anim = 256;
	Fg_plane.nb_step = 4;
	Fg_plane.step = 0;

	// No need for speed up on TI 89
	Fg_plane.step_length = ti89_mode ? 5 : 6;

	Fg_plane.frame = 0;

	//	Fg_plane.p.sprites = Tiles;
	Fg_plane.p.big_vscreen = Block + GRAYDBUFFER_SIZE;
	//	Fg_plane.p.force_update = 1;

	//	Fg_mask.tabanim = Fg_mask_animations;
	Fg_mask.nb_anim = 256;
	Fg_mask.nb_step = 4;
	Fg_mask.step = 0;

	// No need for speed up on TI 89
	Fg_mask.step_length = ti89_mode ? 10 : 12;

	Fg_mask.frame = 0;

	//	Fg_mask.p.sprites = Tilemasks;
	Fg_mask.p.big_vscreen =
		Block + GRAYDBUFFER_SIZE + 2 * GRAY_BIG_VSCREEN_SIZE;
	//	Fg_mask.p.force_update = 1;

	//	Bg_plane.sprites = Bg_tiles;
	Bg_plane.big_vscreen = Block + GRAYDBUFFER_SIZE + GRAY_BIG_VSCREEN_SIZE;
	//	Bg_plane.force_update = 1;

	Statusbar = Block + GRAYDBUFFER_SIZE + 2 * GRAY_BIG_VSCREEN_SIZE +
		    BIG_VSCREEN_SIZE;

	Draw_statusbar();

	Exit = 0;

	// char CurrFolder[9];

	// FolderGetCur(CurrFolder);

	ErrorCode =
		Menus(); // This function enters the game itself. See "menus.c"

// FolderCur( SYMSTR(CurrFolder) , TRUE );
// exit stuff here
// free allocated mem
Quit:
	if (ErrorCode) {
		Errorhandler(ErrorCode);
	}

	/*short j;
  for(j=1;j<=7;j++){//test: Loops through all errormessages
          Errorhandler(j);
  }*/

	/*if(Items)
          free(Items);

  if(Block)
          free(Block);

  if(Level)
          free(Level);

  if(Map)
          free(Map);

  if(Filenames)
          free(Filenames);*/

	Free(Items);
	Free(Block);
	Free(Level);
	Free(Map);
	Free(Filenames);

	// unlock memblocks used by GFX-files
	HeapUnlock(Tilefile_sym_h);
	HeapUnlock(Spritefile_sym_h);
	HeapUnlock(Bg_file_sym_h);
	HeapUnlock(TextFile_sym_h);

	// reinstall all timers/interrupts
	//  SetIntVec(AUTO_INT_5,ai5);
	//  SetIntVec(AUTO_INT_1,ai1);
	//	SetIntVec(AUTO_INT_4,ai4);
	//	while(_rowread(0));//wait until key released

	// for(C=0;C<SHRT_MAX;C++);
	DelayNFrames(DEFAULT_FPS);

	// GKeyFlush();
}
