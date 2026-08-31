#pragma once

#include <stdint.h>
typedef uint16_t HANDLE;

enum { FALSE, TRUE };
typedef uint16_t BOOL; // TIGCC asmtypes.h: BOOL is 2 bytes, not _Bool

#define min(a, b)                              \
	({                                     \
		typeof(a) _tmp1 = (a);         \
		typeof(b) _tmp2 = (b);         \
		_tmp1 > _tmp2 ? _tmp2 : _tmp1; \
	})
#define max(a, b)                              \
	({                                     \
		typeof(a) _tmp1 = (a);         \
		typeof(b) _tmp2 = (b);         \
		_tmp1 > _tmp2 ? _tmp1 : _tmp2; \
	})

// Read big-endian values that were left unswapped in the data files. Nearly
// everything is swapped at build time by tools/mkdata.py; these are for the
// parts whose position in the file is not knowable there - the level headers
// and the map payload. See Load_level() and Swap_map_payload().
#define be16(v) __builtin_bswap16(v)
#define be32(v) __builtin_bswap32(v)
