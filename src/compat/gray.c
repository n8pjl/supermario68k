#include "gray.h"
#include "../render.h"
#include "graph.h"

#include <emscripten.h>

#include <stdbool.h>
#include <stdint.h>

// Hands the RGBA buffer to the canvas. Drawing straight into the active plane
// showed up on the calculator's LCD the moment it was written, so the canvas
// needs a way to be repainted without flipping buffers - see GrayDBufRefresh.
EM_JS(void, pageflip, (const void *rgba, int16_t w, int16_t h), {
	// clang-format off
	const canvas = Module.canvas;

	// The canvas is sized from here rather than from the page, so the shell
	// does not have to know which calculator this was built for. Assigning
	// width/height clears the canvas, so only do it when it changed. How
	// large those pixels are drawn is the shell's business - it is the one
	// that knows how much room the viewport has - so it is told instead.
	if (canvas.width !== w || canvas.height !== h) {
		canvas.width = w;
		canvas.height = h;
		Module.onCanvasResize?.();
	}

	const ctx = canvas.getContext("2d");
	const view = new Uint8ClampedArray(HEAPU8.buffer, rgba, w * h * 4);
	ctx.putImageData(new ImageData(view, w, h), 0, 0);
	// clang-format on
})

// Suspends via JSPI until the frame is due, at the pace the current scene asked
// for.
//
// requestAnimationFrame is the only clock available to wait on, so the period
// is a floor and not a promise: below about 17ms on a 60Hz display, every
// frame is simply due when it is painted.
EM_ASYNC_JS(void, wait_for_frame, (double period), {
	// clang-format off
	if (globalThis.__smPrevFrame === undefined) globalThis.__smPrevFrame = 0;
	let now = performance.now();
	let elapsed;
	while (Math.round((elapsed = now - globalThis.__smPrevFrame)) < period) {
		now = await new Promise((resolve) => requestAnimationFrame(resolve));
	}
	globalThis.__smPrevFrame = now - (elapsed % period);
	// clang-format on
})

static int16_t frame_rate = FPS;

void SetFrameRate(int16_t fps)
{
	frame_rate = fps;
}

static void pageflip_wait(void)
{
	wait_for_frame(1000.0 / frame_rate);
}

void DelayNFrames(uint16_t frames)
{
	for (int16_t i = 0; i < frames; i++)
		pageflip_wait();
}

static uint8_t dbuf0[GRAYDBUFFER_SIZE] = { 0 };
static uint8_t *dbuf1;
static bool dbuf_buf_no;

// Expand the two bit planes into RGBA for the canvas. Only the visible part of
// each plane row is walked - on a TI-89 that is the leftmost 20 of the 30
// bytes, and the top 100 of the 128 rows - so what reaches the canvas is what
// the calculator's LCD would have shown.
static uint8_t renderbuf[PLANE_HEIGHT * PLANE_WIDTH][4];
static void render(uint8_t *restrict light, uint8_t *restrict dark)
{
	for (int16_t y = 0; y < screen_height; y++) {
		for (int16_t bx = 0; bx < screen_width / 8; bx++) {
			uint8_t l = ~light[y * PLANE_STRIDE + bx];
			uint8_t d = ~dark[y * PLANE_STRIDE + bx];

			for (int16_t j = 0; j < 8; j++) {
				uint8_t color =
					(((l >> (7 - j)) & 1) * 0b01010101) |
					(((d >> (7 - j)) & 1) * 0b10101010);
				uint8_t *px =
					renderbuf[y * screen_width + bx * 8 + j];

				px[0] = color;
				px[1] = color;
				px[2] = color;
				px[3] = 255;
			}
		}
	}
}

// The two buffers each hold both planes back to back, light first, the way the
// calculator's gray double buffer lays them out. Which buffer is on screen is
// what alternates - the plane's offset within a buffer does not, so both
// accessors index with the same !!plane.
static inline uint8_t *dbuf_buf(bool no)
{
	return no ? dbuf1 : dbuf0;
}

void GrayDBufInit(void *buf)
{
	dbuf1 = buf;
}

void *GrayDBufGetActivePlane(int16_t plane)
{
	return dbuf_buf(dbuf_buf_no) + LCD_SIZE * !!plane;
}

void *GrayDBufGetHiddenPlane(int16_t plane)
{
	return dbuf_buf(!dbuf_buf_no) + LCD_SIZE * !!plane;
}

void GrayDBufToggleSync(void)
{
	dbuf_buf_no = !dbuf_buf_no;
	GrayDBufRefresh();
	pageflip_wait();
}

// Repaints the canvas from the planes that are already on screen. Code that
// draws into the active plane - the world screen over the map, the level entry
// wipe - relied on the LCD showing those writes without a page flip, and would
// otherwise not appear at all until the next flip overwrote them.
void GrayDBufRefresh(void)
{
	render(GrayDBufGetActivePlane(LIGHT_PLANE),
	       GrayDBufGetActivePlane(DARK_PLANE));
	pageflip(renderbuf, screen_width, screen_height);
}
