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
// Planes are 240x128, one bit per pixel, 30 bytes per row, pixel 0 of a row in
// bit 7 of the first byte.
//
// TODO: the remaining stubs below are pending the real ports.

#define PLANE_STRIDE 30

// Set or clear the horizontal run of pixels x1..x2 (inclusive) on row y.
//
// The ExtGraph originals work a word at a time and unroll to 64-bit writes;
// that is a speed trick for the 68000 and byte-at-a-time is equivalent, so
// this stays bytewise. It is also endian-agnostic, which matters because the
// planes are shared with sprite data read big-endian elsewhere in this file.
static void hspan(unsigned char *plane, short x1, short x2, short y, short set)
{
	unsigned char *row;
	short b, b1, b2;
	unsigned char m1, m2, m;

	if (y < 0 || y >= LCD_HEIGHT || x2 < x1 || x2 < 0 || x1 > LCD_WIDTH - 1)
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 > LCD_WIDTH - 1)
		x2 = LCD_WIDTH - 1;

	row = plane + (short)(y * PLANE_STRIDE);
	b1 = x1 >> 3;
	b2 = x2 >> 3;
	m1 = (unsigned char)(0xFF >> (x1 & 7));
	m2 = (unsigned char)(0xFF << (7 - (x2 & 7)));

	if (b1 == b2) {
		m = m1 & m2;
		if (set)
			row[b1] |= m;
		else
			row[b1] &= (unsigned char)~m;
		return;
	}

	if (set) {
		row[b1] |= m1;
		for (b = b1 + 1; b < b2; b++)
			row[b] = 0xFF;
		row[b2] |= m2;
	} else {
		row[b1] &= (unsigned char)~m1;
		for (b = b1 + 1; b < b2; b++)
			row[b] = 0x00;
		row[b2] &= (unsigned char)~m2;
	}
}

// Set or clear the filled rectangle (x1,y1)-(x2,y2), corners included. The
// originals swap the corners if they arrive the wrong way round, and clip
// nothing at all - a caller passing an off-screen coordinate scribbles past
// the plane. Here the span helper clips instead, which cannot change the
// result for the on-screen coordinates the game actually passes.
static void filled_rect(void *plane, short x1, short y1, short x2, short y2,
			short set)
{
	short y, t;

	if (y2 < y1) {
		t = y1; y1 = y2; y2 = t;
	}
	if (x2 < x1) {
		t = x1; x1 = x2; x2 = t;
	}
	for (y = y1; y <= y2; y++)
		hspan((unsigned char *)plane, x1, x2, y, set);
}

void FastFilledRect_Draw_R(void *plane, unsigned short x1, unsigned short y1,
			   unsigned short x2, unsigned short y2)
{
	filled_rect(plane, (short)x1, (short)y1, (short)x2, (short)y2, 1);
}

void FastFilledRect_Erase_R(void *plane, unsigned short x1, unsigned short y1,
			    unsigned short x2, unsigned short y2)
{
	filled_rect(plane, (short)x1, (short)y1, (short)x2, (short)y2, 0);
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
