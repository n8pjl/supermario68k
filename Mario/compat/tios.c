#include "tios.h"

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 32
#define DATA_DIR "/data"

typedef struct {
	char name[9];		// bare variable name, no folder
	unsigned char *data;	// [2-byte BE size][content], as HeapDeref returns
	size_t size;
	int loaded;		// 0 = slot unused, -1 = known missing
} var_t;

static var_t Vars[MAX_VARS];
static short Var_count;

// The game addresses variables as "folder\name"; we only care about the name.
static const char *basename_of(const char *path)
{
	const char *p = strrchr(path, '\\');
	return p ? p + 1 : path;
}

static var_t *find_slot(const char *name)
{
	for (short i = 0; i < Var_count; i++)
		if (!strcmp(Vars[i].name, name))
			return &Vars[i];
	return NULL;
}

// A save written this session shadows the shipped blob. Stored base64 so it
// survives localStorage's string-only values.
static unsigned char *load_from_storage(const char *name, size_t *out_len)
{
	// Read in two steps - ask for the length, allocate here, then have JS
	// fill the buffer - so the C side owns the allocation. The obvious
	// one-shot version returns stringToNewUTF8(v), but that is an
	// emscripten *library* symbol rather than a runtime one: it is not
	// linked in unless asked for, and the throw it raises when missing was
	// being swallowed by the catch below, so a stored save silently read
	// back as "absent" and the shipped blob was loaded instead. The string
	// is parked on globalThis between the two calls, as gray.c does for its
	// frame timestamp, so localStorage is still only read once.
	int b64len = EM_ASM_INT({
		try {
			var v = localStorage.getItem("sm68k/" + UTF8ToString($0));
			globalThis.__sm68kSave = v;
			return v === null ? -1 : v.length;
		} catch (e) {
			globalThis.__sm68kSave = null;
			return -1;
		}
	}, name);
	if (b64len < 0)
		return NULL;

	char *b64 = malloc((size_t)b64len + 1);
	if (!b64) {
		EM_ASM({ globalThis.__sm68kSave = null; });
		return NULL;
	}
	EM_ASM({
		var v = globalThis.__sm68kSave;
		// This is base64 we wrote ourselves, so every code unit is one
		// ASCII byte and no UTF-8 encoding step is needed.
		for (var i = 0; i < v.length; i++)
			HEAPU8[$0 + i] = v.charCodeAt(i) & 0x7F;
		HEAPU8[$0 + v.length] = 0;
		globalThis.__sm68kSave = null;
	}, b64);

	size_t n = (size_t)b64len;
	unsigned char *out = malloc(n);	// decoded is always smaller
	size_t len = 0;
	int acc = 0, bits = 0;
	for (size_t i = 0; i < n; i++) {
		int c = b64[i], v;
		if (c >= 'A' && c <= 'Z') v = c - 'A';
		else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
		else if (c >= '0' && c <= '9') v = c - '0' + 52;
		else if (c == '+') v = 62;
		else if (c == '/') v = 63;
		else continue;	// '=' and stray whitespace
		acc = (acc << 6) | v;
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out[len++] = (acc >> bits) & 0xFF;
		}
	}
	free(b64);
	*out_len = len;
	return out;
}

