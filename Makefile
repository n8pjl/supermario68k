CC = emcc
# -fcommon: the sources rely on pre-C99 tentative definitions (globals declared
# in headers without extern), which old GCC merged. Modern clang defaults to
# -fno-common and would report them as duplicate symbols.
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
          --preload-file data@/data

SRCDIR = src

OUTDIR = dist

TARGET = $(OUTDIR)/mario.mjs

# Static files served alongside the wasm. COPIES is the list of their built
# locations: $(OUTDIR)/$(COPY) would only prefix the first word.
COPY = index.html shell.js shell.css
COPIES = $(addprefix $(OUTDIR)/,$(COPY))

NAMES = main.c enemies.c gameloop.c items.c player.c render.c \
        scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
        bounch.c map.c titlescreen.c menus.c rle.c level.c savegame.c \
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

all: $(TARGET) $(COPIES)

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
$(TARGET): $(OBJS) .data-stamp Makefile | $(OUTDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OUTDIR)/%: % | $(OUTDIR)
	cp $< $@

$(OUTDIR):
	mkdir -p $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format: $(SRCS) $(HDRS)
	clang-format --sort-includes -i $(SRCS) $(HDRS)


-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) .calc-stamp .data-stamp
	rm -rf $(OUTDIR) data
