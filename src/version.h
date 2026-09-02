#pragma once

#include <stdbool.h>

extern bool ti89_mode;

// Raise this number if new versions with new features are released
#define compatibility 1

// The shape of a savegame in localStorage (see compat/assets.c). Separate from
// `compatibility` above, which describes what levelset features this build
// understands: this one describes only how a save is written down, and a save
// whose version does not match is ignored rather than guessed at. Raise it
// whenever the record's fields or the encoding of its payload change.
#define SAVE_VERSION 2