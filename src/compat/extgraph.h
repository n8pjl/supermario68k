#pragma once

#include <stdint.h>
// ExtGraph's header pulled in <stdlib.h> (for malloc, and its own min() macro);
// several sources include this header and lean on that. Keep providing it.
#include <stdlib.h>

// Declarations the game needs from ExtGraph, inlined here so the build no
// longer depends on the ExtGraph source tree. The routines are reimplemented
// in compat/extgraph.c (and GrayDrawStrExt2B in compat/graph.c); the shipped
// extgraph.a is m68k COFF and cannot be linked into a wasm build.

// Colours accepted by the grayscale utility routines.
enum GrayColors {
	COLOR_WHITE = 0,
	COLOR_LIGHTGRAY = 1,
	COLOR_DARKGRAY = 2,
	COLOR_BLACK = 3
};

// Extra attribute bits ORed into the drawing mode of GrayDrawStrExt2B.
enum ExtAttrs { A_CENTERED = 0x40, A_SHADOWED = 0x80 };

// Byte offset of pixel (x, y) from the start of a 240-pixel-wide plane
// (30 bytes a row): 30*y + x/8.
#define EXT_PIXOFFSET(x, y) ((((y) << 4) - (y)) * 2 + ((x) >> 3))

// Set pixel (x, y) in the 240-pixel-wide plane starting at p; pixel 0 of a row
// is bit 7 of the first byte. ExtGraph's original expands to m68k bset asm,
// which clang cannot build - this is the plain-C equivalent.
#define EXT_SETPIX(p, x, y) \
	(((uint8_t *)(p))[EXT_PIXOFFSET(x, y)] |= (uint8_t)(0x80 >> ((x) & 7)))

// Absolute value of a short, evaluating its argument once.
#define EXT_SHORTABS(a)                     \
	({                                  \
		int16_t __ta = (a);         \
		(__ta >= 0) ? __ta : -__ta; \
	})

// Bounding-box overlap test: true when the two w x h boxes at (x0,y0) and
// (x1,y1) intersect.
#define BOUNDS_COLLIDE(x0, y0, x1, y1, w, h)    \
	(((EXT_SHORTABS((x1) - (x0))) < (w)) && \
	 ((EXT_SHORTABS((y1) - (y0))) < (h)))
#define BOUNDS_COLLIDE16(x0, y0, x1, y1) BOUNDS_COLLIDE(x0, y0, x1, y1, 16, 16)

// Sprite/sprite pixel collision, two 16-wide sprites of independent heights.
int16_t TestCollide162h_R(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
			  uint16_t height0, uint16_t height1,
			  const uint16_t *data0, const uint16_t *data1);

// Fill / erase a hard-coded-mode filled rectangle in one plane.
void FastFilledRect_Draw_R(void *plane, uint16_t x1, uint16_t y1, uint16_t x2,
			   uint16_t y2);
void FastFilledRect_Erase_R(void *plane, uint16_t x1, uint16_t y1, uint16_t x2,
			    uint16_t y2);

// Draw a rectangle outline in the given GrayColors colour across both planes.
void GrayFastOutlineRect_R(void *dest0, void *dest1, uint16_t x1, uint16_t y1,
			   uint16_t x2, uint16_t y2, int16_t color);

// Clear both 240x128 planes.
void GrayClearScreen2B(void *lightplane, void *darkplane);

// 16-wide interlaced grayscale sprite, replace mode, screen-clipped.
void GrayClipISprite16_RPLC_R(int16_t x, int16_t y, uint16_t height,
			      const uint16_t *sprite, void *dest0, void *dest1);

// 16- and 8-wide grayscale sprites drawn through a shared mask, screen-clipped.
void GrayClipSprite16_SMASK_R(int16_t x, int16_t y, uint16_t height,
			      const uint16_t *sprt0, const uint16_t *sprt1,
			      const uint16_t *mask, void *dest0, void *dest1);
void GrayClipSprite8_SMASK_R(int16_t x, int16_t y, uint16_t height,
			     const uint8_t *sprt0, const uint8_t *sprt1,
			     const uint8_t *mask, void *dest0, void *dest1);

// 16-wide grayscale sprite flipped top to bottom, per-plane masks, clipped.
void UpsideDownGrayClipSprite16_MASK_R(int16_t x, int16_t y, uint16_t height,
				       uint16_t *sprt0, uint16_t *sprt1,
				       uint16_t *mask0, uint16_t *mask1,
				       void *dest0, void *dest1);

// Game-local SMASK blit restricted to the columns maskval selects; see
// Mario/GrayClipSprite16_SMASKBLIT_R.s and compat/extgraph.c.
void GrayClipSprite16_SMASKBLIT_R(int16_t x, int16_t y, uint16_t height,
				  uint16_t *sprt0, uint16_t *sprt1,
				  uint16_t *mask, const uint16_t maskval,
				  void *dest0, void *dest1);

// Draw a string into both planes with a bitmap font; attr may carry ExtAttrs.
void GrayDrawStrExt2B(uint16_t x, uint16_t y, const char *s, int16_t attr,
		      int16_t font, void *lightplane, void *darkplane);
// The game calls it by its older, TIGCCLIB-noncompliant name.
#define DrawGrayStrExt2B GrayDrawStrExt2B
