#include "extgraph.h"

#include <string.h>
#include "graph.h"

// Reimplementations of the ExtGraph entry points the game calls. The shipped
// extgraph.a is m68k COFF and cannot be linked into a wasm build, but the
// library's original sources serve as the reference for these ports. The
// TileMap engine's own entry points live in tilemap.c.
//
// Sprite, mask and tile rows are 16-bit and arrive already byte-swapped:
// tools/mkdata.py converts the 16-bit regions of the GFX blobs at build time,
// so they can be read natively here. The two byte-addressed regions it leaves
// alone - Smallsprites and Games - are exactly the ones nothing in this file
// reads as words.
//
// Planes are 240x128 on every model, one bit per pixel, 30 bytes per row,
// pixel 0 of a row in bit 7 of the first byte; see compat/graph.h. The
// originals clip against that, not against the smaller visible area of a
// TI-89, because there is only one set of sprite routines for all models - so
// these use PLANE_WIDTH/PLANE_HEIGHT rather than PLANE_WIDTH/PLANE_HEIGHT.
//
// TODO: the remaining stubs below are pending the real ports.

// Set or clear the horizontal run of pixels x1..x2 (inclusive) on row y.
// Endpoints are swapped if they arrive the wrong way round, matching what
// ExtGraph's FastDrawHLine_R does internally.
//
// The ExtGraph originals work a word at a time and unroll to 64-bit writes;
// that is a speed trick for the 68000 and byte-at-a-time is equivalent, so
// this stays bytewise. It is also endian-agnostic, which matters because the
// planes are shared with sprite data read big-endian elsewhere in this file.
static void hspan(unsigned char *plane, short x1, short x2, short y, short set)
{
	unsigned char *row;
	short b, b1, b2, t;
	unsigned char m1, m2, m;

	if (x2 < x1) {
		t = x1; x1 = x2; x2 = t;
	}
	if (y < 0 || y >= PLANE_HEIGHT || x2 < 0 || x1 > PLANE_WIDTH - 1)
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 > PLANE_WIDTH - 1)
		x2 = PLANE_WIDTH - 1;

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

	// Only the row order matters here; hspan() orders the columns itself.
	if (y2 < y1) {
		t = y1; y1 = y2; y2 = t;
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

// Set or clear the vertical run of pixels y1..y2 (inclusive) in column x.
// Swaps its endpoints, as FastDrawVLine_R does.
static void vspan(unsigned char *plane, short x, short y1, short y2, short set)
{
	unsigned char *p;
	unsigned char m;
	short y, t;

	if (y2 < y1) {
		t = y1; y1 = y2; y2 = t;
	}
	if (x < 0 || x >= PLANE_WIDTH || y2 < 0 || y1 > PLANE_HEIGHT - 1)
		return;
	if (y1 < 0)
		y1 = 0;
	if (y2 > PLANE_HEIGHT - 1)
		y2 = PLANE_HEIGHT - 1;

	m = (unsigned char)(1 << (7 - (x & 7)));
	p = plane + (short)(y1 * PLANE_STRIDE) + (x >> 3);
	for (y = y1; y <= y2; y++, p += PLANE_STRIDE) {
		if (set)
			*p |= m;
		else
			*p &= (unsigned char)~m;
	}
}

// The four edges of a rectangle in one plane.
static void outline(void *plane, short x1, short y1, short x2, short y2,
		    short set)
{
	unsigned char *pl = (unsigned char *)plane;

	hspan(pl, x1, x2, y1, set);
	hspan(pl, x1, x2, y2, set);
	vspan(pl, x1, y1, y2, set);
	vspan(pl, x2, y1, y2, set);
}

// Draw the outline of a rectangle in the given gray color.
//
// The original is C already, four FastDrawHLine_R/FastDrawVLine_R calls per
// plane with A_NORMAL or A_REVERSE picked by the color. Its tests read
// `color & (COLOR_WHITE | COLOR_LIGHTGRAY)` and `color & (COLOR_DARKGRAY |
// COLOR_WHITE)`, but COLOR_WHITE is 0, so those are just bit 0 for the light
// plane and bit 1 for the dark plane - and the else branches mean the edge is
// actively *cleared* in the plane whose bit is off, not left alone.
//
// The original does not swap the corners here; each FastDraw*Line_R swaps its
// own endpoints instead, which comes to the same rectangle. hspan()/vspan()
// follow that contract, so no swap is needed at this level either.
void GrayFastOutlineRect_R(void *dest0, void *dest1, unsigned short x1,
			   unsigned short y1, unsigned short x2,
			   unsigned short y2, short color)
{
	outline(dest0, (short)x1, (short)y1, (short)x2, (short)y2, color & 1);
	outline(dest1, (short)x1, (short)y1, (short)x2, (short)y2, color & 2);
}

// Write the 16-pixel row `val` at absolute pixel (x, y) of a plane, under the
// write-enable mask `wr`: pixels whose `wr` bit is clear keep the plane's
// existing bit, and `val` is ORed in on top. Pixel 0 of both is bit 15.
// Anything falling off the left or right edge is dropped.
//
// That OR is not an accident of the phrasing - the originals really do
// `and.l` the mask into the destination and then `or.l` the sprite word in,
// so a sprite bit set outside its own mask lights the pixel up instead of
// being dropped. Well-formed sprites never have one, but the ports match the
// hardware rather than assuming that.
//
// The originals reach this through four hand-written cases - fully on screen
// split by whether x&15 is under 8, clipped left, clipped right - because the
// 68000 wants the destination read as an aligned 32-bit long and that long
// would run off the end of the row for the last word. Sliding a three-byte
// window instead covers all of them at once: 16 pixels at a bit offset of 0..7
// never span more than three bytes.
//
// Taking the write mask as a parameter is what lets the masked blitters share
// this: for a plain replace it is all ones, for a sprite drawn through a
// SMASK-style mask it is that mask complemented, since there a set bit means
// "keep the background".
static void put_row16(unsigned char *plane, short x, short y, unsigned val,
		      unsigned wr)
{
	unsigned long v, m;
	short xs, bx, sh, i;

	if (y < 0 || y >= PLANE_HEIGHT || x <= -16 || x >= PLANE_WIDTH)
		return;

	// Bias by two bytes so the shift and remainder stay non-negative for
	// the leftmost clipped case; 16 is a whole number of bytes, so x & 7 is
	// unchanged.
	xs = x + 16;
	bx = (short)(xs >> 3) - 2;
	sh = xs & 7;

	v = (unsigned long)(val & 0xFFFF) << (8 - sh);
	m = (unsigned long)(wr & 0xFFFF) << (8 - sh);

	plane += y * PLANE_STRIDE;
	for (i = 0; i < 3; i++) {
		short b = bx + i;
		unsigned char mb, vb;

		if (b < 0 || b >= PLANE_STRIDE)
			continue;
		mb = (unsigned char)(m >> (16 - 8 * i));
		vb = (unsigned char)(v >> (16 - 8 * i));
		plane[b] = (unsigned char)((plane[b] & ~mb) | vb);
	}
}

// Draw a 16-wide interlaced grayscale sprite, replacing what is under it, and
// clip it to the screen.
//
// "Interlaced" is the sprite layout: the two planes' rows alternate in one
// array, dest0's row then dest1's, so a row pair is two words. There is no
// mask - RPLC overwrites all 16 pixels.
//
// Vertical clipping just trims the row range and steps the sprite pointer past
// the rows above the screen; the original folds that into the same registers it
// uses for the destination offset, but it is only a range intersection.
void GrayClipISprite16_RPLC_R(short x, short y, unsigned short height,
			      const unsigned short *sprite, void *dest0,
			      void *dest1)
{
	int first = y < 0 ? -y : 0;		// first sprite row on screen
	int last = height;			// one past the last
	int r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		put_row16((unsigned char *)dest0, x, (short)(y + r),
			  sprite[2 * r], 0xFFFF);
		put_row16((unsigned char *)dest1, x, (short)(y + r),
			  sprite[2 * r + 1], 0xFFFF);
	}
}

