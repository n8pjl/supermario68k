#pragma once

// Screen geometry. The drawing surface is the same on every model: 240x128
// one-bit pixels, 30 bytes to a row, 3840 bytes a plane. What differs is how
// much of it the LCD shows - the TI-89 displays only the top-left 160x100, the
// rest of each row being there but off screen. GCC4TI says as much: its
// LCD_LINE_BYTES is documented as "the visible part of a screen line", 20 on
// the 89 against a 30-byte line.
//
// That split is why the TileMap engine has separate "89" blitters, which copy
// a 160x100 window out of the same big virtual screen and still step the
// destination by 30 bytes a row, and why the sprite routines clip against
// 240x128 on every model - there is only one set of them. The game's own
// statusbar_offset agrees: 2760 on the 89 is row 92 of 30 bytes, putting an
// 8-pixel status bar at the bottom of a 100-row screen.
//
// So: PLANE_* for anything addressing a plane, LCD_* for what the user sees.
#define PLANE_WIDTH 240
#define PLANE_HEIGHT 128
#define PLANE_STRIDE (PLANE_WIDTH / 8)
#define LCD_SIZE (PLANE_STRIDE * PLANE_HEIGHT)

#if defined(PRODUCE_TI89_CODE)
#define LCD_WIDTH 160
#define LCD_HEIGHT 100
#elif defined(PRODUCE_TI92PLUS_CODE) || defined(PRODUCE_V200_CODE)
#define LCD_WIDTH 240
#define LCD_HEIGHT 128
#else
#error "no target calculator - build with make CALC=89|92p|v200"
#endif

// Bytes of a plane row that are actually on screen.
#define LCD_LINE_BYTES (LCD_WIDTH / 8)

enum Fonts {F_4x6, F_6x8, F_8x10};
enum Attrs {A_REVERSE, A_NORMAL, A_XOR, A_SHADED, A_REPLACE, A_OR, A_AND, A_THICK1, A_SHADE_V, A_SHADE_H, A_SHADE_NS, A_SHADE_PS};

// Plain (non-gray) text output, used only by the error screen. On the
// calculator these are TIOS ROM calls that render with the built-in bitmap
// fonts; those fonts are ROM data and are not part of this source drop, so a
// substitute font has to be supplied before they can draw anything.
void ClrScr(void);
void DrawStr(short x, short y, const char *s, short attr);
void DrawChar(short x, short y, char c, short attr);

// Contrast has no meaning on a canvas; kept so the key handler still links.
void OSContrastUp(void);
void OSContrastDn(void);
