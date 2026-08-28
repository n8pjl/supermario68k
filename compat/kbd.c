#include <stdint.h>
#include "kbd.h"

#include <emscripten.h>

// The calculator's keyboard is a 10-row matrix. _rowread takes a row MASK, not
// a row index: a 0 bit selects that row, and the result is the OR of every
// selected row's key bits. _rowread(0) therefore selects all rows and answers
// "is any key down", which is what the WaitKeyPress/WaitKeyReleased loops in
// scankeys.c rely on.
//
// Which row and bit a key sits at differs by model, and scankeys.c has a
// separate Scankeys() per model reading its own layout, so the table below has
// to match the calculator this was built for. That is why it lives here rather
// than in the page: the same -DPRODUCE_*_CODE that picks Scankeys() picks the
// bindings, and the two cannot drift apart.
//
// The four modifiers are the slots the in-game key config offers (custom.c
// names them); the game's own defaults pick two of them for jump and run, and
// the browser keys are bound so that Space/Z is jump and Shift/X is run out of
// the box, with C and V reaching the other two slots so the config screen
// still works.
struct keybind {
	const char *code;	// KeyboardEvent.code
	uint8_t row, bit;
};

static const struct keybind keymap[] = {
#if defined(PRODUCE_TI89_CODE)
	// Row 0 carries the arrows and all four modifiers; Esc is row 6 and
	// ENTER/+/- are row 1. Defaults: jump [2nd], run [Diamond].
	{ "ArrowUp",     0, 0 }, { "ArrowLeft",  0, 1 },
	{ "ArrowDown",   0, 2 }, { "ArrowRight", 0, 3 },
	{ "Space",       0, 4 }, { "KeyZ",       0, 4 },   // [2nd]     slot 0
	{ "KeyC",        0, 5 },                           // [Shift]   slot 1
	{ "ShiftLeft",   0, 6 }, { "ShiftRight", 0, 6 },
	{ "KeyX",        0, 6 },                           // [Diamond] slot 2
	{ "KeyV",        0, 7 },                           // [Alpha]   slot 3
	{ "Escape",      6, 0 },
	{ "Enter",       1, 0 }, { "Equal",      1, 1 }, { "Minus", 1, 2 },
#elif defined(PRODUCE_TI92PLUS_CODE)
	// Defaults: jump [F1], run [F2].
	{ "ArrowLeft",   0, 4 }, { "ArrowUp",    0, 5 },
	{ "ArrowRight",  0, 6 }, { "ArrowDown",  0, 7 },
	{ "KeyC",        0, 3 },                           // [Hand]    slot 0
	{ "Space",       6, 4 }, { "KeyZ",       6, 4 },   // [F1]      slot 1
	{ "ShiftLeft",   4, 4 }, { "ShiftRight", 4, 4 },
	{ "KeyX",        4, 4 },                           // [F2]      slot 2
	{ "KeyV",        5, 4 },                           // [F5]      slot 3
	{ "Escape",      8, 6 }, { "Equal",      8, 4 },
	{ "Enter",       9, 1 }, { "Minus",      9, 0 },
#elif defined(PRODUCE_V200_CODE)
	// Defaults: jump [Hand], run [Q].
	{ "ArrowLeft",   0, 4 }, { "ArrowUp",    0, 5 },
	{ "ArrowRight",  0, 6 }, { "ArrowDown",  0, 7 },
	{ "Space",       0, 3 }, { "KeyZ",       0, 3 },   // [Hand]    slot 0
	{ "ShiftLeft",   9, 3 }, { "ShiftRight", 9, 3 },
	{ "KeyX",        9, 3 },                           // [Q]       slot 1
	{ "KeyC",        6, 4 },                           // [F1]      slot 2
	{ "KeyV",        1, 3 },                           // [W]       slot 3
	{ "Escape",      8, 6 }, { "Equal",      8, 4 },
	{ "Enter",       9, 1 }, { "Minus",      9, 0 },
#else
#error "no target calculator - build with make CALC=89|92p|v200"
#endif
};

EM_JS(void, kbd_bind, (const char *code, int16_t row, int16_t bit), {
	globalThis.keyMap[UTF8ToString(code)] = [row, bit];
});

EM_JS(void, kbd_start, (void), {
	globalThis.keyMap = {};
	globalThis.keyMatrix = new Uint16Array(10);

	const set = (e, down) => {
		const k = globalThis.keyMap[e.code];
		if (!k) return;
		if (down)
			keyMatrix[k[0]] |= 1 << k[1];
		else
			keyMatrix[k[0]] &= ~(1 << k[1]);
		e.preventDefault();
	};

	addEventListener("keydown", (e) => set(e, true));
	addEventListener("keyup", (e) => set(e, false));
	// A window that loses focus never sees the keyup, which would otherwise
	// leave the game running into a wall.
	addEventListener("blur", () => keyMatrix.fill(0));
});

void kbd_init(void)
{
	uint16_t i;

	kbd_start();
	for (i = 0; i < sizeof keymap / sizeof keymap[0]; i++)
		kbd_bind(keymap[i].code, keymap[i].row, keymap[i].bit);
}

uint16_t _rowread(int16_t rowmask)
{
	return EM_ASM_INT({
		if (typeof keyMatrix === "undefined") return 0;
		var mask = $0;
		var out = 0;
		for (var r = 0; r < 10; r++)
			if (!(mask & (1 << r))) out |= keyMatrix[r];
		return out;
	}, rowmask);
}

EM_ASYNC_JS(void, _browser_yield, (void), {
	await new Promise((resolve) => requestAnimationFrame(resolve));
});

void browser_yield(void) {
	_browser_yield();
}
