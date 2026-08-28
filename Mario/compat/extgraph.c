#include "extgraph.h"

#include <string.h>
#include "graph.h"
#include "../../ExtGraph/lib/tilemap.h"

// Reimplementations of the ExtGraph and TileMap entry points the game calls.
// The shipped extgraph.a / TileMap.a are m68k COFF and cannot be linked into a
// wasm build, but the library's original sources are in ExtGraph/src/lib and
// serve as the reference for these ports.
//
// IMPORTANT: sprite, mask and tile data all come from the big-endian m68k data
// blobs, which are deliberately left unswapped. Every 16-bit pixel row read in
// this file must therefore be read big-endian - see be16(). Getting this wrong
// transposes the two 8-pixel halves of every row.
//
// TODO: all of the below are stubs pending the real ports.

void FastFilledRect_Draw_R(void *plane, unsigned short x1, unsigned short y1,
			   unsigned short x2, unsigned short y2)
{
	(void)plane; (void)x1; (void)y1; (void)x2; (void)y2;
}

void FastFilledRect_Erase_R(void *plane, unsigned short x1, unsigned short y1,
			    unsigned short x2, unsigned short y2)
{
	(void)plane; (void)x1; (void)y1; (void)x2; (void)y2;
}

void GrayFastOutlineRect_R(void *dest0, void *dest1, unsigned short x1,
			   unsigned short y1, unsigned short x2,
			   unsigned short y2, short color)
{
	(void)dest0; (void)dest1; (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}

void GrayClipISprite16_RPLC_R(short x, short y, unsigned short height,
			      const unsigned short *sprite, void *dest0,
			      void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprite; (void)dest0; (void)dest1;
}

void GrayClipSprite16_SMASK_R(short x, short y, unsigned short height,
			      const unsigned short *sprt0,
			      const unsigned short *sprt1,
			      const unsigned short *mask, void *dest0, void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprt0; (void)sprt1; (void)mask;
	(void)dest0; (void)dest1;
}

void GrayClipSprite8_SMASK_R(short x, short y, unsigned short height,
			     const unsigned char *sprt0, const unsigned char *sprt1,
			     const unsigned char *mask, void *dest0, void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprt0; (void)sprt1; (void)mask;
	(void)dest0; (void)dest1;
}

short TestCollide162h_R(short x0, short y0, short x1, short y1,
			unsigned short height0, unsigned short height1,
			const unsigned short *data0, const unsigned short *data1)
{
	(void)x0; (void)y0; (void)x1; (void)y1; (void)height0; (void)height1;
	(void)data0; (void)data1;
	return 0;
}


// Mono buffer blitter used for the foreground mask plane.
void DrawBuffer_MASK(const void *src, unsigned short x, unsigned short y,
		     void *dest)
{
	(void)src; (void)x; (void)y; (void)dest;
}

void GrayClearScreen2B(void *lightplane, void *darkplane)
{
	memset(lightplane, 0, LCD_SIZE);
	memset(darkplane, 0, LCD_SIZE);
}

void UpsideDownGrayClipSprite16_MASK_R(short x, short y, unsigned short height,
				       unsigned short *sprt0, unsigned short *sprt1,
				       unsigned short *mask0, unsigned short *mask1,
				       void *dest0, void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprt0; (void)sprt1;
	(void)mask0; (void)mask1; (void)dest0; (void)dest1;
}

// Game-local routine (see Mario/GrayClipSprite16_SMASKBLIT_R.s): a SMASK blit
// that also ORs a constant `maskval` row into the mask.
void GrayClipSprite16_SMASKBLIT_R(short x, short y, unsigned short height,
				  unsigned short *sprt0, unsigned short *sprt1,
				  unsigned short *mask, const unsigned short maskval,
				  void *dest0, void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprt0; (void)sprt1; (void)mask;
	(void)maskval; (void)dest0; (void)dest1;
}

// Mono animated plane renderer (TileMap engine).
void DrawAnimatedPlane16B(unsigned short x, unsigned short y,
			  AnimatedPlane *plane, void *dest, TM_Mode mode)
{
	(void)x; (void)y; (void)plane; (void)dest; (void)mode;
}
