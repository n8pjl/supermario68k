#pragma once

// Shim for the slice of the TIOS variable/file API that Super Mario 68K uses.
//
// On the calculator the game reads its data straight out of VAT variables:
// SymFindPtr locates a SYM_ENTRY, HeapLock pins its memory block, and
// HeapDeref hands back a pointer whose first two bytes are the block size -
// hence the "+2" at every call site. Here the same shape is served from the
// preprocessed blobs in data/, which are byte-identical to those blocks, so
// none of the game's parsing had to change.
//
// Writes (savegames) buffer in memory and are flushed to localStorage on
// FClose, keyed by variable name. A file that has been saved shadows the
// shipped blob on subsequent reads.

#include "utils.h"
#include <stddef.h>

#define H_NULL 0
#define HS_NULL 0

#define OTH_TAG 0xF8
#define FO_RECURSE 0x02

enum fileMode { FM_CLOSED = 0, FM_READ = 1, FM_WRITE = 2, FM_APPEND = 3 };
enum FileStatusEnum {
	FS_OK = 0x0000,
	FS_EOF = 0xFFFF,
	FS_ERROR = 0xFFFE,
	FS_BAD_NAME = 0xFFFD,
	FS_MEMORY = 0xFFFC,
	FS_NOT_FOUND = 0xFFFB
};

// Field layout mirrors TIGCC's vat.h; only `name` and `handle` are ever read
// by the game, but the rest is kept so the struct stays recognisable.
typedef struct {
	char name[8];
	unsigned short compat;
	unsigned short flags;
	HANDLE handle;
} SYM_ENTRY;

typedef const char *SYM_STR;

// The game passes C strings; the calculator's reversed/NUL-prefixed SYM_STR
// encoding buys us nothing here, so SYMSTR is the identity.
#define SYMSTR(s) ((SYM_STR)(s))

typedef struct {
	char name[9];
	char tag[8];
	unsigned char *buf;
	size_t len;
	size_t cap;
	short mode;
} FILES;

SYM_ENTRY *SymFindPtr(SYM_STR name, short flags);
void *HeapDeref(HANDLE handle);
HANDLE HeapLock(HANDLE handle);
void *HToESI(HANDLE handle);

char *SymFindFolderName(void);
BOOL FolderCur(SYM_STR name, BOOL create);
SYM_ENTRY *FFindFirst(short flags, const char *tag, const char *folder);
SYM_ENTRY *FFindNext(void);

unsigned short FOpen(const char *name, FILES *f, short mode, const char *tag);
unsigned short FWrite(const void *data, short len, FILES *f);
unsigned short FClose(FILES *f);

// Archive memory management: there is no archive here, so these all succeed.
short EM_findEmptySlot(long size);
short EM_moveSymFromExtMem(SYM_STR name, HANDLE h);
short EM_moveSymToExtMem(SYM_STR name, HANDLE h);
short EM_GC(BOOL all);
