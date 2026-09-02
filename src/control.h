#pragma once

#include <stdbool.h>

enum map_statusbar_state { status = 0, itemlist = 1 };

extern char Exit; // quit asap. when errors and when quit choosen from menus
// etc.
extern char ErrorCode;
extern bool Skip_anim;

extern char Map_statusbar;
