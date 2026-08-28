#pragma once

#include <stdint.h>
// TIOS bitmap fonts lifted from an AMS OS image by tools/mkfont.py.
// Each glyph is one byte per row, left-aligned: pixel 0 is bit 7.
extern const uint8_t Font6x8[256][8];
extern const uint8_t Font8x10[256][10];

// The 4x6 face is proportional: Font4x6Width is the per-character advance in
// pixels (glyph width plus one column of spacing). Row 5 of every cell is the
// blank line gap.
extern const uint8_t Font4x6[256][6];
extern const uint8_t Font4x6Width[256];
