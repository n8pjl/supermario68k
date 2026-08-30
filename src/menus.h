#pragma once

#include <stdint.h>
enum Menus {
  none = 0,
  mainMenu = 1,
  load = 2,
  options = 3

};

/*char*/ int16_t doMenu(char *Menudata, int16_t Nr_of_options);

int16_t Menus();

void Paste_saveslot_text(char *Buffer, char *Text);
// void Paste_saveslot_text(char* Buffer,char* Text);//short L);

#define nr_of_visible_options 4
