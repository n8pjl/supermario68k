#pragma once

#define nr_of_bounching_tiles 2

struct bounching_tile {
  short X; // Xpos
  short Y; // Ypos

  // char Active;			//timer, 0=inactive
  short Active; // timer, 0=inactive
  // Changing Active to short saved bytes
};

extern struct bounching_tile Bounching_tiles[nr_of_bounching_tiles];

void Add_bounching_tile(short X, short Y);
