CC = emcc
# TODO: maybe use -msimd128 ?
CFLAGS = -Os -std=gnu99 -g
LDFLAGS = -sJSPI

TARGET = mario

SRCS = main.c enemies.c gameloop.c  items.c player.c render.c \
       scankeys.c shells.c custom.c objects.c flying.c smallgames.c \
       bounch.c map.c titlescreen.c menus.c rle.c level.c savegame.c \
       stringcopy.c error.c bosses.c gfx.c compat/alloc.c compat/gray.c compat/browser_io.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)