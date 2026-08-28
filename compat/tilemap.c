#include "tilemap.h"
#include "graph.h"

#include <stddef.h>

// Port of the old TileMap engine's plane renderers. The shipped TileMap.a is
// m68k COFF and cannot be linked here, so these are reimplementations; the
// library's original sources served as the reference, and for the routines
// present in both, the objects in the game's TileMap.a are byte-identical to
// the ones built from those sources.
//
// The engine's idea: a map is a matrix of tile numbers, one byte each, `width`
// bytes to a row. Rather than blit tiles to the screen every frame, it renders
// a 272x160 window of the map into a "big virtual screen" and then copies a
// 240x128 window out of that. The buffer is a whole 16x16 tile wider and two
// taller than the screen, so the view can scroll up to 31 pixels in either
// direction before the buffer has to be rebuilt.
//
// Tile rows arrive already byte-swapped - tools/mkdata.py converts the 16-bit
// regions of the GFX blobs at build time - so they can be read natively. The
// big virtual screen is a different matter: like the planes it is a
// byte-addressed bitmap with the leftmost pixel in bit 7 of the first byte, so
// its 16-bit words are assembled and read back a byte at a time. Reading them
// as native shorts would transpose the halves of every word on a
// little-endian target.

// The big virtual screen: 34 bytes (272 pixels) by 160 rows, holding 17x10
// tiles of 16x16. A gray one is two of these back to back, light then dark.
#define VS_STRIDE 34
#define VS_HEIGHT 160
#define VS_SIZE (VS_STRIDE * VS_HEIGHT)
#define VS_COLS 17
#define VS_ROWS 10

// Word k of a buffer row, seen through a window `shift` pixels to the right:
// each output word is stitched from two source words,
//
//	out[k] = (w[k] << shift) | (w[k+1] >> (16 - shift))
//
// which is what the originals' rol.l/mask dance computes - they keep the
// carry-in for the next word in a register instead of re-reading it, and
// unroll all 15 words of a row, but the result is the same. Reading each
// source word twice is the cost of writing it plainly; it stays within one
// cache line and this is not the hot path.
static unsigned vs_word(const unsigned char *row, short k, unsigned shift)
{
	unsigned w0 = ((unsigned)row[k] << 8) | row[k + 1];
	unsigned w1;

	if (!shift)
		return w0;
	w1 = ((unsigned)row[k + 2] << 8) | row[k + 3];
	return ((w0 << shift) | (w1 >> (16 - shift))) & 0xFFFF;
}

// Split the window origin into the source offset and the residual shift the
// stitch above needs. The callers guarantee 0 <= x, y < 32; the whole word of
// horizontal offset is folded into the pointer, leaving 0..15 to shift.
static const unsigned char *vs_window(const void *src, unsigned short x,
				      unsigned short y, unsigned *shift)
{
	const unsigned char *s = (const unsigned char *)src + y * VS_STRIDE;

	*shift = x;
	if (*shift >= 16) {
		s += 2;
		*shift -= 16;
	}
	return s;
}

// Mono buffer blitter used for the foreground mask plane: AND a scrolled
// window of the big virtual screen onto a plane.
//
// The window is the visible screen, so this is both DrawBuffer_MASK and the
// TI-89's DrawBuffer89_MASK: the originals differ only in copying 30 bytes on
// 128 rows against 20 bytes on 100, and both step the destination by a full
// 30-byte plane row either way. Sized from LCD_LINE_BYTES/LCD_HEIGHT, one
// function covers both.
void DrawBuffer_MASK(const void *src, unsigned short x, unsigned short y,
		     void *dest)
{
	unsigned shift;
	const unsigned char *s = vs_window(src, x, y, &shift);
	unsigned char *d = (unsigned char *)dest;
	short row, k;

	for (row = 0; row < LCD_HEIGHT; row++, s += VS_STRIDE,
					      d += PLANE_STRIDE) {
		for (k = 0; k < LCD_LINE_BYTES; k += 2) {
			unsigned w = vs_word(s, k, shift);

			d[k] &= (unsigned char)(w >> 8);
			d[k + 1] &= (unsigned char)w;
		}
	}
}

