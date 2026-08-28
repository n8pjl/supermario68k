CC = emcc
# TODO: maybe use -msimd128 ?
# -fcommon: the sources rely on pre-C99 tentative definitions (globals declared
# in headers without extern), which old GCC merged. Modern clang defaults to
# -fno-common and would report them as duplicate symbols.
# -fgnu89-inline: `inline` here means the pre-C99 GNU semantics the sources were
# written against, where a header's inline declaration does not suppress the
# out-of-line definition in the .c file.
CFLAGS = -Os -std=gnu99 -g -fgnu89-inline
LDFLAGS = -sJSPI \
          --js-library compat/library.js \
          --pre-js compat/library_pre.js \
          --preload-file data@/data

TARGET = mario

SRCS = main.c enemies.c gameloop.c  items.c player.c render.c \
       scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
       bounch.c map.c titlescreen.c menus.c rle.c level.c savegame.c \
       stringcopy.c error.c bosses.c gfx.c compat/alloc.c compat/tios.c compat/tilemap.c compat/extgraph.c compat/graph.c compat/kbd.c compat/gray.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)