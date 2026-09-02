#include "custom.h"
#include "control.h"
#include "level.h"
#include "scankeys.h"
#include "text.h"

struct settings Settings;

void Custom()
{
	if (Keystate.esc) {
		// handle midgame menu

		char OldBg = Leveldata.Background;
		int16_t OldBg_offset = Leveldata.Bg_offset;

		int16_t Res = do_menu("MidGameLevel");

		SetBg(OldBg /*,Bg_file_sym->handle*/);
		Leveldata.Bg_offset = OldBg_offset;

		if (Res == 2) {
			Exit = 9;
		}

		WaitKeyReleased();
	}
}
