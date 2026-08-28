#pragma once

typedef unsigned short HANDLE;

enum { FALSE, TRUE };
typedef unsigned short BOOL;	// TIGCC asmtypes.h: BOOL is 2 bytes, not _Bool

#define min(a, b) ({ typeof(a) _tmp1 = (a); typeof(b) _tmp2 = (b); _tmp1 > _tmp2 ? _tmp2 : _tmp1; })
#define max(a, b) ({ typeof(a) _tmp1 = (a); typeof(b) _tmp2 = (b); _tmp1 > _tmp2 ? _tmp1 : _tmp2; })