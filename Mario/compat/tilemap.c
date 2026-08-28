#include "tilemap.h"
#include "graph.h"

// Port of the old TileMap engine's gray path. The shipped TileMap.a is m68k
// COFF and cannot be linked here, so these are reimplementations.
//
// NOTE: the tile and sprite data is big-endian m68k data. Everything in this
// file that reads 16-bit pixel rows out of the tile arrays must do so
// big-endian; that is deliberate and is why the blobs are not byte-swapped.
//
// TODO: implement. Stubbed so the game links and the loop can be exercised.

void DrawGrayBuffer2B_RPLC(const void *src, unsigned short x, unsigned short y,
			   void *lightplane, void *darkplane)
{
	(void)src; (void)x; (void)y; (void)lightplane; (void)darkplane;
}

void DrawGrayBuffer2B_OR(const void *src, unsigned short x, unsigned short y,
			 void *lightplane, void *darkplane)
{
	(void)src; (void)x; (void)y; (void)lightplane; (void)darkplane;
}


void DrawGrayPlane16B2B(unsigned short x, unsigned short y, Plane *plane,
			void *lightplane, void *darkplane, TM_GrayMode mode)
{
	(void)x; (void)y; (void)plane; (void)lightplane; (void)darkplane; (void)mode;
}

void DrawGrayPlane16B2B_ROLL(unsigned short x, unsigned short y, Plane *plane,
			     void *lightplane, void *darkplane, TM_GrayMode mode)
{
	(void)x; (void)y; (void)plane; (void)lightplane; (void)darkplane; (void)mode;
}

void DrawGrayAnimatedPlane16B2B(unsigned short x, unsigned short y,
				AnimatedPlane *plane, void *lightplane,
				void *darkplane, TM_GrayMode mode)
{
	(void)x; (void)y; (void)plane; (void)lightplane; (void)darkplane; (void)mode;
}