static void save_to_storage(const char *name, const unsigned char *data, size_t len)
{
	static const char T[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t n = ((len + 2) / 3) * 4;
	char *b64 = malloc(n + 1);
	size_t o = 0;
	for (size_t i = 0; i < len; i += 3) {
		unsigned v = data[i] << 16;
		if (i + 1 < len) v |= data[i + 1] << 8;
		if (i + 2 < len) v |= data[i + 2];
		b64[o++] = T[(v >> 18) & 63];
		b64[o++] = T[(v >> 12) & 63];
		b64[o++] = (i + 1 < len) ? T[(v >> 6) & 63] : '=';
		b64[o++] = (i + 2 < len) ? T[v & 63] : '=';
	}
	b64[o] = 0;
	EM_ASM({
		try {
			localStorage.setItem("sm68k/" + UTF8ToString($0), UTF8ToString($1));
		} catch (e) {}
	}, name, b64);
	free(b64);
}

// Pull a variable into memory: a localStorage save if one exists, otherwise the
// shipped blob preloaded into MEMFS at /data.
static var_t *load_var(const char *path)
{
	const char *name = basename_of(path);
	var_t *v = find_slot(name);
	if (v)
		return v->loaded > 0 ? v : NULL;

	if (Var_count >= MAX_VARS)
		return NULL;
	v = &Vars[Var_count++];
	snprintf(v->name, sizeof v->name, "%s", name);

	size_t len = 0;
	unsigned char *data = load_from_storage(name, &len);

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
SYM_ENTRY *SymFindPtr(SYM_STR name, short flags)
{
	(void)flags;
	var_t *v = load_var((const char *)name);
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
	return handle;	// nothing can move underneath us here
}

// TIOS returns a pointer to the variable's last byte, which is the OTH tag.
// menus.c relies on HToESI(h)-6 landing on {0,'M','L','S','T',0,OTH_TAG}.
void *HToESI(HANDLE handle)
{
	if (!handle || handle > Var_count)
		return NULL;
	var_t *v = &Vars[handle - 1];
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
	return TRUE;	// single flat namespace
}

// Levelset enumeration. The game asks for every variable carrying a given OTH
// tag; we scan the shipped blobs and match on the tag block at the tail.
static const char *Find_tag;
static short Find_pos;

static int has_tag(var_t *v, const char *tag)
{
	size_t tl = strlen(tag);
	if (v->size < tl + 3)
		return 0;
	const unsigned char *p = v->data + v->size - (tl + 3);
	return p[0] == 0 && !memcmp(p + 1, tag, tl) && p[tl + 1] == 0 &&
	       p[tl + 2] == OTH_TAG;
}

// Names of the blobs shipped in data/; MEMFS has no cheap directory scan here
// and the set is fixed, so enumerate it directly.
static const char *const Known[] = {
	"sm68k", "common", "world1", "world2", "world3", "world4",
	"world5", "world6", "world7", "world8", NULL
};

SYM_ENTRY *FFindFirst(short flags, const char *tag, const char *folder)
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
		var_t *v = load_var(name);
		if (v && has_tag(v, Find_tag))
			return SymFindPtr(SYMSTR(name), 0);
	}
	return NULL;
}

unsigned short FOpen(const char *name, FILES *f, short mode, const char *tag)
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

unsigned short FWrite(const void *data, short len, FILES *f)
{
	if (f->mode != FM_WRITE)
		return FS_ERROR;
	size_t need = f->len + (size_t)(unsigned short)len;
	if (need > f->cap) {
		while (f->cap < need)
			f->cap *= 2;
		unsigned char *b = realloc(f->buf, f->cap);
		if (!b)
			return FS_MEMORY;
		f->buf = b;
	}
	memcpy(f->buf + f->len, data, (unsigned short)len);
	f->len = need;
	return FS_OK;
}

// Rebuild the on-calc variable block: [2-byte BE size][payload][tag block],
// where the tag block is {0, tag..., 0, OTH_TAG} - the layout observed in every
// shipped blob. The result replaces the cached copy so a save is visible to the
// very next read, and is persisted to localStorage.
unsigned short FClose(FILES *f)
{
	if (f->mode != FM_WRITE)
		return FS_ERROR;

	size_t tl = strlen(f->tag);
	size_t content = f->len + tl + 3;
	size_t total = content + 2;

	unsigned char *blob = malloc(total);
	if (!blob) {
		free(f->buf);
		f->mode = FM_CLOSED;
		return FS_MEMORY;
	}
	blob[0] = (content >> 8) & 0xFF;	// big-endian, as on the calculator
	blob[1] = content & 0xFF;
	memcpy(blob + 2, f->buf, f->len);

	unsigned char *t = blob + 2 + f->len;
	*t++ = 0;
	memcpy(t, f->tag, tl);
	t += tl;
	*t++ = 0;
	*t = OTH_TAG;

	var_t *v = find_slot(f->name);
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

short EM_findEmptySlot(long size)      { (void)size; return 1; }
short EM_moveSymFromExtMem(SYM_STR n, HANDLE h) { (void)n; (void)h; return 1; }
short EM_moveSymToExtMem(SYM_STR n, HANDLE h)   { (void)n; (void)h; return 1; }
short EM_GC(BOOL all)                  { (void)all; return 1; }
