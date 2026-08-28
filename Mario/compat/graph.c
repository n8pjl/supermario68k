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

#define PLANE_STRIDE 30

// F_4x6 is proportional - each character carries its own advance - while the
// other two faces are fixed-width.
static const unsigned char *font_glyph(short font, unsigned char c, short *h,
				       short *advance)
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

static void blit_glyph(unsigned char *plane, short x, short y,
		       const unsigned char *g, short h, short attr)
{
	for (short r = 0; r < h; r++) {
		short py = y + r;
		if (py < 0 || py >= LCD_HEIGHT)
			continue;
		// Place the 8-bit glyph row into a 16-bit window so a glyph
		// straddling a byte boundary writes both bytes.
		unsigned short win = (unsigned short)g[r] << (8 - (x & 7));
		short col = x >> 3;
		for (short b = 0; b < 2; b++) {
			short cc = col + b;
			if (cc < 0 || cc >= PLANE_STRIDE)
				continue;
			unsigned char bits = (win >> (8 - 8 * b)) & 0xFF;
			unsigned char *p = plane + py * PLANE_STRIDE + cc;
			switch (attr) {
			case A_REVERSE:
				*p &= ~bits;
				break;
			case A_XOR:
				*p ^= bits;
				break;
			default:
				*p |= bits;
				break;
			}
		}
	}
}

static void draw_str_plane(void *plane, short x, short y, const char *s,
			   short attr, short font)
{
	short h, adv;
	for (; *s; s++) {
		const unsigned char *g = font_glyph(font, (unsigned char)*s, &h, &adv);
		blit_glyph((unsigned char *)plane, x, y, g, h, attr);
		x += adv;
	}
}

static short str_width(const char *s, short font)
{
	short h, adv, w = 0;
	for (; *s; s++) {
		font_glyph(font, (unsigned char)*s, &h, &adv);
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
void DrawStr(short x, short y, const char *s, short attr)
{
	draw_str_plane(GrayDBufGetHiddenPlane(LIGHT_PLANE), x, y, s, attr, F_6x8);
	draw_str_plane(GrayDBufGetHiddenPlane(DARK_PLANE), x, y, s, attr, F_6x8);
}

void DrawChar(short x, short y, char c, short attr)
{
	char s[2] = { c, 0 };
	DrawStr(x, y, s, attr);
}

// Mirrors ExtGraph's GrayDrawStrExt2B: draw into both planes, honouring the
// A_CENTERED and A_SHADOWED extended attributes.
void GrayDrawStrExt2B(unsigned short x, unsigned short y, const char *s,
		      short attr, short font, void *lightplane, void *darkplane)
{
	short shadow = attr & A_SHADOWED;

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
