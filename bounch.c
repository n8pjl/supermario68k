#include "bounch.h"
#include <stdint.h>

struct bounching_tile Bounching_tiles[nr_of_bounching_tiles];

void Add_bounching_tile(int16_t X, int16_t Y) {
  int16_t C;

  // X = 16*(X/16);
  // Y = 16*(Y/16)-8;

  for (C = 0; C < nr_of_bounching_tiles; C++) {
    if (!Bounching_tiles[C].Active) {

      Bounching_tiles[C].Active = 1;
      Bounching_tiles[C].X = (X & 0xfff0);
      Bounching_tiles[C].Y = (Y & 0xfff0) - 8;
      return;
    };
  };
};
