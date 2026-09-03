#include "text.h"
#include "compat/assets.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "compat/utils.h"
#include "control.h"
#include "gameloop.h"
#include "level.h"
#include "levelset.h"
#include "render.h"
#include "savegame.h"
#include "scankeys.h"
#include "speedrun.h"
#include "stringcopy.h"
#include "titlescreen.h"
#include "version.h"
#include <assert.h>
#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <emscripten/em_macros.h>
#include <string.h>

struct menu_text {
	uint32_t no_options;
	const char *title;
	const char *options[];
};

// Special-case load_menu and Save in here. The string processing is way easier in JS.
static struct menu_text *get_menu_text(const char *menu)
{
	_Static_assert(sizeof(void *) == 4,
		       "JS code assumes pointers are 32-bits");

	return (struct menu_text *)EM_ASM_PTR(
		{
			// clang-format off
			const menuId = AsciiToString($0);
			let arr = Module.maTexts.menus[menuId];
			if (menuId === "load_menu" || menuId === "Save") {
				const oldArr = arr;
				arr = [oldArr[0]];
				for (let i = 0; i < 3; i++) {
					if (HEAPU16[$1 / 2 + i] === 0)
						arr.push(oldArr[2]);
					else
						arr.push(oldArr[1].slice(0, oldArr[2].length - 2) + (i + 1));
				}
			}
			let size = 4 + arr.length * 4;
			for (const str of arr) size += str.length + 1;
			const addr = _malloc(size);
			let cursor = addr + arr.length * 4 + 4;
			HEAPU32[addr / 4] = arr.length - 1;

			for (let i = 0; i < arr.length; i++) {
				HEAPU32[addr / 4 + i + 1] = cursor;
				for (let j = 0; j < arr[i].length; j++)
					HEAPU8[cursor++] = arr[i].charCodeAt(j);
				HEAPU8[cursor++] = 0;
			}

			return addr;
			// clang-format on
		},
		menu, &Levelsetdata.Savegames);
}

char *get_string_locale(const char *str_id)
{
	_Static_assert(sizeof(void *) == 4,
		       "JS code assumes pointers are 32-bits");

	char *str = (char *)EM_ASM_PTR(
		// clang-format off
		{
			const str = Module.maTexts.strings[AsciiToString($0)];
			const out = _malloc(str.length + 1);
			let i = 0;
			while (i < Math.min(str.length, 255)) {
				HEAPU8[out + i] = str.charCodeAt(i);
				i++;
			}
			HEAPU8[out + i] = 0;
			return out;
		},
		// clang-format on
		str_id);

	return str;
}
EM_JS_DEPS(get_string_locale, "$AsciiToString,malloc")

static int16_t do_menu_inner(struct menu_text *options)
{
	int16_t current_selection = 1;

	assert(options->no_options <= nr_of_visible_options);

	SetBg(0);

	while (true) {
		// draw bg
		DrawBg(40, 40);

		// draw title
		DrawString(30, 5, options->title, A_REPLACE | A_SHADOWED,
			   F_8x10);

		DrawString(24, screen_height - 6,
			   "http://tifreakware.net/lachprog/", A_REPLACE,
			   F_4x6);

		for (uint32_t i = 0; i < options->no_options; i++) {
			DrawString(30, 25 + 15 * i, options->options[i],
				   A_NORMAL | A_SHADOWED, F_6x8);
		}

		DrawMarioCursor(10, 25 - 20 + 15 * current_selection);

		GrayDBufToggleSync_SetPointers();

		WaitKeyPress();

		ScanKeys();

		if (Keystate.esc) {
			free(options);
			return 0;
		}
		if (Keystate.enter || Keystate.jump) {
			free(options);
			return current_selection;
		}

		if (Keystate.up) {
			current_selection--;
			if (current_selection < 1)
				current_selection = options->no_options;
		}
		if (Keystate.down) {
			current_selection++;
			if (current_selection > options->no_options)
				current_selection = 1;
		}
	}

	return 0;
}

int16_t do_menu(const char *menu_id)
{
	struct menu_text *options = get_menu_text(menu_id);

	int16_t ret = do_menu_inner(options);

	free(options);
	return ret;
}

