CC = em++
# -MMD -MP: emit the header dependencies picked up below, so editing a header
# rebuilds what includes it. Nearly every file here includes all.h, which
# reaches most of the others, so without this a header edit is silently
# ignored and the link fails - or worse, does not.
# -I.: compat/assets.cpp reaches the game data as #embed "data/<name>.bin", which
# is resolved from here rather than from the including file's directory.
# -fwasm-exceptions: the WebAssembly exception proposal rather than Emscripten's
# JS-based unwinding, which is a size and speed cost on every call. There is one
# throw in the whole game - speedrun::Stopped, unwinding out of a level when a
# recording is finished with it - and main() is the only thing that catches.
# -sWASM_LEGACY_EXCEPTIONS=0: emit the standardised try_table rather than the
# superseded try, which browsers now warn about on every load. Both are a
# compile and link setting, so both appear in LDFLAGS as well.
CFLAGS = -Os -std=gnu++26 -flto -msimd128 -MMD -MP -I. -fwasm-exceptions \
         -sWASM_LEGACY_EXCEPTIONS=0
# -sENVIRONMENT=web: this only ever runs in a browser, so drop the node,
# worker and shell startup paths Emscripten emits by default.
# -sEXPORT_ES6: emit mario.mjs as an ES module (a default-exported factory),
# so shell.js can `import` it instead of reaching for a global Module. This
# also implies -sMODULARIZE.
# -lembind: speedrun.cpp reports its events through emscripten::val, and
# registers their payload structs as value_objects so they convert themselves.
LDFLAGS = -sJSPI -Os -flto -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=0 \
          -sENVIRONMENT=web -sEXPORT_ES6=1 -lembind \
          -sEXPORTED_FUNCTIONS=_main,_malloc

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

# esbuild strips the types out of the speedrun module without ever looking at
# them; tsc is the thing that checks them. Both come from the one npm ci below.
TSC = ./node_modules/.bin/tsc

# The timer is several modules, and reaches the page as one file: esbuild
# bundles it, so the hashed-filename scheme in tools/mkdist.py still has a
# single leaf to name rather than a graph of imports to rewrite.
SPEEDRUN = $(wildcard speedrun/*.ts)

# Written by that check, which has nothing else to show for itself.
TYPECHECK = .typecheck-stamp

# What is served is built in one pass by tools/mkdist.py, which minifies each
# file and renames it to carry a hash of its contents - see the comment at the
# top of that script for why, and for the order it has to work in. Those names
# cannot be make targets: they are not known until the bytes being hashed
# exist. This stamp is what make tracks in their place.
DIST = .dist-stamp

NAMES = main.cpp enemies.cpp gameloop.cpp items.cpp player.cpp render.cpp \
        scankeys.cpp shells.cpp custom.cpp objects.cpp flying.cpp smallgames.cpp \
        bounch.cpp map.cpp titlescreen.cpp text.cpp rle.cpp level.cpp savegame.cpp \
        stringcopy.cpp error.cpp bosses.cpp gfx.cpp speedrun.cpp \
        compat/assets.cpp compat/tilemap.cpp compat/extgraph.cpp \
        compat/graph.cpp compat/font_data.cpp compat/gray.cpp

SRCS = $(addprefix $(SRCDIR)/,$(NAMES))
HDRS = $(wildcard $(SRCDIR)/*.h $(SRCDIR)/compat/*.h)

# Objects land beside their sources, so $(SRCDIR)/compat already exists and
# the %.o: %.cpp rule needs no per-directory handling.
OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean data format typecheck

all: speedrun.js $(DIST)

# The level data's source: JSON under levels/, compiled to the blobs the game
# embeds. See tools/mklevels.py for the format and for why encoding it
# reproduces the shipped bytes exactly.
LEVELS = $(wildcard levels/*.json)

# Both tools read the asset list out of assets.cpp and check their half of it,
# so a file added or renamed there has to run them again.
.data-stamp: tools/mkdata.py tools/mklevels.py $(LEVELS) \
             $(SRCDIR)/compat/assets.cpp
	python3 tools/mkdata.py calc-data data
	python3 tools/mklevels.py levels data
	@touch $@

data: .data-stamp

# assets.cpp #embeds data/, so the converted files have to exist before it can be
# compiled at all. Once they do, -MMD lists each embedded file in assets.d and
# picks up any later change to it; this stamp is only what gets the first build
# off the ground.
$(SRCDIR)/compat/assets.o: .data-stamp

# CFLAGS and LDFLAGS live here, so a change to this file has to rebuild and
# relink - the same reason the data and the headers are prerequisites.
$(OBJS): Makefile

# Order-only: the link also drops mario.wasm next to $@, so the directory has
# to exist, but its timestamp must not force a relink.
$(TARGET): $(OBJS) Makefile | $(BUILDDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Byproduct of that link. Naming it with an empty recipe is what tells make it
# is already up to date once $(TARGET) is, rather than having no rule.
$(BUILDDIR)/mario.wasm: $(TARGET) ;

# mkdist.py empties $(OUTDIR) and refills it, so there is nothing here for make
# to build incrementally and nothing for a stale hash to survive in.
$(DIST): $(TARGET) $(BUILDDIR)/mario.wasm \
         index.html shell.js shell.css ma_texts.json $(SPEEDRUN) \
         $(TYPECHECK) tools/mkdist.py Makefile | $(ESBUILD)
	ESBUILD=$(ESBUILD) python3 tools/mkdist.py $(BUILDDIR) $(OUTDIR)
	@touch $@

# Order-only where it is used: reinstalling the minifier must not force a
# rebuild of anything it did not change.
$(ESBUILD): package-lock.json
	npm ci --ignore-scripts
	@touch $@

# Installed by that same npm ci, the way mario.wasm falls out of the link above.
$(TSC): $(ESBUILD) ;

# A type error fails the build rather than riding along into dist/: nothing
# downstream of here would notice one, least of all esbuild.
$(TYPECHECK): $(SPEEDRUN) tsconfig.json | $(TSC)
	$(TSC) --noEmit
	@touch $@

# The speedrun module is the one source here a browser cannot load as it
# stands, and serving this directory as it is - index.html reaches shell.js and
# shell.css by name - is how a change gets tried without building dist/. So it
# is bundled beside its source for that, gitignored and read by nothing else;
# dist/'s own copy is built from the same entry point by mkdist.py.
speedrun.js: $(SPEEDRUN) $(TYPECHECK) | $(ESBUILD)
	$(ESBUILD) speedrun/index.ts --bundle --format=esm --target=esnext \
		--outfile=$@

$(BUILDDIR):
	mkdir -p $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

format: $(SRCS) $(HDRS)
	clang-format --style=file --sort-includes -i $(SRCS) $(HDRS)

typecheck: $(TYPECHECK)


-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) .data-stamp $(DIST) $(TYPECHECK) speedrun.js
	rm -rf $(OUTDIR) $(BUILDDIR) data
