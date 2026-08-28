#pragma once

#include <stdint.h>
// Bind the browser's keys to the calculator's key matrix and start listening.
// Call once, before anything reads the keyboard.
void kbd_init(void);

// scankeys.c reads the key matrix directly via _rowread. kbd_init() has the JS
// side maintain the same bit layout from keyboard events, so scankeys.c itself
// is unchanged.
uint16_t _rowread(int16_t row);

// Hand control back to the browser for one frame. The game's key-wait loops
// were written for hardware, where a tight poll is harmless; in a browser a
// non-yielding loop pegs the main thread, so no key event can ever arrive and
// the loop can never exit. Implemented with JSPI, so callers stay synchronous.
void browser_yield(void);
