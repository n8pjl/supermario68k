#include "kbd.h"

#include <emscripten.h>

// The calculator's keyboard is a 10-row matrix. _rowread takes a row MASK, not
// a row index: a 0 bit selects that row, and the result is the OR of every
// selected row's key bits. _rowread(0) therefore selects all rows and answers
// "is any key down", which is what the WaitKeyPress/WaitKeyReleased loops in
// scankeys.c rely on.
//
// The JS side maintains keyMatrix[] with the same bit layout, so scankeys.c is
// unchanged.
unsigned short _rowread(short rowmask)
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

EM_ASYNC_JS(void, browser_yield, (void), {
	await new Promise((resolve) => requestAnimationFrame(resolve));
});
