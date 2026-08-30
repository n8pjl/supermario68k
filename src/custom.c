#include "custom.h"
#include "control.h"
#include "gfx.h"
#include "level.h"
#include "menus.h"
#include "scankeys.h"
#include <stdint.h>

struct settings Settings;

inline void Custom()
{
	if (Keystate.esc) {
		// handle midgame menu

		char OldBg = Leveldata.Background;
		int16_t OldBg_offset = Leveldata.Bg_offset;

		int16_t Res = doMenu(Texts + GameTextData.MidGameLevel, 2);

		SetBg(OldBg /*,Bg_file_sym->handle*/);
		Leveldata.Bg_offset = OldBg_offset;

		if (Res == 2) {
			Exit = 9;
		}

		WaitKeyReleased();
	};
}
