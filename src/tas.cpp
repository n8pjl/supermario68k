#include "tas.h"
#include "compat/gray.h"
#include "player.h"
#include "speedrun.h"

#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>
#include <unistd.h>

// The single suspension point. Nothing may follow the await - see tas.h.
EM_ASYNC_JS(void, tas_suspend, (int boot, double period), {
	// clang-format off
	await Module.tas.suspend(!!boot, period);
	// clang-format on
})

EM_JS(bool, tas_any_action, (void), {
	// clang-format off
	return Object.values(Module.gameActions()).some(Boolean);
	// clang-format on
})

EM_JS(void, tas_event, (const char *kind, size_t length, int world, int index), {
	// clang-format off
	Module.tas.event(UTF8ToString(kind, length), world, index);
	// clang-format on
})

EM_JS(bool, tas_drawing, (void), {
	// clang-format off
	return Module.tas.drawing;
	// clang-format on
})

namespace tas
{

void frame(double period)
{
	tas_suspend(false, period);
}

void power_on()
{
	tas_suspend(true, 0);
}

bool any_action()
{
	return tas_any_action();
}

void event(std::string_view kind, int world, int index)
{
	tas_event(kind.data(), kind.size(), world, index);
}

bool drawing()
{
	return tas_drawing();
}

}

// For tas-runtime.js, which takes and restores snapshots. Everything the game
// has allocated lies below the break, and everything static below that, so a
// snapshot is the memory up to here and no further.
extern "C" EMSCRIPTEN_KEEPALIVE uintptr_t tas_heap_end(void)
{
	return (uintptr_t)sbrk(0);
}

// Repaints the canvas from the planes a restored snapshot left on screen, which
// is otherwise not shown until the game next flips.
extern "C" EMSCRIPTEN_KEEPALIVE void tas_refresh(void)
{
	GrayDBufRefresh();
}

// For tas-runtime.js: the player as the page shows it, read between frames.
// Written into a buffer of its own rather than read from struct player's
// fields in place, whose offsets are the compiler's to choose. The first
// value says whether the rest mean anything: outside a level or a monster
// fight, the player is a map sprite and these are left over from the last.
//
// The buffer is part of the memory a snapshot copies, which does no harm:
// nothing in the game reads it, so the game plays the same whatever is in it.
extern "C" EMSCRIPTEN_KEEPALIVE const int32_t *tas_player(void)
{
	static int32_t values[13];

	values[0] = speedrun::in_play();
	values[1] = Player.X;
	values[2] = Player.Y;
	values[3] = Player.Walkspeed;
	values[4] = Player.Walkspeed2;
	values[5] = Player.Jumpspeed;
	values[6] = Player.Fallspeed;
	values[7] = Player.IsJumping;
	values[8] = Player.IsFalling;
	values[9] = Player.Runcount;
	values[10] = Player.Flycount;
	values[11] = Player.Xoffset;
	values[12] = Player.Face;
	return values;
}
