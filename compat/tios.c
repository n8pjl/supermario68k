#include "tios.h"
#include <stdint.h>

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 32
#define DATA_DIR "/data"
struct var_t {
	char name[9]; // bare variable name, no folder
	uint8_t *data; // [2-byte BE size][content], as HeapDeref returns
	size_t size;
	int16_t loaded; // 0 = slot unused, -1 = known missing
};

static struct var_t Vars[MAX_VARS];
static int16_t Var_count;

// The game addresses variables as "folder\name"; we only care about the name.
static const char *basename_of(const char *path)
{
	const char *p = strrchr(path, '\\');
	return p ? p + 1 : path;
}

static struct var_t *find_slot(const char *name)
{
	for (int16_t i = 0; i < Var_count; i++)
		if (!strcmp(Vars[i].name, name))
			return &Vars[i];
	return NULL;
}

// A save written this session shadows the shipped blob. It is kept base64 so it
// survives localStorage's string-only values; the base64 itself is done in JS
// with Uint8Array.{to,from}Base64() rather than a codec hand-rolled here.
static uint8_t *load_from_storage(const char *name, size_t *out_len)
{
	// Decode in JS in two steps - first return the decoded byte count, then
	// copy the bytes into a buffer allocated here - so the C side owns the
	// allocation. The decoded Uint8Array is parked on globalThis between the
	// calls, as gray.c does for its frame timestamp, so localStorage and the
	// decode each happen once.

  int len = EM_ASM_INT({
  // clang-format off
		try {
			const v = localStorage.getItem(`sm68k/${UTF8ToString($0)}`);
			if (!v) {
				globalThis.__sm68kSave = null;
				return -1;
			}
			const arr = Uint8Array.fromBase64(v);
			globalThis.__sm68kSave = arr;
			return arr.length;
		} catch (e) {
			globalThis.__sm68kSave = null;
			return -1;
		}
// clang-format on
}, name);
if (len < 0)
	return NULL;

uint8_t *out = malloc((size_t)len + 1); // +1: never malloc(0)
if (!out) {
	EM_ASM({
		// clang-format off
			globalThis.__sm68kSave = null;
		// clang-format on
	});
	return NULL;
}
EM_ASM(
	{
		// clang-format off
		HEAPU8.set(globalThis.__sm68kSave, $0);
		globalThis.__sm68kSave = null;
		// clang-format on
	},
	out);

*out_len = (size_t)len;
return out;
}

static void save_to_storage(const char *name, const uint8_t *data, size_t len)
{
  EM_ASM({
			// clang-format off
		try {
			// slice() copies the range out of the wasm heap;
			// toBase64() then encodes that copy.
			const b64 = HEAPU8.slice($1, $1 + $2).toBase64();
			localStorage.setItem(`sm68k/${UTF8ToString($0)}`, b64);
		} catch {}
// clang-format on
}, name, data, len);
}

// Pull a variable into memory: a localStorage save if one exists, otherwise the
// shipped blob preloaded into MEMFS at /data.
static struct var_t *load_var(const char *path)
{
	const char *name = basename_of(path);
	struct var_t *v = find_slot(name);
	if (v)
		return v->loaded > 0 ? v : NULL;

	if (Var_count >= MAX_VARS)
		return NULL;
	v = &Vars[Var_count++];
	snprintf(v->name, sizeof v->name, "%s", name);

	size_t len = 0;
	uint8_t *data = load_from_storage(name, &len);

	if (!data) {
		char p[64];
		snprintf(p, sizeof p, DATA_DIR "/%s.bin", name);
		FILE *f = fopen(p, "rb");
		if (!f) {
			v->loaded = -1;
			return NULL;
		}
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		data = malloc(len);
		if (fread(data, 1, len, f) != len) {
			free(data);
			fclose(f);
			v->loaded = -1;
			return NULL;
		}
		fclose(f);
	}

	v->data = data;
	v->size = len;
	v->loaded = 1;
	return v;
}

// Handles are 1-based indices into Vars, so 0 doubles as H_NULL.
SYM_ENTRY *SymFindPtr(SYM_STR name, int16_t flags)
{
	(void)flags;
	struct var_t *v = load_var((const char *)name);
	if (!v)
		return NULL;

	static SYM_ENTRY sym;
	memset(&sym, 0, sizeof sym);
	snprintf(sym.name, sizeof sym.name + 1, "%s", v->name);
	sym.handle = (HANDLE)((v - Vars) + 1);
	return &sym;
}

void *HeapDeref(HANDLE handle)
{
	if (!handle || handle > Var_count)
		return NULL;
	return Vars[handle - 1].data;
}

HANDLE HeapLock(HANDLE handle)
{
	return handle; // nothing can move underneath us here
}

