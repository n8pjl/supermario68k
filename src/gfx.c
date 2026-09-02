#include "gfx.h"
#include "compat/assets.h"
#include "items.h"
#include "render.h"
#include "smallgames.h"
#include <stdint.h>

const uint8_t *Bg_file_data;

inline int16_t Load_gfx_from_file()
{ // sets up the pointers to gfx arrays in external files

	const struct asset *File;

	// New: V 1.01 Changed "ma_tiles" to "mario\\ma_tiles"
	if (!(File = Asset_find("ma_tiles"))) {
		return 2; // error "failed to open gfx file"
	}

	// The tile file is one run of 16-bit tables, so the cursor walks it in
	// words: each step is the length of the region just handed out, in the
	// units gfx.h counts it in.
	const uint16_t *Tiles = (const uint16_t *)File->data;

	Fg_plane.p.sprites = Tiles;
	Tiles += 32 * Nr_of_fg_tiles;

	Fg_mask.p.sprites = Tiles;
	Tiles += 16 * Nr_of_tilemasks;

	Bg_plane.sprites = Tiles;
	Tiles += 32 * Nr_of_bg_tiles;

	Fg_plane.tabanim = Tiles;
	Tiles += Nr_of_fg_animations * 4;

	Fg_mask.tabanim = Tiles;
	Tiles += Nr_of_fg_animations * 4;

	Map_plane.p.sprites = Tiles;
	Tiles += 32 * Nr_of_map_tiles;

	Map_plane.tabanim = Tiles;

	// New: V 1.01 Changed "ma_sprts" to "mario\\ma_sprts"
	if (!(File = Asset_find("ma_sprts"))) {
		return 2; // error "failed to open gfx file"
	}

	const uint16_t *Sprts = (const uint16_t *)File->data;

	Mariosprites = Sprts;
	Sprts += Size_of_mariosprites;

	Mariomasks = Sprts;
	Sprts += Size_of_mariomasks;

	Marioanimtab = (const int16_t (*)[11])Sprts;
	Sprts += Size_of_marioanimtab;

	Enemysprites = Sprts;
	Sprts += Size_of_enemysprites;

	// The one region counted in bytes rather than words: the smallsprites are
	// 8 pixels wide, one byte per row, which is also why mkdata.py leaves them
	// unswapped.
	Smallsprites = (const uint8_t *)Sprts;
	Sprts = (const uint16_t *)(Smallsprites + Size_of_smallsprites);

	Sprites = Sprts;
	Sprts += Size_of_sprites;

	Itemsprites = Sprts;
	Sprts += Size_of_itemsprites;

	Boss_sprites = Sprts;
	Sprts += Size_of_boss_sprites;

	Games = (const char *)Sprts;

	if (!(File = Asset_find("ma_bckgr"))) {
		return 2; // error "failed to open gfx file"
	}

	Bg_file_data = File->data;

	return 0; // success!
}
