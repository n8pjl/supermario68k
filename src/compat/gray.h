#pragma once

#include <stdint.h>
#define GRAYDBUFFER_SIZE 7688

// The calculator drew at 30fps, but bought speed in the levels by skipping
// every fourth frame, so their logic ticked at 40. The browser draws them all,
// so the levels run at that 40 and the map keeps the pace it always had.
// Whichever applies is picked where the scene draws: Playloop and Render_map.
#define FPS 40
#define MAP_FPS 30

enum GrayPlanes { LIGHT_PLANE = 0, DARK_PLANE = 1 };

#define GrayOn() (1)
#define GrayOff() \
	do {      \
	} while (0)

void SetFrameRate(int16_t fps);
void DelayNFrames(uint16_t frames);

void GrayDBufInit(void *buf);
void *GrayDBufGetActivePlane(int16_t plane);
void *GrayDBufGetHiddenPlane(int16_t plane);

void GrayDBufToggleSync(void);
void GrayDBufRefresh(void);