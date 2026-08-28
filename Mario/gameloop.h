
#ifndef __gameloop__
#define __gameloop__

#define map_speed 4

#ifdef speedtest
volatile short Fps; // for fps measurement
volatile char counter;
volatile short Update_frame_counter;
#endif

extern char *Filenames;

extern char Levelsetfile[9];

void Playloop();

void Gameloop();

short RunGame(char Saveslot);

void Play_level();

#endif