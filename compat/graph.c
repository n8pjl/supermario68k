#include <stdint.h>
#include "graph.h"
#include "gray.h"
#include "font_data.h"
#include "extgraph.h"	// A_CENTERED / A_SHADOWED

#include <string.h>

// Text output for the plain and gray planes.
//
// On the calculator these are TIOS ROM calls rendering with the built-in
// bitmap fonts. Those fonts are ROM data rather than part of this source drop,
// so tools/mkfont.py lifts them out of an AMS image into font_data.c and the
// blitting below is a reimplementation.
//
// Planes are 240x128, one bit per pixel, 30 bytes per row, pixel 0 in bit 7.


// F_4x6 is proportional - each character carries its own advance - while the
// other two faces are fixed-width.
static const uint8_t *font_glyph(int16_t font, uint8_t c, int16_t *h,
				       int16_t *advance)
{
	if (font == F_8x10) {
		*h = 10;
		*advance = 8;
		return Font8x10[c];
	}
	if (font == F_6x8) {
		*h = 8;
		*advance = 6;
		return Font6x8[c];
	}
	*h = 6;
	*advance = Font4x6Width[c];
	return Font4x6[c];
}

// Draw one glyph. `adv` is the width of its character cell - what the string
// advances by - which is the glyph's own width for the fixed faces and the
// per-character advance for the proportional 4x6 one.
//
// The cell is what separates A_REPLACE from A_NORMAL. A_NORMAL ORs the ink and
// leaves everything else as it found it; A_REPLACE writes the whole cell, so
// the background inside it is cleared whether or not the glyph has ink there.
// Text drawn over something therefore erases what it covers, which is what
// most of the game's text relies on: the status bar redraws the coin and score
// counters in place, and Draw_time() blanks its field by drawing a run of
// spaces in A_REPLACE, which does nothing at all if the attribute only ORs.
//
// For the 4x6 face the cell includes the blank spacing column the advance
// carries past the ink, and its blank sixth row - so a replaced line of text
// clears the gaps between its characters too, as the ROM's DrawStr does.
static void blit_glyph(uint8_t *plane, int16_t x, int16_t y,
		       const uint8_t *g, int16_t h, int16_t adv, int16_t attr)
{
	// The glyph is one byte wide, so the cell cannot usefully be wider.
	int16_t cw = adv > 8 ? 8 : adv;
	uint8_t cell = cw > 0 ? (uint8_t)(0xFF << (8 - cw)) : 0;

	for (int16_t r = 0; r < h; r++) {
		int16_t py = y + r;
		if (py < 0 || py >= PLANE_HEIGHT)
			continue;
		// Place the 8-bit glyph row into a 16-bit window so a glyph
		// straddling a byte boundary writes both bytes.
		uint16_t win = (uint16_t)g[r] << (8 - (x & 7));
		uint16_t cellwin = (uint16_t)cell << (8 - (x & 7));
		int16_t col = x >> 3;
		for (int16_t b = 0; b < 2; b++) {
			int16_t cc = col + b;
			if (cc < 0 || cc >= PLANE_STRIDE)
				continue;
			uint8_t bits = (win >> (8 - 8 * b)) & 0xFF;
			uint8_t cbits = (cellwin >> (8 - 8 * b)) & 0xFF;
			uint8_t *p = plane + py * PLANE_STRIDE + cc;
			switch (attr) {
			case A_REVERSE:
				*p &= ~bits;
				break;
			case A_XOR:
				*p ^= bits;
				break;
			case A_REPLACE:
				*p = (uint8_t)((*p & ~cbits) | bits);
				break;
			default:
				*p |= bits;
				break;
			}
		}
	}
}

static void draw_str_plane(void *plane, int16_t x, int16_t y, const char *s,
			   int16_t attr, int16_t font)
{
	int16_t h, adv;
	for (; *s; s++) {
		const uint8_t *g = font_glyph(font, (uint8_t)*s, &h, &adv);
		blit_glyph((uint8_t *)plane, x, y, g, h, adv, attr);
		x += adv;
	}
}

static int16_t str_width(const char *s, int16_t font)
{
	int16_t h, adv, w = 0;
	for (; *s; s++) {
		font_glyph(font, (uint8_t)*s, &h, &adv);
		w += adv;
	}
	return w;
}

void ClrScr(void)
{
	memset(GrayDBufGetHiddenPlane(LIGHT_PLANE), 0, LCD_SIZE);
	memset(GrayDBufGetHiddenPlane(DARK_PLANE), 0, LCD_SIZE);
}

// The error screen draws with the plain-plane calls; render to both planes so
// the result reads as solid black rather than a gray shade.
void DrawStr(int16_t x, int16_t y, const char *s, int16_t attr)
{
	draw_str_plane(GrayDBufGetHiddenPlane(LIGHT_PLANE), x, y, s, attr, F_6x8);
	draw_str_plane(GrayDBufGetHiddenPlane(DARK_PLANE), x, y, s, attr, F_6x8);
}

void DrawChar(int16_t x, int16_t y, char c, int16_t attr)
{
	char s[2] = { c, 0 };
	DrawStr(x, y, s, attr);
}

// Mirrors ExtGraph's GrayDrawStrExt2B: draw into both planes, honouring the
// A_CENTERED and A_SHADOWED extended attributes.
void GrayDrawStrExt2B(uint16_t x, uint16_t y, const char *s,
		      int16_t attr, int16_t font, void *lightplane, void *darkplane)
{
	int16_t shadow = attr & A_SHADOWED;

	if (attr & A_CENTERED)
		x = (LCD_WIDTH - str_width(s, font)) / 2;
	attr &= ~(A_CENTERED | A_SHADOWED);

	draw_str_plane(darkplane, x, y, s, attr, font);
	draw_str_plane(lightplane, x, y, s, attr, font);

	if (shadow)
		draw_str_plane(lightplane, x + 1, y + 1, s, A_NORMAL, font);
}

void OSContrastUp(void) {}
void OSContrastDn(void) {}
