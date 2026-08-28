

//include guards to prevent errors with infinite include nesting :-) 
#ifndef __all__
#define __all__


					//Generate calc specific code. Nesscessarry in scankeys and some graphics functions

// Speedtest:
// Note: This does NOT give an accurate FPS, probably not even close to. See main.c for more info.
//#define speedtest


// Which calculator to build for. The Makefile passes exactly one of
// PRODUCE_TI89_CODE, PRODUCE_TI92PLUS_CODE or PRODUCE_V200_CODE on the command
// line - see CALC there - rather than it being picked here, because the choice
// also selects which of the Bin/ data sets gets packaged, and the two must
// agree. compat/graph.h raises the error if none was passed.

//Raise this number if new versions with new features are released
#define compatibility 1


//#define debug
//gives some debugging features like ability to skip levels and worlds
//extern short Slow;//used in debug only

//#define immortal_mode
//Gives immortal mario: Useful when debugging

#undef MIN_AMS
#define MIN_AMS 200

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "compat/tilemap.h"
#include "compat/utils.h"
#include "compat/alloc.h"
#include "compat/tios.h"
#include "compat/gray.h"
#include "compat/graph.h"
#include "compat/kbd.h"

#include "error.h"
#include "savegame.h"
#include "../ExtGraph/lib/tilemap.h"
#include "../ExtGraph/lib/extgraph.h"
#include "compat/extgraph.h"
#include "comp.h"
#include "enemies.h"
#include "player.h"
#include "levelset.h"
#include "level.h"
#include "items.h"
#include "gfx.h"
#include "control.h"
#include "custom.h"
#include "objects.h"
#include "shells.h"
#include "render.h"
#include "gameloop.h"
#include "scankeys.h"
#include "flying.h"
#include "bosses.h"
#include "smallgames.h"
//#include "DrawGrayPlane16BRoll.h"
#include "bounch.h"
#include "map.h"
#include "titlescreen.h"
#include "menus.h"
#include "rle.h"
#include "stringcopy.h"



#endif




