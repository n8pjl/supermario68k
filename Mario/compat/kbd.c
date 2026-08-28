#include "kbd.h"

#include <emscripten.h>

// scankeys.c reads the calculator's 7-row key matrix directly. The JS side
// maintains the same bit layout from keyboard events; row 0 is the OR of all
// rows, which is what the "any key" waits in scankeys.c rely on.
unsigned short _rowread(short row)
{
	return EM_ASM_INT({
		return (typeof keyMatrix === "undefined") ? 0 : (keyMatrix[$0] | 0);
	}, row);
}
