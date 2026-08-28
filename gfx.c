#include "gfx.h"
#include "compat/tios.h"
#include "compat/utils.h"
#include "items.h"
#include "render.h"
#include "savegame.h"
#include "smallgames.h"
#include <string.h>

// SYM_ENTRY *Tilefile_sym, *Spritefile_sym, *Bg_file_sym, *TextFile_sym;
HANDLE Tilefile_sym_h, Spritefile_sym_h, Bg_file_sym_h, TextFile_sym_h;
struct gametextdata GameTextData;
char *Texts;

inline short
Load_gfx_from_file() { // sets up the pointers to gfx arrays in external files

  HANDLE Temp;

  // New: V 1.01 Changed "ma_texts" to "mario\\ma_texts"
  if (!(/*Temp =*/TextFile_sym_h =
            File_get_pointer_and_lock("mario\\ma_texts"))) {
    return 7; // error "failed to open txt file"
  }

  Texts = HeapDeref(/*Temp*/ TextFile_sym_h) + sizeof(struct gametextdata) + 2;

  memcpy(&GameTextData, Texts - sizeof(struct gametextdata),
         sizeof(struct gametextdata)); // same as above, but a little smaller

  // New: V 1.01 Changed "ma_tiles" to "mario\\ma_tiles"
  if (!(Temp = Tilefile_sym_h = File_get_pointer_and_lock("mario\\ma_tiles"))) {
    return 2; // error "failed to open gfx file"
  }

  Fg_plane.p.sprites = HeapDeref(Temp) /*+ sizeof(tilefiledata)*/ + 2;

  Fg_mask.p.sprites =
      Fg_plane.p.sprites +
      32 * /*Tilefiledata.*/ Nr_of_fg_tiles * sizeof(unsigned short);

  Bg_plane.sprites = Fg_mask.p.sprites + 16 *
                                             /*Tilefiledata.*/ Nr_of_tilemasks *
                                             sizeof(unsigned short);

  Fg_plane.tabanim =
      Bg_plane.sprites +
      32 * /*Tilefiledata.*/ Nr_of_bg_tiles *
          sizeof(unsigned short); // Tilemasks + 16*Gfxfiledata.Nr_of_tilemasks;

  Fg_mask.tabanim = Fg_plane.tabanim + /*Tilefiledata.*/ Nr_of_fg_animations *
                                           4 * sizeof(unsigned short);

  Map_plane.p.sprites =
      Fg_mask.tabanim +
      /*Tilefiledata.*/ Nr_of_fg_animations * 4 * sizeof(unsigned short);

  Map_plane.tabanim =
      Map_plane.p.sprites +
      32 * /*Tilefiledata.*/ Nr_of_map_tiles * sizeof(unsigned short);

  // New: V 1.01 Changed "ma_sprts" to "mario\\ma_sprts"
  if (!(Temp = Spritefile_sym_h =
            File_get_pointer_and_lock("mario\\ma_sprts"))) {
    return 2; // error "failed to open gfx file"
  }

  Mariosprites = HeapDeref(Temp) /*+ sizeof(spritefiledata)*/ + 2;

  Mariomasks = Mariosprites + /*Spritefiledata.*/ Size_of_mariosprites;
  Marioanimtab = (short (*)[11])((short *)Mariomasks +
                                 /*Spritefiledata.*/ Size_of_mariomasks);

  Enemysprites =
      (unsigned short *)Marioanimtab + /*Spritefiledata.*/
      Size_of_marioanimtab; // Mariomasks + Spritefiledata.Size_of_mariomasks;

  Smallsprites =
      (char *)(Enemysprites + /*Spritefiledata.*/ Size_of_enemysprites);

  Sprites = (short *)(Smallsprites + /*Spritefiledata.*/ Size_of_smallsprites);

  Itemsprites = Sprites + /*Spritefiledata.*/ Size_of_sprites;

  Boss_sprites = Itemsprites + /*Spritefiledata.*/ Size_of_itemsprites;

  Games = (char *)(Boss_sprites + /*Spritefiledata.*/ Size_of_boss_sprites);

  if (!(/*Temp = */ Bg_file_sym_h =
            File_get_pointer_and_lock("mario\\ma_bckgr"))) {
    return 2; // error "failed to open gfx file"
  }

  return 0; // success!
}
