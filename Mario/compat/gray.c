#include "gray.h"
#include "graph.h"

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// Implemented via a JS async function
extern void pageflip_sync(uint8_t *light, uint8_t *dark);

static uint8_t dbuf0[GRAYDBUFFER_SIZE] = { 0 };
static uint8_t *dbuf1;
static bool dbuf_buf_no;

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


static inline uint8_t *dbuf_active_buf(void)
{
	return dbuf_buf_no ? dbuf1 : dbuf0;
}

void GrayDBufInit(void *buf)
{
	dbuf1 = buf;
}

void *GrayDBufGetActivePlane(short plane)
{
	return dbuf_active_buf() + LCD_SIZE * !!plane;
}

void *GrayDBufGetHiddenPlane(short plane)
{
	return dbuf_active_buf() + LCD_SIZE * !plane;
}

void GrayDBufToggleSync(void)
{
	dbuf_buf_no = !dbuf_buf_no;
	pageflip_sync(GrayDBufGetActivePlane(LIGHT_PLANE), GrayDBufGetActivePlane(DARK_PLANE));
}