CC = emcc
# -MMD -MP: emit the header dependencies picked up below, so editing a header
# rebuilds what includes it. Nearly every file here includes all.h, which
# reaches most of the others, so without this a header edit is silently
# ignored and the link fails - or worse, does not.
CFLAGS = -Os -std=gnu23 -flto -msimd128 -MMD -MP
# -sENVIRONMENT=web: this only ever runs in a browser, so drop the node,
# worker and shell startup paths Emscripten emits by default.
# -sEXPORT_ES6: emit mario.mjs as an ES module (a default-exported factory),
# so shell.js can `import` it instead of reaching for a global Module. This
# also implies -sMODULARIZE.
LDFLAGS = -sJSPI -Os -flto -sENVIRONMENT=web -sEXPORT_ES6=1 \
          -sEXPORTED_FUNCTIONS=_main,_malloc --preload-file data@/data

SRCDIR = src

OUTDIR = dist

# Emscripten links into here rather than straight into $(OUTDIR). Everything is
# minified on its way across, and esbuild will not write over its own input.
BUILDDIR = build

TARGET = $(BUILDDIR)/mario.js

# esbuild is pinned in package.json, so the shipped bytes do not change under us
# when a new version lands. --ignore-scripts on the install because the binary
# is resolved out of @esbuild/<platform> at run time anyway - the postinstall it
# skips only swaps the launcher shim for that binary.
ESBUILD = ./node_modules/.bin/esbuild

# What is served is built in one pass by tools/mkdist.py, which minifies each
# file and renames it to carry a hash of its contents - see the comment at the
# top of that script for why, and for the order it has to work in. Those names
# cannot be make targets: they are not known until the bytes being hashed
# exist. This stamp is what make tracks in their place.
DIST = .dist-stamp

NAMES = main.c enemies.c gameloop.c items.c player.c render.c \
        scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
        bounch.c map.c titlescreen.c text.c rle.c level.c savegame.c \
        stringcopy.c error.c bosses.c gfx.c \
        compat/tios.c compat/tilemap.c compat/extgraph.c \
        compat/graph.c compat/font_data.c compat/gray.c

SRCS = $(addprefix $(SRCDIR)/,$(NAMES))
HDRS = $(wildcard $(SRCDIR)/*.h $(SRCDIR)/compat/*.h)

# Objects land beside their sources, so $(SRCDIR)/compat already exists and
# the %.o: %.c rule needs no per-directory handling.
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean data

all: $(DIST)

# The link depends on this rather than on a phony `data` target, so that
# changing the converter or the target actually repackages mario.data - a
# preloaded file is baked in at link time, and a phony prerequisite would be
# rebuilt without the link noticing.
.data-stamp: tools/mkdata.py
	python3 tools/mkdata.py calc-data data
	@touch $@

data: .data-stamp

# CFLAGS and LDFLAGS live here, so a change to this file has to rebuild and
# relink - the same reason the data and the headers are prerequisites.
$(OBJS): Makefile

# Order-only: the link also drops mario.wasm and mario.data next to $@, so the
# directory has to exist, but its timestamp must not force a relink.
$(TARGET): $(OBJS) .data-stamp Makefile | $(BUILDDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Byproducts of that link. Naming them with an empty recipe is what tells make
# they are already up to date once $(TARGET) is, rather than having no rule.
$(BUILDDIR)/mario.wasm $(BUILDDIR)/mario.data: $(TARGET) ;

# mkdist.py empties $(OUTDIR) and refills it, so there is nothing here for make
# to build incrementally and nothing for a stale hash to survive in.
$(DIST): $(TARGET) $(BUILDDIR)/mario.wasm $(BUILDDIR)/mario.data \
         index.html shell.js shell.css ma_texts.json \
         tools/mkdist.py Makefile | $(ESBUILD)
	ESBUILD=$(ESBUILD) python3 tools/mkdist.py $(BUILDDIR) $(OUTDIR)
	@touch $@

# Order-only where it is used: reinstalling the minifier must not force a
# rebuild of anything it did not change.
$(ESBUILD): package-lock.json
	npm ci --ignore-scripts
	@touch $@

$(BUILDDIR):
	mkdir -p $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format: $(SRCS) $(HDRS)
	clang-format --style=file --sort-includes -i $(SRCS) $(HDRS)


-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) .data-stamp $(DIST)
	rm -rf $(OUTDIR) $(BUILDDIR) data
