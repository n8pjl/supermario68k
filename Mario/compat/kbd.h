#pragma once

// The keyboard is a 7-row matrix on the calculator; scankeys.c reads it
// directly via _rowread. The JS side fills in the same bit layout from
// keyboard events, so scankeys.c itself is unchanged.
unsigned short _rowread(short row);
