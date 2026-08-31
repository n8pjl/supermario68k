#pragma once

#include <stdint.h>
#define nr_of_bounching_tiles 2

struct bounching_tile {
	int16_t X; // Xpos
	int16_t Y; // Ypos

	// char Active;			//timer, 0=inactive
	int16_t Active; // timer, 0=inactive
	// Changing Active to short saved bytes
};

extern struct bounching_tile Bounching_tiles[nr_of_bounching_tiles];

void Add_bounching_tile(int16_t X, int16_t Y);