// Gray buffer blitters: copy a scrolled window of the gray big virtual screen
// onto the two planes, either replacing what is there or ORing onto it. As
// with the mono blitter above, the visible-window sizing makes each of these
// serve as both the 92+/V200 and the TI-89 variant.
//
// The originals take a single destination and reach the dark plane at
// dest+3840, which holds on the calculator because the gray double buffer
// allocates the two planes back to back. Here they take the two planes the
// game's own macros already pass separately, which does not assume that.
static void gray_blit(const void *src, unsigned short x, unsigned short y,
		      void *lightplane, void *darkplane, short combine)
{
	unsigned shift;
	const unsigned char *s = vs_window(src, x, y, &shift);
	unsigned char *dl = (unsigned char *)lightplane;
	unsigned char *dd = (unsigned char *)darkplane;
	short row, k;

	for (row = 0; row < LCD_HEIGHT; row++, s += VS_STRIDE,
					      dl += PLANE_STRIDE,
					      dd += PLANE_STRIDE) {
		for (k = 0; k < LCD_LINE_BYTES; k += 2) {
			unsigned l = vs_word(s, k, shift);
			unsigned d = vs_word(s + VS_SIZE, k, shift);

			if (combine) {
				dl[k] |= (unsigned char)(l >> 8);
				dl[k + 1] |= (unsigned char)l;
				dd[k] |= (unsigned char)(d >> 8);
				dd[k + 1] |= (unsigned char)d;
			} else {
				dl[k] = (unsigned char)(l >> 8);
				dl[k + 1] = (unsigned char)l;
				dd[k] = (unsigned char)(d >> 8);
				dd[k + 1] = (unsigned char)d;
			}
		}
	}
}

void DrawGrayBuffer2B_RPLC(const void *src, unsigned short x, unsigned short y,
			   void *lightplane, void *darkplane)
{
	gray_blit(src, x, y, lightplane, darkplane, 0);
}

void DrawGrayBuffer2B_OR(const void *src, unsigned short x, unsigned short y,
			 void *lightplane, void *darkplane)
{
	gray_blit(src, x, y, lightplane, darkplane, 1);
}

// Render the 17x10 tiles starting at map tile (col0, row0) into the big
// virtual screen.
//
// `anim` is the current step's row of the animation table, or NULL for a plain
// plane: a plain plane's map holds tile numbers, an animated one's holds
// animation numbers that the table turns into tile numbers for this step.
//
// `gray` picks a 32-word tile whose light and dark rows alternate, written to
// both halves of the buffer, over a 16-word mono tile written to one. `wrap`,
// when nonzero, is the map width to take the column index modulo, so the map
// repeats horizontally - see DrawGrayPlane16B2B_ROLL.
//
// The originals walk the buffer in column-major order, which lets them keep
// one running destination pointer and step the map pointer by `width` per row.
// Indexing both directly is the same traversal without the bookkeeping.
static void refresh_buffer16b(const unsigned char *matrix, unsigned short width,
			      short col0, short row0,
			      const unsigned short *sprites,
			      const unsigned short *anim, unsigned char *buf,
			      short wrap, short gray)
{
	short col, row, r;

	for (col = 0; col < VS_COLS; col++) {
		short mc = col0 + col;

		if (wrap)
			mc %= wrap;

		for (row = 0; row < VS_ROWS; row++) {
			unsigned n = matrix[(row0 + row) * width + mc];
			const unsigned short *t;
			unsigned char *p;

			if (anim)
				n = anim[n];
			t = sprites + (gray ? 32 : 16) * n;
			p = buf + row * 16 * VS_STRIDE + col * 2;

			for (r = 0; r < 16; r++, p += VS_STRIDE) {
				unsigned l = gray ? t[2 * r] : t[r];

				p[0] = (unsigned char)(l >> 8);
				p[1] = (unsigned char)l;
				if (gray) {
					unsigned d = t[2 * r + 1];

					p[VS_SIZE] = (unsigned char)(d >> 8);
					p[VS_SIZE + 1] = (unsigned char)d;
				}
			}
		}
	}
}

// The buffer holds the map from tile (2*(x/32), 2*(y/32)) on, so it stays
// valid until the view crosses a 32-pixel cell boundary - the `& 32` tests -
// and the blitters take the leftover 0..31 pixels as their window origin. The
// distance tests catch a jump that skipped over a boundary without changing
// bit 5. `reserved` is the Plane field the engine keeps that last origin in.
static short plane_stale(const Plane *plane, unsigned short x, unsigned short y)
{
	short ox = (short)((unsigned long)plane->reserved >> 16);
	short oy = (short)((unsigned long)plane->reserved & 0xFFFF);
	short dx = (short)(ox - (short)x);
	short dy = (short)(oy - (short)y);

	if (plane->force_update)
		return 1;
	if (dx < 0)
		dx = -dx;
	if (dy < 0)
		dy = -dy;
	return dx >= 32 || dy >= 32 ||
	       ((ox ^ (short)x) & 32) || ((oy ^ (short)y) & 32);
}

