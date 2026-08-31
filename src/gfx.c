#include "gfx.h"
#include "compat/tios.h"
#include "compat/utils.h"
#include "items.h"
#include "render.h"
#include "savegame.h"
#include "smallgames.h"
#include <stdint.h>
#include <string.h>

HANDLE Tilefile_sym_h, Spritefile_sym_h, Bg_file_sym_h;
char *Texts;

inline int16_t Load_gfx_from_file()
{ // sets up the pointers to gfx arrays in external files

	HANDLE Temp;

	// New: V 1.01 Changed "ma_tiles" to "mario\\ma_tiles"
	if (!(Temp = Tilefile_sym_h =
		      File_get_pointer_and_lock("mario\\ma_tiles"))) {
		return 2; // error "failed to open gfx file"
	}

	Fg_plane.p.sprites = HeapDeref(Temp) /*+ sizeof(tilefiledata)*/ + 2;

	Fg_mask.p.sprites =
		Fg_plane.p.sprites +
		32 * /*Tilefiledata.*/ Nr_of_fg_tiles * sizeof(uint16_t);

	Bg_plane.sprites =
		Fg_mask.p.sprites + 16 *
					    /*Tilefiledata.*/ Nr_of_tilemasks *
					    sizeof(uint16_t);

	Fg_plane.tabanim =
		Bg_plane.sprites +
		32 * /*Tilefiledata.*/ Nr_of_bg_tiles *
			sizeof(uint16_t); // Tilemasks + 16*Gfxfiledata.Nr_of_tilemasks;

	Fg_mask.tabanim =
		Fg_plane.tabanim +
		/*Tilefiledata.*/ Nr_of_fg_animations * 4 * sizeof(uint16_t);

	Map_plane.p.sprites =
		Fg_mask.tabanim +
		/*Tilefiledata.*/ Nr_of_fg_animations * 4 * sizeof(uint16_t);

	Map_plane.tabanim =
		Map_plane.p.sprites +
		32 * /*Tilefiledata.*/ Nr_of_map_tiles * sizeof(uint16_t);

	// New: V 1.01 Changed "ma_sprts" to "mario\\ma_sprts"
	if (!(Temp = Spritefile_sym_h =
		      File_get_pointer_and_lock("mario\\ma_sprts"))) {
		return 2; // error "failed to open gfx file"
	}

	Mariosprites = HeapDeref(Temp) /*+ sizeof(spritefiledata)*/ + 2;

	Mariomasks = Mariosprites + /*Spritefiledata.*/ Size_of_mariosprites;
	Marioanimtab =
		(int16_t (*)[11])((int16_t *)Mariomasks +
				  /*Spritefiledata.*/ Size_of_mariomasks);

	Enemysprites =
		(uint16_t *)Marioanimtab + /*Spritefiledata.*/
		Size_of_marioanimtab; // Mariomasks + Spritefiledata.Size_of_mariomasks;

	Smallsprites = (char *)(Enemysprites +
				/*Spritefiledata.*/ Size_of_enemysprites);

	Sprites = (int16_t *)(Smallsprites +
			      /*Spritefiledata.*/ Size_of_smallsprites);

	Itemsprites = Sprites + /*Spritefiledata.*/ Size_of_sprites;

	Boss_sprites = Itemsprites + /*Spritefiledata.*/ Size_of_itemsprites;

	Games = (char *)(Boss_sprites +
			 /*Spritefiledata.*/ Size_of_boss_sprites);

	if (!(/*Temp = */ Bg_file_sym_h =
		      File_get_pointer_and_lock("mario\\ma_bckgr"))) {
		return 2; // error "failed to open gfx file"
	}

	return 0; // success!
}
