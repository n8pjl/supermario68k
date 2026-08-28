#include "graph.h"
#include "gray.h"

#include <string.h>

// Plain-plane text and contrast. On the calculator these are TIOS ROM calls.
// The bitmap fonts they render with live in the calculator's ROM and are not
// part of this source drop, so DrawStr/DrawChar cannot draw until a substitute
// font is supplied. They are stubbed rather than approximated so that missing
// text is obvious on screen instead of silently wrong.
// TODO: supply a 4x6 / 6x8 / 8x10 substitute font.

void ClrScr(void)
{
	memset(GrayDBufGetHiddenPlane(LIGHT_PLANE), 0, LCD_SIZE);
	memset(GrayDBufGetHiddenPlane(DARK_PLANE), 0, LCD_SIZE);
}

void DrawStr(short x, short y, const char *s, short attr)
{
	(void)x; (void)y; (void)s; (void)attr;
}

void DrawChar(short x, short y, char c, short attr)
{
	(void)x; (void)y; (void)c; (void)attr;
}

void OSContrastUp(void) {}
void OSContrastDn(void) {}