static void plane_refresh(Plane *plane, unsigned short x, unsigned short y,
			  const unsigned short *anim, short wrap, short gray)
{
	plane->force_update = 0;
	refresh_buffer16b((const unsigned char *)plane->matrix, plane->width,
			  (short)(2 * (x >> 5)), (short)(2 * (y >> 5)),
			  (const unsigned short *)plane->sprites, anim,
			  (unsigned char *)plane->big_vscreen, wrap, gray);
}

static void plane_remember(Plane *plane, unsigned short x, unsigned short y)
{
	plane->reserved = (long)(((unsigned long)x << 16) | (unsigned short)y);
}

// Advance the animation one frame, and say whether the step changed - which is
// what forces the buffer to be rebuilt, since every animated tile in it now
// resolves to a different tile number.
//
// The frame counter runs up to step_length, then resets and moves to the next
// of nb_step steps, wrapping. Note that the game drives the foreground mask
// through here twice a frame, once per plane, and compensates by giving it
// double the step_length of the foreground it has to stay in step with.
static short plane_animate(AnimatedPlane *plane)
{
	if (++plane->frame != plane->step_length)
		return 0;
	plane->frame = 0;
	if (++plane->step == plane->nb_step)
		plane->step = 0;
	return 1;
}

// The current step's row of the animation table: nb_anim words per step.
static const unsigned short *anim_row(const AnimatedPlane *plane)
{
	return (const unsigned short *)plane->tabanim +
	       (unsigned)plane->nb_anim * (unsigned)plane->step;
}

void DrawGrayPlane16B2B(unsigned short x, unsigned short y, Plane *plane,
			void *lightplane, void *darkplane, TM_GrayMode mode)
{
	if (plane_stale(plane, x, y))
		plane_refresh(plane, x, y, NULL, 0, 1);
	plane_remember(plane, x, y);
	mode(plane->big_vscreen, x & 31, y & 31, lightplane, darkplane);
}

// The game's own scrolling variant of the above, which it uses for the
// background and nothing else. It came from a separate archive
// (drawgrayplane16broll.a, referenced from mario.tpr) that is not part of this
// source drop, so its behaviour is inferred rather than ported: it wraps the
// column index around the map width, letting a background narrower than the
// level repeat as the view scrolls past its right edge.
//
// The evidence is in render.c, where the two `BgX = BgX % (Bg_plane.width*16)`
// clamps that would otherwise be needed are commented out, while the matching
// vertical clamp on BgY is still live - so the horizontal wrap, and only the
// horizontal one, moved into this function.
void DrawGrayPlane16B2B_ROLL(unsigned short x, unsigned short y, Plane *plane,
			     void *lightplane, void *darkplane, TM_GrayMode mode)
{
	if (plane_stale(plane, x, y))
		plane_refresh(plane, x, y, NULL, (short)plane->width, 1);
	plane_remember(plane, x, y);
	mode(plane->big_vscreen, x & 31, y & 31, lightplane, darkplane);
}

// Mono animated plane, used for the foreground mask: the tiles that punch the
// foreground's silhouette out of both planes before the foreground itself is
// ORed in.
//
// An animation step always rebuilds the buffer, so the staleness test only
// gets a look in on the frames that do not step - which is what the original
// does, checking the animation first and branching straight to the refresh.
void DrawAnimatedPlane16B(unsigned short x, unsigned short y,
			  AnimatedPlane *plane, void *dest, TM_Mode mode)
{
	if (plane_animate(plane) || plane_stale(&plane->p, x, y))
		plane_refresh(&plane->p, x, y, anim_row(plane), 0, 0);
	plane_remember(&plane->p, x, y);
	mode(plane->p.big_vscreen, x & 31, y & 31, dest);
}

// Gray animated plane: the foreground layer itself, and the world map.
void DrawGrayAnimatedPlane16B2B(unsigned short x, unsigned short y,
				AnimatedPlane *plane, void *lightplane,
				void *darkplane, TM_GrayMode mode)
{
	if (plane_animate(plane) || plane_stale(&plane->p, x, y))
		plane_refresh(&plane->p, x, y, anim_row(plane), 0, 1);
	plane_remember(&plane->p, x, y);
	mode(plane->p.big_vscreen, x & 31, y & 31, lightplane, darkplane);
}
