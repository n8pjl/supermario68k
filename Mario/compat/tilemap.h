#pragma once

#include "../../ExtGraph/lib/tilemap.h"

// The game targets the older TileMap engine (the TileMap.a shipped alongside
// the sources), whose gray entry points take the light and dark planes as two
// separate arguments. ExtGraph 2.x reworked those to a single interleaved
// destination, so its TM_* macros have the wrong arity here and are replaced.

#undef TM_G16B
#undef TM_GA16B
#undef TM_GRPLC
#undef TM_GOR
#undef TM_GRPLC89
#undef TM_GOR89
#undef TM_MASK
#undef TM_MASK89

// Blitter callback: copies one refreshed buffer onto both planes.
typedef void (*TM_GrayMode)(const void *src, unsigned short x, unsigned short y,
			    void *lightplane, void *darkplane);

void DrawGrayBuffer2B_RPLC(const void *src, unsigned short x, unsigned short y,
			   void *lightplane, void *darkplane);
void DrawGrayBuffer2B_OR(const void *src, unsigned short x, unsigned short y,
			 void *lightplane, void *darkplane);

// The "89" variants differ from the others only in copying a 160x100 window
// instead of a 240x128 one - the destination step is a full 30-byte plane row
// either way - and the ports take that window from LCD_WIDTH/LCD_HEIGHT, which
// the build target already fixes. So one function serves both names, and
// render.c can keep asking for whichever its per-model #ifdef picks.
#define TM_GRPLC DrawGrayBuffer2B_RPLC
#define TM_GOR DrawGrayBuffer2B_OR
#define TM_MASK DrawBuffer_MASK
#define TM_GRPLC89 DrawGrayBuffer2B_RPLC
#define TM_GOR89 DrawGrayBuffer2B_OR
#define TM_MASK89 DrawBuffer_MASK

// Plane drawers. _ROLL is the game's own scrolling variant of the 16x16 big
// tile renderer (see DrawGrayPlane16BRoll referenced from render.c).
void DrawGrayPlane16B2B(unsigned short x, unsigned short y, Plane *plane,
			void *lightplane, void *darkplane, TM_GrayMode mode);
void DrawGrayPlane16B2B_ROLL(unsigned short x, unsigned short y, Plane *plane,
			     void *lightplane, void *darkplane, TM_GrayMode mode);
void DrawGrayAnimatedPlane16B2B(unsigned short x, unsigned short y,
				AnimatedPlane *plane, void *lightplane,
				void *darkplane, TM_GrayMode mode);

#define TM_G16B DrawGrayPlane16B2B
#define TM_G16B_ROLL DrawGrayPlane16B2B_ROLL
#define TM_GA16B DrawGrayAnimatedPlane16B2B

#define DrawGrayPlane(x, y, plane, dest0, dest1, mode, type) \
	type((x), (y), (plane), (dest0), (dest1), (mode))
#define DrawGrayAnimatedPlane(x, y, plane, dest0, dest1, mode, type) \
	type((x), (y), (plane), (dest0), (dest1), (mode))
