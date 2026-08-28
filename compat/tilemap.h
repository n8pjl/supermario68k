#pragma once

// Types and constants the game needs from the old ExtGraph/TileMap engine,
// inlined here so the build no longer depends on the ExtGraph source tree. The
// implementations live in compat/tilemap.c; see there for how the engine works.
//
// The game targets the older TileMap engine (the TileMap.a shipped alongside
// the sources), whose gray entry points take the light and dark planes as two
// separate arguments. ExtGraph 2.x reworked those to a single interleaved
// destination, so its TM_* macros have the wrong arity here and are replaced.

// Big virtual screen geometry: the engine renders a 272x160 window of the map
// into this buffer and copies the visible screen out of it.
#define BIG_VSCREEN_SIZE 5440
#define GRAY_BIG_VSCREEN_SIZE (5440 * 2)
#define BIG_VSCREEN_WIDTH 272
#define BIG_VSCREEN_HEIGHT 160

// Describes a tilemap plane, used by the high-level DrawPlane functions.
struct Plane {
  void *matrix;         // matrix of tile numbers, one byte each
  unsigned short width; // map width, in tiles
  void *sprites;        // tile sprite array
  char *big_vscreen;    // big virtual screen (see sizes above)
  long mask;            // obsolete, kept for layout compatibility
  long reserved;        // engine scratch
  short force_update;   // set to force a big_vscreen rebuild
};

// Describes a tilemap animated plane, used by the DrawAnimatedPlane functions.
struct AnimatedPlane {
  struct Plane p;
  void *tabanim;     // matrix of animations
  short nb_anim;     // number of animations
  short nb_step;     // number of animation steps
  short step;        // current step
  short step_length; // step length, in frames
  short frame;       // current frame within the step
};

// Blitter callback for the mono plane drawers: one refreshed buffer to one
// plane. (ExtGraph's own name for this signature.)
typedef void (*TM_Mode)(const void *src, unsigned short x, unsigned short y,
                        void *dest);

// Blitter callback for the gray plane drawers: one refreshed buffer onto both
// planes.
typedef void (*TM_GrayMode)(const void *src, unsigned short x, unsigned short y,
                            void *lightplane, void *darkplane);

void DrawBuffer_MASK(const void *src, unsigned short x, unsigned short y,
                     void *dest);
void DrawGrayBuffer2B_RPLC(const void *src, unsigned short x, unsigned short y,
                           void *lightplane, void *darkplane);
void DrawGrayBuffer2B_OR(const void *src, unsigned short x, unsigned short y,
                         void *lightplane, void *darkplane);

// The "89" variants differ from the others only in copying a 160x100 window
// instead of a 240x128 one - the destination step is a full 30-byte plane row
// either way - and the ports take that window from LCD_WIDTH/LCD_HEIGHT, which
// the build target already fixes. So one function serves both names, and
// render.c can keep asking for whichever its per-model #ifdef picks.
#define TM_GRPLC DrawGrayBuffer2B_RPLC
#define TM_GOR DrawGrayBuffer2B_OR
#define TM_MASK DrawBuffer_MASK
#define TM_GRPLC89 DrawGrayBuffer2B_RPLC
#define TM_GOR89 DrawGrayBuffer2B_OR
#define TM_MASK89 DrawBuffer_MASK

// Plane drawers. _ROLL is the game's own scrolling variant of the 16x16 big
// tile renderer (see DrawGrayPlane16BRoll referenced from render.c).
void DrawGrayPlane16B2B(unsigned short x, unsigned short y, struct Plane *plane,
                        void *lightplane, void *darkplane, TM_GrayMode mode);
void DrawGrayPlane16B2B_ROLL(unsigned short x, unsigned short y,
                             struct Plane *plane, void *lightplane,
                             void *darkplane, TM_GrayMode mode);
void DrawGrayAnimatedPlane16B2B(unsigned short x, unsigned short y,
                                struct AnimatedPlane *plane, void *lightplane,
                                void *darkplane, TM_GrayMode mode);
void DrawAnimatedPlane16B(unsigned short x, unsigned short y,
                          struct AnimatedPlane *plane, void *dest,
                          TM_Mode mode);

#define TM_G16B DrawGrayPlane16B2B
#define TM_G16B_ROLL DrawGrayPlane16B2B_ROLL
#define TM_GA16B DrawGrayAnimatedPlane16B2B
#define TM_A16B DrawAnimatedPlane16B

#define DrawGrayPlane(x, y, plane, dest0, dest1, mode, type)                   \
  type((x), (y), (plane), (dest0), (dest1), (mode))
#define DrawGrayAnimatedPlane(x, y, plane, dest0, dest1, mode, type)           \
  type((x), (y), (plane), (dest0), (dest1), (mode))
#define DrawAnimatedPlane(x, y, plane, dest, mode, type)                       \
  type((x), (y), (plane), (dest), (mode))
