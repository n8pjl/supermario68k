#pragma once

#include <stdint.h>
#define GRAYDBUFFER_SIZE 7688

#define DEFAULT_FPS 30

enum GrayPlanes {LIGHT_PLANE = 0, DARK_PLANE = 1};

#define GrayOn() (1)
#define GrayOff() do {} while (0)

void DelayNFrames(uint16_t frames);

void GrayDBufInit(void *buf);
void *GrayDBufGetActivePlane(int16_t plane);
void *GrayDBufGetHiddenPlane(int16_t plane);

void GrayDBufToggleSync(void);
void GrayDBufRefresh(void);