// TIOS returns a pointer to the variable's last byte, which is the OTH tag.
// menus.c relies on HToESI(h)-6 landing on {0,'M','L','S','T',0,OTH_TAG}.
void *HToESI(HANDLE handle)
{
	if (!handle || handle > Var_count)
		return NULL;
	struct var_t *v = &Vars[handle - 1];
	return v->data + v->size - 1;
}

char *SymFindFolderName(void)
{
	static char folder[] = "mario";
	return folder;
}

BOOL FolderCur(SYM_STR name, BOOL create)
{
	(void)name;
	(void)create;
	return TRUE; // single flat namespace
}

// Levelset enumeration. The game asks for every variable carrying a given OTH
// tag; we scan the shipped blobs and match on the tag block at the tail.
static const char *Find_tag;
static int16_t Find_pos;

static int16_t has_tag(struct var_t *v, const char *tag)
{
	size_t tl = strlen(tag);
	if (v->size < tl + 3)
		return 0;
	const uint8_t *p = v->data + v->size - (tl + 3);
	return p[0] == 0 && !memcmp(p + 1, tag, tl) && p[tl + 1] == 0 &&
	       p[tl + 2] == OTH_TAG;
}

// Names of the blobs shipped in data/; MEMFS has no cheap directory scan here
// and the set is fixed, so enumerate it directly.
static const char *const Known[] = { "sm68k",  "common", "world1", "world2",
				     "world3", "world4", "world5", "world6",
				     "world7", "world8", NULL };

SYM_ENTRY *FFindFirst(int16_t flags, const char *tag, const char *folder)
{
	(void)flags;
	(void)folder;
	Find_tag = tag;
	Find_pos = 0;
	return FFindNext();
}

SYM_ENTRY *FFindNext(void)
{
	while (Known[Find_pos]) {
		const char *name = Known[Find_pos++];
		struct var_t *v = load_var(name);
		if (v && has_tag(v, Find_tag))
			return SymFindPtr(SYMSTR(name), 0);
	}
	return NULL;
}

uint16_t FOpen(const char *name, FILES *f, int16_t mode, const char *tag)
{
	if (mode != FM_WRITE)
		return FS_ERROR;
	memset(f, 0, sizeof *f);
	snprintf(f->name, sizeof f->name, "%s", basename_of(name));
	snprintf(f->tag, sizeof f->tag, "%s", tag);
	f->cap = 4096;
	f->buf = malloc(f->cap);
	if (!f->buf)
		return FS_MEMORY;
	f->mode = mode;
	return FS_OK;
}

uint16_t FWrite(const void *data, int16_t len, FILES *f)
{
	if (f->mode != FM_WRITE)
		return FS_ERROR;
	size_t need = f->len + (size_t)(uint16_t)len;
	if (need > f->cap) {
		while (f->cap < need)
			f->cap *= 2;
		uint8_t *b = realloc(f->buf, f->cap);
		if (!b)
			return FS_MEMORY;
		f->buf = b;
	}
	memcpy(f->buf + f->len, data, (uint16_t)len);
	f->len = need;
	return FS_OK;
}

// Rebuild the on-calc variable block: [2-byte BE size][payload][tag block],
// where the tag block is {0, tag..., 0, OTH_TAG} - the layout observed in every
// shipped blob. The result replaces the cached copy so a save is visible to the
// very next read, and is persisted to localStorage.
uint16_t FClose(FILES *f)
{
	if (f->mode != FM_WRITE)
		return FS_ERROR;

	size_t tl = strlen(f->tag);
	size_t content = f->len + tl + 3;
	size_t total = content + 2;

	uint8_t *blob = malloc(total);
	if (!blob) {
		free(f->buf);
		f->mode = FM_CLOSED;
		return FS_MEMORY;
	}
	blob[0] = (content >> 8) & 0xFF; // big-endian, as on the calculator
	blob[1] = content & 0xFF;
	memcpy(blob + 2, f->buf, f->len);

	uint8_t *t = blob + 2 + f->len;
	*t++ = 0;
	memcpy(t, f->tag, tl);
	t += tl;
	*t++ = 0;
	*t = OTH_TAG;

	struct var_t *v = find_slot(f->name);
	if (!v) {
		if (Var_count >= MAX_VARS) {
			free(blob);
			free(f->buf);
			f->mode = FM_CLOSED;
			return FS_MEMORY;
		}
		v = &Vars[Var_count++];
		snprintf(v->name, sizeof v->name, "%s", f->name);
	} else {
		free(v->data);
	}
	v->data = blob;
	v->size = total;
	v->loaded = 1;

	save_to_storage(v->name, blob, total);

	free(f->buf);
	f->buf = NULL;
	f->mode = FM_CLOSED;
	return FS_OK;
}
