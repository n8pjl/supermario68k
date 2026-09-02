#include "extgraph.h"
#include <stdint.h>

#include "graph.h"
#include <string.h>

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

// Set or clear the horizontal run of pixels x1..x2 (inclusive) on row y.
// Endpoints are swapped if they arrive the wrong way round, matching what
// ExtGraph's FastDrawHLine_R does internally.
//
// The ExtGraph originals work a word at a time and unroll to 64-bit writes;
// that is a speed trick for the 68000 and byte-at-a-time is equivalent, so
// this stays bytewise. It is also endian-agnostic, which matters because the
// planes are shared with sprite data read big-endian elsewhere in this file.
static void hspan(uint8_t *plane, int16_t x1, int16_t x2, int16_t y,
		  int16_t set)
{
	uint8_t *row;
	int16_t b, b1, b2, t;
	uint8_t m1, m2, m;

	if (x2 < x1) {
		t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y < 0 || y >= PLANE_HEIGHT || x2 < 0 || x1 > PLANE_WIDTH - 1)
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 > PLANE_WIDTH - 1)
		x2 = PLANE_WIDTH - 1;

	row = plane + (int16_t)(y * PLANE_STRIDE);
	b1 = x1 >> 3;
	b2 = x2 >> 3;
	m1 = (uint8_t)(0xFF >> (x1 & 7));
	m2 = (uint8_t)(0xFF << (7 - (x2 & 7)));

	if (b1 == b2) {
		m = m1 & m2;
		if (set)
			row[b1] |= m;
		else
			row[b1] &= (uint8_t)~m;
		return;
	}

	if (set) {
		row[b1] |= m1;
		for (b = b1 + 1; b < b2; b++)
			row[b] = 0xFF;
		row[b2] |= m2;
	} else {
		row[b1] &= (uint8_t)~m1;
		for (b = b1 + 1; b < b2; b++)
			row[b] = 0x00;
		row[b2] &= (uint8_t)~m2;
	}
}

// Set or clear the filled rectangle (x1,y1)-(x2,y2), corners included. The
// originals swap the corners if they arrive the wrong way round, and clip
// nothing at all - a caller passing an off-screen coordinate scribbles past
// the plane. Here the span helper clips instead, which cannot change the
// result for the on-screen coordinates the game actually passes.
static void filled_rect(void *plane, int16_t x1, int16_t y1, int16_t x2,
			int16_t y2, int16_t set)
{
	int16_t y, t;

	// Only the row order matters here; hspan() orders the columns itself.
	if (y2 < y1) {
		t = y1;
		y1 = y2;
		y2 = t;
	}
	for (y = y1; y <= y2; y++)
		hspan((uint8_t *)plane, x1, x2, y, set);
}

void FastFilledRect_Draw_R(void *plane, uint16_t x1, uint16_t y1, uint16_t x2,
			   uint16_t y2)
{
	filled_rect(plane, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2,
		    1);
}

void FastFilledRect_Erase_R(void *plane, uint16_t x1, uint16_t y1, uint16_t x2,
			    uint16_t y2)
{
	filled_rect(plane, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2,
		    0);
}

// Set or clear the vertical run of pixels y1..y2 (inclusive) in column x.
// Swaps its endpoints, as FastDrawVLine_R does.
static void vspan(uint8_t *plane, int16_t x, int16_t y1, int16_t y2,
		  int16_t set)
{
	uint8_t *p;
	uint8_t m;
	int16_t y, t;

	if (y2 < y1) {
		t = y1;
		y1 = y2;
		y2 = t;
	}
	if (x < 0 || x >= PLANE_WIDTH || y2 < 0 || y1 > PLANE_HEIGHT - 1)
		return;
	if (y1 < 0)
		y1 = 0;
	if (y2 > PLANE_HEIGHT - 1)
		y2 = PLANE_HEIGHT - 1;

	m = (uint8_t)(1 << (7 - (x & 7)));
	p = plane + (int16_t)(y1 * PLANE_STRIDE) + (x >> 3);
	for (y = y1; y <= y2; y++, p += PLANE_STRIDE) {
		if (set)
			*p |= m;
		else
			*p &= (uint8_t)~m;
	}
}

