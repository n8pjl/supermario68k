#pragma once

#include <stdlib.h>

// We're running in a browser, so let's use WASM->JS calls to load data rather than embedding it in the binary and then fixing the parsing code.
// This will need extensive testing, and for existing files to be ported

enum menu_string {
	main_menu_text,
	load_menu_text,
	options_menu_text,

	mid_game_map_text,

	save_menu_text,
	game_over_text,
	overwrite_text,
};

extern void get_menu_string(enum menu_string menu_string, char **arr,
			    size_t arr_len);

void free_menu_strings(char **arr, size_t arr_len);