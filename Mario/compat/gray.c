#include "gray.h"
#include "graph.h"

#include <emscripten.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// Hands the RGBA buffer to the canvas, then suspends via JSPI until the frame
// is due. The calculator ran the game at 20fps, so pace to a 50ms period.
//
// Note for the preprocessor's sake: a top-level comma in an EM_*_JS body would
// split the macro arguments, since only parentheses are balanced. Keep commas
// inside parentheses.
EM_ASYNC_JS(void, pageflip_sync, (const void *rgba), {
	const ctx = Module.canvas.getContext("2d");
	const view = new Uint8ClampedArray(HEAPU8.buffer, rgba, 3840 * 8 * 4);
	ctx.putImageData(new ImageData(view, 240, 128), 0, 0);

	if (globalThis.__smPrevFrame === undefined) globalThis.__smPrevFrame = 0;
	let now = performance.now();
	let elapsed;
	while (Math.round((elapsed = now - globalThis.__smPrevFrame)) < 50) {
		now = await new Promise((resolve) => requestAnimationFrame(resolve));
	}
	globalThis.__smPrevFrame = now - (elapsed % 50);
});

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
	render(GrayDBufGetActivePlane(LIGHT_PLANE), GrayDBufGetActivePlane(DARK_PLANE));
	pageflip_sync(renderbuf);
}