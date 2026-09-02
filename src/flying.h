#pragma once

#include <stdint.h>
struct flying_platform {
	int16_t X;
	int16_t Y;

	int16_t Width; // char ???

	// char Active;
	int16_t Active;

	int16_t Data0;
	int16_t Data1;
	int16_t Data2;
	int16_t Data3;

	// char PlayerOn;
	int16_t PlayerOn;
	//	short DrawMode;
	uint16_t Sprite;

	void (*Handler)(struct flying_platform *Flying_platform);
};

extern struct flying_platform *Flying_platforms;

void Handle_flying_platforms();

void Platform_handler_1(struct flying_platform *Platform);

void Platform_handler_2(struct flying_platform *Platform);

void Platform_handler_3(struct flying_platform *Platform);
// short Sign(short);
