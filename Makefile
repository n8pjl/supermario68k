# Which calculator to build for: 89 (TI-89 / TI-89 Titanium), 92p (TI-92 Plus)
# or v200 (Voyage 200). This picks both the PRODUCE_*_CODE the sources switch
# on - screen size, scroll limits, status bar position - and the matching data
# set under Bin/, which is not interchangeable between models: the backgrounds,
# the common level and some worlds were retuned for the 89's smaller screen.
CALC ?= v200

ifeq ($(CALC),89)
CALCDEF = PRODUCE_TI89_CODE
else ifeq ($(CALC),92p)
CALCDEF = PRODUCE_TI92PLUS_CODE
else ifeq ($(CALC),v200)
CALCDEF = PRODUCE_V200_CODE
else
$(error CALC must be 89, 92p or v200)
endif

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
CFLAGS = -Os -std=gnu99 -fgnu89-inline -flto -msimd128 -MMD -MP -D$(CALCDEF)
# -sENVIRONMENT=web: this only ever runs in a browser, so drop the node,
# worker and shell startup paths Emscripten emits by default.
# -sEXPORT_ES6: emit mario.mjs as an ES module (a default-exported factory),
# so shell.js can `import` it instead of reaching for a global Module. This
# also implies -sMODULARIZE.
LDFLAGS = -sJSPI -Os -flto -sENVIRONMENT=web -sEXPORT_ES6=1 \
          --preload-file data@/data

TARGET = mario.mjs

SRCS = main.c enemies.c gameloop.c  items.c player.c render.c \
       scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
       bounch.c map.c titlescreen.c menus.c rle.c level.c savegame.c \
       stringcopy.c error.c bosses.c gfx.c compat/alloc.c compat/tios.c compat/tilemap.c compat/extgraph.c compat/graph.c compat/font_data.c compat/kbd.c compat/gray.c

OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# Filter out anything under compat/ from SRCS
NON_COMPAT_SRCS = $(filter-out compat/%,$(SRCS))
.PHONY: all clean data FORCE

all: $(TARGET)

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

$(TARGET): $(OBJS) .data-stamp Makefile
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format: $(NON_COMPAT_SRCS) $(HDRS)
	clang-format --sort-includes -i $(NON_COMPAT_SRCS) $(HDRS)


-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET) mario.js mario.wasm mario.data .calc-stamp .data-stamp
	rm -rf data