// Draw a 16-wide grayscale sprite through a mask, and clip it to the screen.
//
// Unlike the interlaced RPLC above, the two planes' rows live in separate
// arrays, sprt0 and sprt1, and share one mask. The mask runs the other way
// round from put_row16's write enable: a set bit means "keep the background",
// which is why it is complemented on the way in. Per row and plane the
// original is
//
//	dest = (dest & mask) | sprite
//
// The original reaches that through three hand-written cases - on screen,
// clipped left, clipped right - differing only in whether the destination is
// touched as a long or a word and which way the shift goes. put_row16's
// sliding three-byte window covers all three, and its `moveq #-1` equivalent
// (the ones outside the 16-pixel window that the original's rol.l shifts in)
// falls out of only writing the bytes the window reaches.
void GrayClipSprite16_SMASK_R(short x, short y, unsigned short height,
			      const unsigned short *sprt0,
			      const unsigned short *sprt1,
			      const unsigned short *mask, void *dest0, void *dest1)
{
	int first = y < 0 ? -y : 0;		// first sprite row on screen
	int last = height;			// one past the last
	int r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		unsigned wr = (unsigned)(unsigned short)~mask[r];

		put_row16((unsigned char *)dest0, x, (short)(y + r),
			  sprt0[r], wr);
		put_row16((unsigned char *)dest1, x, (short)(y + r),
			  sprt1[r], wr);
	}
}

