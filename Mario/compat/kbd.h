#pragma once

// The keyboard is a 7-row matrix on the calculator; scankeys.c reads it
// directly via _rowread. The JS side fills in the same bit layout from
// keyboard events, so scankeys.c itself is unchanged.
unsigned short _rowread(short row);

// Hand control back to the browser for one frame. The game's key-wait loops
// were written for hardware, where a tight poll is harmless; in a browser a
// non-yielding loop pegs the main thread, so no key event can ever arrive and
// the loop can never exit. Implemented with JSPI, so callers stay synchronous.
void browser_yield(void);
