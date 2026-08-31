#pragma once
#include <stdbool.h>
#include <stdint.h>

enum Menus { none = 0, mainMenu = 1, load = 2, options = 3 };

char *get_string_locale(const char *str_id);

int16_t do_menu(const char *menu_id);

int16_t Menus();

#define nr_of_visible_options 4
