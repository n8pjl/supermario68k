#ifndef __levelset__
#define __levelset__

// Reconstructed from the shipped levelset binary (sm68k). The original
// levelset.h was not part of the released source drop, but all.h still
// referenced it and the layout is pinned down by the data plus its use sites:
//
//   savegame.c:19  Size = sizeof(levelsetdata)+21 + 9*(Nr_of_files+1)
//                  -> the 9 filename slots in sm68k give Nr_of_files == 8
//   menus.c:175    filenames start at +sizeof(levelsetdata)+21
//                  -> the "Super Mario 68K" name sits at content offset 20,
//                     so sizeof(levelsetdata) == 20
//   map.c:608      CurrentWorld = Commonfile -> Commonfile indexes the slot
//                  just past the worlds (8), matching "common" in the file
//   gameloop.c:373 Mode == 1 means map mode, which is how the game ships
//
// Savegames[] holds a byte offset into the levelset file (savegame.c:32
// dereferences it and savegame.c:34 assigns a size to it), NOT a flag - the
// name is misleading. A zero offset means "slot empty".

typedef struct {
  char Nr_of_files; // number of world files (common excluded)
  char Commonfile;  // index of the common file, i.e. Nr_of_files
  char Mode;        // 1 = map mode
  char CurrentWorld;
  unsigned short Compatibility; // checked against `compatibility` in all.h
  unsigned short Savegames[3];  // offset of each save slot, 0 = empty
  unsigned short Savegame_size[3];
  unsigned short Spare;
} levelsetdata;

extern levelsetdata Levelsetdata;

#endif
