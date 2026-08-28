#include "tilemap.h"
#include "graph.h"

// Port of the old TileMap engine's gray path. The shipped TileMap.a is m68k
// COFF and cannot be linked here, so these are reimplementations.
//
// Tile rows arrive already byte-swapped - tools/mkdata.py converts the 16-bit
// regions of the GFX blobs at build time - so they can be read natively. The
// big virtual screen this engine fills is a different matter: like the planes
// it is a byte-addressed bitmap with the leftmost pixel in bit 7 of the first
// byte, so 16-bit words in it are assembled a byte at a time. DrawBuffer_MASK
// in extgraph.c reads it back the same way round.
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
