#pragma once

#include <stdint.h>
#define map_speed 4

#ifdef speedtest
volatile int16_t Fps; // for fps measurement
volatile char counter;
volatile int16_t Update_frame_counter;
#endif

extern char *Filenames;

extern char Levelsetfile[9];

void Playloop();

void Gameloop();

int16_t RunGame(char Saveslot);

void Play_level();
