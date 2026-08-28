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
EM_ASYNC_JS(void, pageflip_sync, (const void *rgba, int w, int h), {
	const canvas = Module.canvas;

	// The canvas is sized from here rather than from the page, so the shell
	// does not have to know which calculator this was built for. Assigning
	// width/height clears the canvas, so only do it when it changed.
	if (canvas.width !== w || canvas.height !== h) {
		canvas.width = w;
		canvas.height = h;
		canvas.style.width = (w * 3) + "px";
		canvas.style.height = (h * 3) + "px";
	}

	const ctx = canvas.getContext("2d");
	const view = new Uint8ClampedArray(HEAPU8.buffer, rgba, w * h * 4);
	ctx.putImageData(new ImageData(view, w, h), 0, 0);

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

// Expand the two bit planes into RGBA for the canvas. Only the visible part of
// each plane row is walked - on a TI-89 that is the leftmost 20 of the 30
// bytes, and the top 100 of the 128 rows - so what reaches the canvas is what
// the calculator's LCD would have shown.
static uint8_t renderbuf[LCD_HEIGHT][LCD_WIDTH][4];
static void render(uint8_t *restrict light, uint8_t *restrict dark)
{
	for (int y = 0; y < LCD_HEIGHT; y++) {
		for (int bx = 0; bx < LCD_LINE_BYTES; bx++) {
			uint8_t l = ~light[y * PLANE_STRIDE + bx];
			uint8_t d = ~dark[y * PLANE_STRIDE + bx];

			for (int j = 0; j < 8; j++) {
				uint8_t color =
					(((l >> (7 - j)) & 1) * 0b10101010) |
					(((d >> (7 - j)) & 1) * 0b01010101);
				uint8_t *px = renderbuf[y][bx * 8 + j];

				px[0] = color;
				px[1] = color;
				px[2] = color;
				px[3] = 255;
			}
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
	pageflip_sync(renderbuf, LCD_WIDTH, LCD_HEIGHT);
}