int16_t Menus()
{
	int16_t C = 0;
	int16_t Res;
	char Menu = mainMenu;

	char *TempBuffer = Fg_plane.p.big_vscreen;

	int16_t Offset =
		StringCopy(TempBuffer /*Fg_plane.p.big_vscreen*/, "Levelset:");

	// find all levelsets
	for (const struct asset *Set = Asset_next_tagged(NULL, "MLST"); Set;
	     Set = Asset_next_tagged(Set, "MLST")) {
		// The levelset's title, which is what the menu would show, sits
		// just past the header.
		Offset += StringCopy(
			TempBuffer /*Fg_plane.p.big_vscreen*/ + Offset,
			(const char *)Set->data + sizeof(struct levelsetdata));

		StringCopy(TempBuffer /*Fg_plane.p.big_vscreen*/ + 512 + 9 * C,
			   Set->name);

		C++;
	}

	// char Levelset[9];

	// Error: no levelset found possible

	// if (C > 1) { // more than one levelset detected

	// 	Res = doMenu(TempBuffer /*Fg_plane.p.big_vscreen*/,
	// 		     C); // let the user choose which to use

	// 	// strcpy(Levelset,Fg_plane.p.big_vscreen+512+(C-1)*9);
	// 	StringCopy(Levelsetfile, TempBuffer /*Fg_plane.p.big_vscreen*/ +
	// 					 512 + (Res - 1) * 9);

	// } else { // only one levelset found
	if (C == 0) {
		// strcpy(Levelset,Fg_plane.p.big_vscreen+512);
		return 5;
	} else {
		StringCopy(Levelsetfile,
			   TempBuffer /*Fg_plane.p.big_vscreen*/ + 512);
		Res = 1;
	}
	//}

	// Load levelsetdata!
	const struct asset *Set = Asset_find(Levelsetfile);

	if (!Set) {
		return 5;
	}

	memcpy(&Levelsetdata, Set->data, sizeof(struct levelsetdata));

	memcpy(&Commonfilename,
	       Set->data + sizeof(struct levelsetdata) + 21 +
		       9 * Levelsetdata.Commonfile,
	       9);

	if (Levelsetdata.Compatibility >
	    compatibility) { // If chosen levelset is designed for newer versions of
		// the game...
		return 8; // Levelset not compatible. New version of the game must be
		// installed
	}

	// Error: Memory possible
	if (!(Filenames = (char *)malloc(9 * (Levelsetdata.Nr_of_files)))) {
		return 1;
	}

	memcpy(Filenames, Set->data + sizeof(struct levelsetdata) + 21,
	       9 * Levelsetdata.Nr_of_files);

	// New: V 1.01 Added error check
	if (Show_titlescreen()) {
		return 3;
	}
	// End of new

	while (!Exit) {
		while (Menu == mainMenu) {
			Res = do_menu("main_menu");

			switch (Res) {
			case 1: // new game
				speedrun::report(speedrun::RunStarted{});

				RunGame(-1);

				// However that went - the ending, a game over,
				// or quitting out of the middle of a level -
				// the main menu is back and the run is over.
				speedrun::report(speedrun::RunAbandoned{});

				if (ErrorCode)
					return ErrorCode;

				break;
			case 2: // load game
				Menu = load;
				break;
			case 3: // options
				// PORT: remove key configuration.
				// Remove the menu option once text system is refactored.
				//Key_configure();
				break;
			/*case 4://help

      break;*/
			case 0: // esc
			case 4: // quit
				Exit = 1;
				Menu = none;
				break;
			}

		} // end main menu

		/*while(Menu==options){//to come

            Res = doMenu(Texts+GameTextData.Options,3);

            switch(Res){

                    case 1://statusbar

                    break;

                    case 2://controls

                    break;

                    case 0://esc
                    case 3://back
                            Menu=main;
                    break;

            };

    };//end controls menu
    */
		while (Menu == load) {
			Res = do_menu("load_menu");

			if ((Res >= 1) && (Res <= 3)) {
				if (Levelsetdata.Savegames
					    [Res -
					     1]) { // Only load if saveslot is used!
					RunGame(Res - 1);
					if (ErrorCode)
						return ErrorCode;
					Menu = mainMenu; // Go to main menu when finiched/exit
					Exit = 0;
				}

			} else {
				Menu = mainMenu;
			}

		} // end controls menu
	}

	return 0;
}
