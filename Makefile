CC = emcc
# -fgnu89-inline: `inline` here means the pre-C99 GNU semantics the sources were
# written against, where a header's inline declaration does not suppress the
# out-of-line definition in the .c file.
# -MMD -MP: emit the header dependencies picked up below, so editing a header
# rebuilds what includes it. Nearly every file here includes all.h, which
# reaches most of the others, so without this a header edit is silently
# ignored and the link fails - or worse, does not.
CFLAGS = -Os -std=gnu99 -fgnu89-inline -flto -msimd128 -MMD -MP
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

# --target=esnext rather than a particular year: at anything older esbuild
# strips the `with { type: "json" }` attribute off shell.js's ma_texts import,
# and a browser then refuses the module over its MIME type. Nothing is lost by
# it - the shell already asks for browsers far newer than any syntax involved.
ESBUILD_JS = $(ESBUILD) --minify --format=esm --target=esnext

# Everything served, in its built location. $(OUTDIR)/$(NAMES) would only
# prefix the first word, hence addprefix.
DIST = $(addprefix $(OUTDIR)/,index.html shell.js shell.css ma_texts.json \
                              mario.js mario.wasm mario.data)

NAMES = main.c enemies.c gameloop.c items.c player.c render.c \
        scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
        bounch.c map.c titlescreen.c text.c rle.c level.c savegame.c \
        stringcopy.c error.c bosses.c gfx.c \
        compat/alloc.c compat/tios.c compat/tilemap.c compat/extgraph.c \
        compat/graph.c compat/font_data.c compat/gray.c

SRCS = $(addprefix $(SRCDIR)/,$(NAMES))
HDRS = $(wildcard $(SRCDIR)/*.h $(SRCDIR)/compat/*.h)

# Objects land beside their sources, so $(SRCDIR)/compat already exists and
# the %.o: %.c rule needs no per-directory handling.
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean data FORCE

all: $(DIST)

# Both the objects and the data depend on which calculator is being built for,
# so record it in a stamp file that changes whenever CALC does. Without this,
# switching targets would leave stale objects and the wrong data packaged.
.calc-stamp: FORCE
	@echo "$(CALC)" | cmp -s - $@ || echo "$(CALC)" > $@

FORCE:

# The link depends on this rather than on a phony `data` target, so that
# changing the converter or the target actually repackages mario.data - a
# preloaded file is baked in at link time, and a phony prerequisite would be
# rebuilt without the link noticing.
.data-stamp: tools/mkdata.py .calc-stamp
	python3 tools/mkdata.py calc-data data
	@touch $@

data: .data-stamp

# CFLAGS and LDFLAGS live here, so a change to this file has to rebuild and
# relink - the same reason the data and the headers are prerequisites.
$(OBJS): .calc-stamp Makefile

# Order-only: the link also drops mario.wasm and mario.data next to $@, so the
# directory has to exist, but its timestamp must not force a relink.
$(TARGET): $(OBJS) .data-stamp Makefile | $(BUILDDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Byproducts of that link. Naming them with an empty recipe is what tells make
# they are already up to date once $(TARGET) is, rather than having no rule.
$(BUILDDIR)/mario.wasm $(BUILDDIR)/mario.data: $(TARGET) ;

$(OUTDIR)/mario.js: $(TARGET) | $(OUTDIR) $(ESBUILD)
	$(ESBUILD_JS) $< --outfile=$@

# The wasm and the preloaded data are already as small as they are going to get.
$(OUTDIR)/mario.wasm $(OUTDIR)/mario.data: $(OUTDIR)/%: $(BUILDDIR)/% | $(OUTDIR)
	cp $< $@

$(OUTDIR)/shell.js: shell.js | $(OUTDIR) $(ESBUILD)
	$(ESBUILD_JS) $< --outfile=$@

$(OUTDIR)/shell.css: shell.css | $(OUTDIR) $(ESBUILD)
	$(ESBUILD) $< --minify --outfile=$@

# ma_texts.json is indented for the person editing it; nothing that reads it
# cares. ensure_ascii=False keeps the accented text as UTF-8 rather than
# doubling its size in \u escapes.
$(OUTDIR)/ma_texts.json: ma_texts.json | $(OUTDIR)
	python3 -c 'import json,sys; json.dump(json.load(open(sys.argv[1])), \
	    open(sys.argv[2], "w"), separators=(",", ":"), ensure_ascii=False)' $< $@

$(OUTDIR)/index.html: index.html | $(OUTDIR)
	cp $< $@

# Order-only everywhere it is used: reinstalling must not relink or re-minify.
$(ESBUILD): package-lock.json
	npm ci --ignore-scripts
	@touch $@

$(OUTDIR) $(BUILDDIR):
	mkdir -p $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format: $(SRCS) $(HDRS)
	clang-format --style=file --sort-includes -i $(SRCS) $(HDRS)


-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) .calc-stamp .data-stamp
	rm -rf $(OUTDIR) $(BUILDDIR) data
