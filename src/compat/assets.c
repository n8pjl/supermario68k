#include "assets.h"
#include "../version.h"

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

// alignas: the tile and sprite files are read through uint16_t pointers (see
// Load_gfx_from_file), and an array of bytes is only byte-aligned. The heap
// copies these arrays replaced came from malloc and were aligned by accident;
// here it has to be said.
alignas(uint16_t) static const uint8_t sm68k_blob[] = {
#embed "data/sm68k.bin"
};

alignas(uint16_t) static const uint8_t common_blob[] = {
#embed "data/common.bin"
};

alignas(uint16_t) static const uint8_t world1_blob[] = {
#embed "data/world1.bin"
};

alignas(uint16_t) static const uint8_t world2_blob[] = {
#embed "data/world2.bin"
};

alignas(uint16_t) static const uint8_t world3_blob[] = {
#embed "data/world3.bin"
};

alignas(uint16_t) static const uint8_t world4_blob[] = {
#embed "data/world4.bin"
};

alignas(uint16_t) static const uint8_t world5_blob[] = {
#embed "data/world5.bin"
};

alignas(uint16_t) static const uint8_t world6_blob[] = {
#embed "data/world6.bin"
};

alignas(uint16_t) static const uint8_t world7_blob[] = {
#embed "data/world7.bin"
};

alignas(uint16_t) static const uint8_t world8_blob[] = {
#embed "data/world8.bin"
};

alignas(uint16_t) static const uint8_t ma_tiles_blob[] = {
#embed "data/ma_tiles.bin"
};

alignas(uint16_t) static const uint8_t ma_sprts_blob[] = {
#embed "data/ma_sprts.bin"
};

alignas(uint16_t) static const uint8_t ma_bckgr_blob[] = {
#embed "data/ma_bckgr.bin"
};

// The shipped files and their tags. tools/mkdata.py reads this list and fails
// the build if the files it converts do not match it, so the two cannot drift:
// an array above that no entry names would only draw an unused-variable
// warning, and a name here that no array backs would not compile at all, but
// neither says anything about what is actually in data/.
#define ASSET_LIST(X)      \
	X(sm68k, "MLST")   \
	X(common, "MLEV")  \
	X(world1, "MLEV")  \
	X(world2, "MLEV")  \
	X(world3, "MLEV")  \
	X(world4, "MLEV")  \
	X(world5, "MLEV")  \
	X(world6, "MLEV")  \
	X(world7, "MLEV")  \
	X(world8, "MLEV")  \
	X(ma_tiles, "GFX") \
	X(ma_sprts, "GFX") \
	X(ma_bckgr, "MBG")

#define ASSET_ENTRY(name, tag) \
	{ #name, tag, name##_blob, sizeof name##_blob, false },

// Not const, unlike everything it points at: an entry's data and size move to
// the heap when a save shadows the file.
static struct asset Assets[] = { ASSET_LIST(ASSET_ENTRY) };

#undef ASSET_ENTRY

#define ASSET_COUNT (sizeof Assets / sizeof *Assets)

// Each save is one localStorage entry holding a JSON record:
//
//   {"version":2,"name":"sm68k","data":"<base64>","saved":"<ISO 8601>"}
//
// SAVE_VERSION (version.h) is what a future format change keys off: a record
// carrying any other version is ignored rather than guessed at, so the game
// falls back to the shipped file instead of parsing a save it cannot
// understand. The payload is a file's contents, so it stays base64 inside the
// record - JSON has no byte string, and localStorage no non-string value. The
// base64 is done in JS with Uint8Array.{to,from}Base64() rather than a codec
// hand-rolled here.
//
// Decoding takes two steps - first return the decoded byte count, then copy the
// bytes into a buffer allocated here - so the C side owns the allocation. The
// decoded Uint8Array is parked on globalThis between the calls, as gray.c does
// for its frame timestamp, so localStorage and the decode each happen once.
static void load_save(struct asset *a)
{
  int len = EM_ASM_INT({
	// clang-format off
		// Everything below throws rather than returns on bad input -
		// a missing entry, a record that is not JSON, a version that
		// is not ours, base64 that does not decode - and the catch
		// turns all of it into the same "no save here" answer.
		try {
			const rec = JSON.parse(
				localStorage.getItem(`sm68k/${UTF8ToString($0)}`));
			if (rec.version !== $1 || typeof rec.data !== "string")
				throw 0;

			const arr = Uint8Array.fromBase64(rec.data);
			globalThis.__sm68kSave = arr;
			return arr.length;
		} catch {
			globalThis.__sm68kSave = null;
			return -1;
		}
// clang-format on
}, a->name, SAVE_VERSION);
if (len < 0)
	return;

uint8_t *save = malloc((size_t)len + 1); // +1: never malloc(0)
if (!save) {
	EM_ASM({ globalThis.__sm68kSave = null; });
	return;
}
EM_ASM(
	{
		HEAPU8.set(globalThis.__sm68kSave, $0);
		globalThis.__sm68kSave = null;
	},
	save);

a->data = save;
a->size = (size_t)len;
a->saved = true;
}

// localStorage is read once per file, on the first lookup of any of them. Only
// the levelset is ever written, but nothing here needs to know that.
static void load_saves(void)
{
	static bool done;

	if (done)
		return;
	done = true;

	for (size_t i = 0; i < ASSET_COUNT; i++)
		load_save(&Assets[i]);
}

struct asset *Asset_find(const char *name)
{
	load_saves();

	for (size_t i = 0; i < ASSET_COUNT; i++)
		if (!strcmp(Assets[i].name, name))
			return &Assets[i];

	return NULL;
}

struct asset *Asset_next_tagged(const struct asset *prev, const char *tag)
{
	load_saves();

	for (size_t i = prev ? (size_t)(prev - Assets) + 1 : 0; i < ASSET_COUNT;
	     i++)
		if (!strcmp(Assets[i].tag, tag))
			return &Assets[i];

	return NULL;
}

bool Asset_save(struct asset *a, const void *data, size_t size)
{
	uint8_t *save = malloc(size + 1); // +1: never malloc(0)

	if (!save)
		return false;
	memcpy(save, data, size);

	// A record that cannot be written is not a failed save: the copy above
	// still shadows the file for the rest of the session, which is what the
	// game shows the player. Only the next session loses it.
  EM_ASM({
	// clang-format off
		try {
			// slice() copies the range out of the wasm heap;
			// toBase64() then encodes that copy.
			const b64 = HEAPU8.slice($1, $1 + $2).toBase64();
			const name = UTF8ToString($0);
			localStorage.setItem(`sm68k/${name}`, JSON.stringify({
				version: $3,
				name: name,
				data: b64,
				saved: new Date().toISOString(),
			}));
		} catch {}
// clang-format on
}, a->name, save, size, SAVE_VERSION);

if (a->saved)
	free((void *)a->data); // ours: the save it is replacing

a->data = save;
a->size = size;
a->saved = true;
return true;
}
