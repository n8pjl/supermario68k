#pragma once

typedef unsigned short HANDLE;

enum { FALSE, TRUE };
typedef unsigned short BOOL;	// TIGCC asmtypes.h: BOOL is 2 bytes, not _Bool

#define min(a, b) ({ typeof(a) _tmp1 = (a); typeof(b) _tmp2 = (b); _tmp1 > _tmp2 ? _tmp2 : _tmp1; })
#define max(a, b) ({ typeof(a) _tmp1 = (a); typeof(b) _tmp2 = (b); _tmp1 > _tmp2 ? _tmp1 : _tmp2; })

// Read a big-endian 16-bit value that was left unswapped in the data files.
// Nearly everything is swapped at build time by tools/mkdata.py; this is for
// the one structure whose position in the file is not knowable there. See
// Load_level().
#define be16(v) __builtin_bswap16(v)
