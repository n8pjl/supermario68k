#pragma once

#define LCD_SIZE 3840

enum Fonts {F_4x6, F_6x8, F_8x10};
enum Attrs {A_REVERSE, A_NORMAL, A_XOR, A_SHADED, A_REPLACE, A_OR, A_AND, A_THICK1, A_SHADE_V, A_SHADE_H, A_SHADE_NS, A_SHADE_PS};

#ifndef LCD_WIDTH
#define LCD_WIDTH 240
#endif
#ifndef LCD_HEIGHT
#define LCD_HEIGHT 128
#endif

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
