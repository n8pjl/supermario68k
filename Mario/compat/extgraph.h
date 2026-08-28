#pragma once

#include "../../ExtGraph/lib/extgraph.h"

// ExtGraph's pixel macros expand to m68k `bset`/`bclr` inline asm, whose "di"
// operand constraints clang rejects. The operations themselves are plain bit
// twiddling, so redefine them in C using ExtGraph's own address/mask helpers.
//
//   EXT_PIXOFFSET(x,y) == y*30 + x/8   (240-pixel-wide plane, 30 bytes a row)
//   EXT_PIXNBIT(x)     == ~x, of which `bset` uses only the low 3 bits,
//                         i.e. bit 7-(x&7): pixel 0 is the most significant.

#undef EXT_SETPIX_AN
#undef EXT_CLRPIX_AN
#undef EXT_XORPIX_AN
#undef EXT_GETPIX_AN

#define EXT_PIXMASK_(x) ((unsigned char)(1 << (~(x) & 7)))
#define EXT_PIXBYTE_(p, offset) (((unsigned char *)(p))[(offset)])

#define EXT_SETPIX_AN(p, offset, n) (EXT_PIXBYTE_(p, offset) |= (unsigned char)(1 << ((n) & 7)))
#define EXT_CLRPIX_AN(p, offset, n) (EXT_PIXBYTE_(p, offset) &= (unsigned char)~(1 << ((n) & 7)))
#define EXT_XORPIX_AN(p, offset, n) (EXT_PIXBYTE_(p, offset) ^= (unsigned char)(1 << ((n) & 7)))
#define EXT_GETPIX_AN(p, offset, n) (EXT_PIXBYTE_(p, offset) & (unsigned char)(1 << ((n) & 7)))
