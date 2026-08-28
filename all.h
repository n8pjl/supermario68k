

// include guards to prevent errors with infinite include nesting :-)
#ifndef __all__
#define __all__

// Generate calc specific code. Nesscessarry in scankeys and some graphics
// functions

// Speedtest:
// Note: This does NOT give an accurate FPS, probably not even close to. See
// main.c for more info.
// #define speedtest

// Which calculator to build for. The Makefile passes exactly one of
// PRODUCE_TI89_CODE, PRODUCE_TI92PLUS_CODE or PRODUCE_V200_CODE on the command
// line - see CALC there - rather than it being picked here, because the choice
// also selects which of the Bin/ data sets gets packaged, and the two must
// agree. compat/graph.h raises the error if none was passed.

// Raise this number if new versions with new features are released
#define compatibility 1

// #define debug
// gives some debugging features like ability to skip levels and worlds
// extern short Slow;//used in debug only

// #define immortal_mode
// Gives immortal mario: Useful when debugging

#undef MIN_AMS
#define MIN_AMS 200

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "compat/alloc.h"
#include "compat/graph.h"
#include "compat/gray.h"
#include "compat/kbd.h"
#include "compat/tilemap.h"
#include "compat/tios.h"
#include "compat/utils.h"

#include "bosses.h"
#include "comp.h"
#include "compat/extgraph.h"
#include "control.h"
#include "custom.h"
#include "enemies.h"
#include "error.h"
#include "flying.h"
#include "gameloop.h"
#include "gfx.h"
#include "items.h"
#include "level.h"
#include "levelset.h"
#include "objects.h"
#include "player.h"
#include "render.h"
#include "savegame.h"
#include "scankeys.h"
#include "shells.h"
#include "smallgames.h"
// #include "DrawGrayPlane16BRoll.h"
#include "bounch.h"
#include "map.h"
#include "menus.h"
#include "rle.h"
#include "stringcopy.h"
#include "titlescreen.h"

#endif