void GrayClipSprite8_SMASK_R(short x, short y, unsigned short height,
			     const unsigned char *sprt0, const unsigned char *sprt1,
			     const unsigned char *mask, void *dest0, void *dest1)
{
	(void)x; (void)y; (void)height; (void)sprt0; (void)sprt1; (void)mask;
	(void)dest0; (void)dest1;
}

// Pixel-exact collision test between two 16-wide sprites of independent
// heights, one word per row, top-left corners at (x0,y0) and (x1,y1). Returns
// nonzero if any pixel is set in both.
//
// The original spends most of its length juggling registers so that one code
// path serves all four sign combinations of dx and dy; the actual test is two
// instructions. Written out plainly, it is: normalise so sprite 0 is the
// leftmost, intersect the two row ranges, and per row check
//
//	((unsigned long)row0 << dx) & row1
//
// The 32-bit shift is the crux. Treating bit 15 as the leftmost pixel, sprite
// 0's pixel i sits at absolute x0+i and sprite 1's pixel j at x0+dx+j, so they
// coincide when j == i-dx; shifting row0 left by dx lines those bits up.
// Widening to 32 bits first means the pixels of sprite 0 that fall left of
// sprite 1's window move into bits 16..31, where row1 has nothing, and are
// discarded rather than wrapping into a false hit.
short TestCollide162h_R(short x0, short y0, short x1, short y1,
			unsigned short height0, unsigned short height1,
			const unsigned short *data0, const unsigned short *data1)
{
	int dx = x1 - x0;
	int top, bot, r;

	// Normalise to sprite 0 being the leftmost; the test is symmetric.
	if (dx < 0) {
		const unsigned short *dt = data0;
		short t;
		unsigned short ht;

		dx = -dx;
		data0 = data1; data1 = dt;
		t = y0; y0 = y1; y1 = t;
		ht = height0; height0 = height1; height1 = ht;
	}
	if (dx >= 16)
		return 0;	// too far apart horizontally to share a column

	// Intersect the row ranges; bot is exclusive.
	top = y0 > y1 ? y0 : y1;
	bot = (int)y0 + height0 < (int)y1 + height1 ? (int)y0 + height0
						    : (int)y1 + height1;
	if (bot <= top)
		return 0;

	data0 += top - y0;
	data1 += top - y1;
	for (r = 0; r < bot - top; r++)
		if (((unsigned long)data0[r] << dx) & data1[r])
			return 1;
	return 0;
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