// The four edges of a rectangle in one plane.
static void outline(void *plane, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
		    int16_t set)
{
	uint8_t *pl = (uint8_t *)plane;

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
void GrayFastOutlineRect_R(void *dest0, void *dest1, uint16_t x1, uint16_t y1,
			   uint16_t x2, uint16_t y2, int16_t color)
{
	outline(dest0, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2,
		color & 1);
	outline(dest1, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2,
		color & 2);
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
static void put_row16(uint8_t *plane, int16_t x, int16_t y, uint16_t val,
		      uint16_t wr)
{
	uint32_t v, m;
	int16_t xs, bx, sh, i;

	if (y < 0 || y >= PLANE_HEIGHT || x <= -16 || x >= PLANE_WIDTH)
		return;

	// Bias by two bytes so the shift and remainder stay non-negative for
	// the leftmost clipped case; 16 is a whole number of bytes, so x & 7 is
	// unchanged.
	xs = x + 16;
	bx = (int16_t)(xs >> 3) - 2;
	sh = xs & 7;

	v = (uint32_t)(val & 0xFFFF) << (8 - sh);
	m = (uint32_t)(wr & 0xFFFF) << (8 - sh);

	plane += y * PLANE_STRIDE;
	for (i = 0; i < 3; i++) {
		int16_t b = bx + i;
		uint8_t mb, vb;

		if (b < 0 || b >= PLANE_STRIDE)
			continue;
		mb = (uint8_t)(m >> (16 - 8 * i));
		vb = (uint8_t)(v >> (16 - 8 * i));
		plane[b] = (uint8_t)((plane[b] & ~mb) | vb);
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
void GrayClipISprite16_RPLC_R(int16_t x, int16_t y, uint16_t height,
			      const uint16_t *sprite, void *dest0, void *dest1)
{
	int16_t first = y < 0 ? -y : 0; // first sprite row on screen
	int16_t last = height; // one past the last
	int16_t r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		put_row16((uint8_t *)dest0, x, (int16_t)(y + r), sprite[2 * r],
			  0xFFFF);
		put_row16((uint8_t *)dest1, x, (int16_t)(y + r),
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
void GrayClipSprite16_SMASK_R(int16_t x, int16_t y, uint16_t height,
			      const uint16_t *sprt0, const uint16_t *sprt1,
			      const uint16_t *mask, void *dest0, void *dest1)
{
	int16_t first = y < 0 ? -y : 0; // first sprite row on screen
	int16_t last = height; // one past the last
	int16_t r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		uint16_t wr = (uint16_t)(uint16_t)~mask[r];

		put_row16((uint8_t *)dest0, x, (int16_t)(y + r), sprt0[r], wr);
		put_row16((uint8_t *)dest1, x, (int16_t)(y + r), sprt1[r], wr);
	}
}

// Write the 8-pixel row `val` at absolute pixel (x, y) of a plane, under the
// write-enable mask `wr`, both with pixel 0 in bit 7.
//
// This is put_row16 with the byte parked in the top half of the 16-pixel
// window and the bottom half left write-disabled: those eight pixels then have
// `wr` clear, so put_row16 leaves the plane's bits alone there and the byte
// lands at x..x+7 alone. The clipping falls out too - a byte entirely off the
// left edge is one whose enabled bits all sit in bytes put_row16 skips.
static void put_row8(uint8_t *plane, int16_t x, int16_t y, uint16_t val,
		     uint16_t wr)
{
	put_row16(plane, x, y, (val & 0xFF) << 8, (wr & 0xFF) << 8);
}

// Draw an 8-wide grayscale sprite through a mask, and clip it to the screen.
//
// The 8-wide twin of GrayClipSprite16_SMASK_R: one byte per row in each of
// sprt0, sprt1 and the shared mask, same conventions - the planes live in
// separate arrays, and a set mask bit means "keep the background", so it is
// complemented into a write enable.
//
// The original has a fourth hand-written case the 16-wide one does not, since
// eight pixels at a bit offset of 0..7 may or may not fit inside the one word
// it reads; put_row8's window makes that distinction disappear as well.
void GrayClipSprite8_SMASK_R(int16_t x, int16_t y, uint16_t height,
			     const uint8_t *sprt0, const uint8_t *sprt1,
			     const uint8_t *mask, void *dest0, void *dest1)
{
	int16_t first = y < 0 ? -y : 0; // first sprite row on screen
	int16_t last = height; // one past the last
	int16_t r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		uint16_t wr = (uint16_t)(uint8_t)~mask[r];

		put_row8((uint8_t *)dest0, x, (int16_t)(y + r), sprt0[r], wr);
		put_row8((uint8_t *)dest1, x, (int16_t)(y + r), sprt1[r], wr);
	}
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
int16_t TestCollide162h_R(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
			  uint16_t height0, uint16_t height1,
			  const uint16_t *data0, const uint16_t *data1)
{
	int16_t dx = x1 - x0;
	int16_t top, bot, r;

	// Normalise to sprite 0 being the leftmost; the test is symmetric.
	if (dx < 0) {
		const uint16_t *dt = data0;
		int16_t t;
		uint16_t ht;

		dx = -dx;
		data0 = data1;
		data1 = dt;
		t = y0;
		y0 = y1;
		y1 = t;
		ht = height0;
		height0 = height1;
		height1 = ht;
	}
	if (dx >= 16)
		return 0; // too far apart horizontally to share a column

	// Intersect the row ranges; bot is exclusive.
	top = y0 > y1 ? y0 : y1;
	bot = (int16_t)y0 + height0 < (int16_t)y1 + height1 ?
		      (int16_t)y0 + height0 :
		      (int16_t)y1 + height1;
	if (bot <= top)
		return 0;

	data0 += top - y0;
	data1 += top - y1;
	for (r = 0; r < bot - top; r++)
		if (((uint32_t)data0[r] << dx) & data1[r])
			return 1;
	return 0;
}

void GrayClearScreen2B(void *lightplane, void *darkplane)
{
	memset(lightplane, 0, LCD_SIZE);
	memset(darkplane, 0, LCD_SIZE);
}

// Draw a 16-wide grayscale sprite flipped top to bottom, through a mask, and
// clip it to the screen.
//
// MASK rather than SMASK: each plane carries its own mask, mask0 with sprt0
// and mask1 with sprt1, though the game passes the same array for both. The
// sense is unchanged - a set mask bit keeps the background - and so is the
// per-row operation, dest = (dest & mask) | sprite.
//
// The flip is all in the indexing: screen row y+k takes sprite row
// height-1-k, mask included, which is what the original gets by pointing all
// four arrays one row past their ends and walking them backwards with
// pre-decrements. The vertical clip then still trims the same range of k, and
// a sprite hanging off the top of the screen loses its *last* rows.
void UpsideDownGrayClipSprite16_MASK_R(int16_t x, int16_t y, uint16_t height,
				       const uint16_t *sprt0,
				       const uint16_t *sprt1,
				       const uint16_t *mask0,
				       const uint16_t *mask1, void *dest0,
				       void *dest1)
{
	int16_t first = y < 0 ? -y : 0; // first row slot on screen
	int16_t last = height; // one past the last
	int16_t k;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (k = first; k < last; k++) {
		int16_t r = height - 1 - k; // the sprite row that lands there

		put_row16((uint8_t *)dest0, x, (int16_t)(y + k), sprt0[r],
			  (uint16_t)(uint16_t)~mask0[r]);
		put_row16((uint8_t *)dest1, x, (int16_t)(y + k), sprt1[r],
			  (uint16_t)(uint16_t)~mask1[r]);
	}
}

// Game-local routine (see Mario/GrayClipSprite16_SMASKBLIT_R.s): a SMASK blit
// restricted to the columns `maskval` selects. The game uses it to reveal Mario
// a slice at a time - Player.Blit is the window - so a zero maskval draws
// nothing and 0xFFFF makes it an ordinary GrayClipSprite16_SMASK_R.
//
// Per row and plane the original is the SMASK operation with maskval folded in
// twice, once each way round:
//
//	dest = (dest & (mask | ~maskval)) | (sprite & maskval)
//
// Complementing the effective mask into a write enable turns the first half
// into ~mask & maskval - write only where the sprite's own mask allows it and
// maskval selects the column - which is the AND of the two write enables, as
// one would hope. The second half is not redundant with it, mind: put_row16
// ORs `val` in over the enable, so a sprite bit outside the blit window would
// still light a pixel if it were not cleared here. Same as the note on
// put_row16 about bits outside a sprite's own mask - the original clears these
// with an `and.l`, so the port does too.
void GrayClipSprite16_SMASKBLIT_R(int16_t x, int16_t y, uint16_t height,
				  const uint16_t *sprt0, const uint16_t *sprt1,
				  const uint16_t *mask, const uint16_t maskval,
				  void *dest0, void *dest1)
{
	int16_t first = y < 0 ? -y : 0; // first sprite row on screen
	int16_t last = height; // one past the last
	int16_t r;

	if (last > PLANE_HEIGHT - y)
		last = PLANE_HEIGHT - y;

	for (r = first; r < last; r++) {
		uint16_t wr = (uint16_t)(uint16_t)~mask[r] & maskval;

		put_row16((uint8_t *)dest0, x, (int16_t)(y + r),
			  sprt0[r] & maskval, wr);
		put_row16((uint8_t *)dest1, x, (int16_t)(y + r),
			  sprt1[r] & maskval, wr);
	}
}
