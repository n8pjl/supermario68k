#include "tas.h"
#include "compat/gray.h"

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

EM_JS(void, tas_event, (const char *kind, size_t length), {
	// clang-format off
	Module.tas.event(UTF8ToString(kind, length));
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

void event(std::string_view kind)
{
	tas_event(kind.data(), kind.size());
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
