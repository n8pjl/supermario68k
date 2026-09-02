#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The game's data files, compiled in.
//
// On the calculator the game read its data straight out of VAT variables:
// SymFindPtr located a SYM_ENTRY, HeapLock pinned its memory block and
// HeapDeref handed back a void* to it. This port used to reproduce that shape
// on top of an Emscripten MEMFS image - a shim opened the file, copied it onto
// the heap and returned an untyped pointer, and every caller did its own
// arithmetic from there.
//
// Nothing about that survives here. The set of files is fixed and known at
// build time, so each one is #embed-ed as a const array and described by a
// struct asset; a caller asks for an asset by name and gets typed, sized,
// read-only bytes.
//
// tools/mkdata.py produces the embedded files: it strips the .9xy container
// and byte-swaps everything the m68k original read as a 16-bit word. What is
// left is the on-calc variable block's contents, which still ends in the
// {0, tag..., 0, 0xF8} block that identified the variable's type on the
// calculator. The tag is recorded in the table instead, so looking for a
// levelset is a field comparison rather than a scan backwards from the end of
// the data.
//
// Saves are the one thing here that is not read-only: a save is written to
// localStorage and shadows the file it was written from, for the rest of the
// session and every session after it.

struct asset {
	const char *name;
	const char *tag; // MLST levelset, MLEV levels, GFX tiles/sprites, MBG
	const uint8_t *data; // the save, if there is one, else the shipped file
	size_t size;
	bool saved; // data is a save on the heap, not the embedded array
};

// Look up one asset by name, or NULL if there is no such file.
struct asset *Asset_find(const char *name);

// Walk the assets carrying `tag`: pass NULL to start, then the previous result.
struct asset *Asset_next_tagged(const struct asset *prev, const char *tag);

// Replace an asset's contents with a copy of `size` bytes from `data`, and
// persist it. False if the copy could not be made, leaving the asset as it was.
bool Asset_save(struct asset *a, const void *data, size_t size);
