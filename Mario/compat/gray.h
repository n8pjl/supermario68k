#pragma once

#define GRAYDBUFFER_SIZE 7688

enum GrayPlanes {LIGHT_PLANE = 0, DARK_PLANE = 1};

#define GrayOn() (1)
#define GrayOff() do {} while (0)

void GrayDBufInit(void *buf);
void *GrayDBufGetActivePlane(short plane);
void *GrayDBufGetHiddenPlane(short plane);

void GrayDBufToggleSync(void);
void GrayDBufRefresh(void);