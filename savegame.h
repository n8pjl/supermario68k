#pragma once

#include "compat/utils.h"

void Savegame(char Slot);
void Loadgame(char Slot /* , char* Raw*/);

// HANDLE Levelsetfile_get_pointer_and_lock();

HANDLE File_get_pointer_and_lock(const char *FileName /*,SYM_ENTRY* File_sym*/);
