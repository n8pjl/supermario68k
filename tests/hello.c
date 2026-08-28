#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define LCD_SIZE 3840

static uint8_t light[LCD_SIZE], dark[LCD_SIZE];

static uint8_t renderbuf[LCD_SIZE][8][4];

static void render(uint8_t *restrict light, uint8_t *restrict dark)
{
	for (int i = 0; i < LCD_SIZE; i++) {
		uint8_t l = ~light[i];
		uint8_t d = ~dark[i];
		for (int j = 0; j < 8; j++) {
			uint8_t color = (((l >> (7 - j)) & 1) * 0b01010101) |
					(((d >> (7 - j)) & 1) * 0b10101010);

			renderbuf[i][j][0] = color;
			renderbuf[i][j][1] = color;
			renderbuf[i][j][2] = color;
			renderbuf[i][j][3] = 255;
		}
	}
}

extern void pageflip_sync(uint8_t *renderbuf);

int main()
{
	uint8_t i = 0, j = 0;
	while (true) {
		memset(light, i, LCD_SIZE);
		memset(dark, j, LCD_SIZE);
		i += 5;
		j += 7;
		render(light, dark);
		pageflip_sync((uint8_t *)&renderbuf);
		//puts("returned");
		if (i == 200) {
			for (int i = 0; i < 1000; i++) {
				puts("delaying");
			}
		}
	}

	return 0;
}
