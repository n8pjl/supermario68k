#pragma once

#include <stdint.h>
#define shell_speed 3
// 3
#define max_nr_of_shells 5

// 16 X 16 square collision detection
#define BoundsCollide16(x0, y0, x1, y1)                              \
	(((x0 + 16) > x1) && ((x1 + 16) > x0) && ((y0 + 16) > y1) && \
	 ((y1 + 16) > y0))
// Collition detection with different heights
#define BoundsCollide(x0, y0, h0, x1, y1, h1)                        \
	(((x0 + 16) > x1) && ((x1 + 16) > x0) && ((y0 + h0) > y1) && \
	 ((y1 + h1) > y0))
// Collition detection with different heights and widths
#define BoundsCollide2(x0, y0, h0, w0, x1, y1, h1, w1)               \
	(((x0 + w0) > x1) && ((x1 + w1) > x0) && ((y0 + h0) > y1) && \
	 ((y1 + h1) > y0))

struct shell {
	int16_t X;
	int16_t Y;
	/*
          char Dir;
          char Active;
  //	short Height;
          char Count;			//used as a framecounter to ensure that
  mario is not killed instantly when jumping on a shell,
                                                                          //ACTIVE
  CAN REPLACE THIS, BY COUNTING DOWN TO 1 !!!
  */
	int16_t Dir;
	int16_t Active;
	//	short Height;
	int16_t Count; // used as a framecounter to ensure that mario is not killed
	// instantly when jumping on a shell,

	struct enemy *
		Enemy; // Pointer to the enemy that might resurrect from this shell
};

extern struct shell *Shells;

void Player_bounching_shell_hadler();

void Add_shell(/*short X,short Y,*/ int16_t Dir, char Type,
	       struct enemy *Enemy);

// void Handle_collided_shells(object* Object);

void Add_killed_shell(struct shell *Shell, int16_t DirSpeed);
