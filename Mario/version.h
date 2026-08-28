#pragma once

// Which calculator to build for. The Makefile passes exactly one of
// PRODUCE_TI89_CODE, PRODUCE_TI92PLUS_CODE or PRODUCE_V200_CODE on the command
// line - see CALC there - rather than it being picked here, because the choice
// also selects which of the Bin/ data sets gets packaged, and the two must
// agree. compat/graph.h raises the error if none was passed.

// Raise this number if new versions with new features are released
#define compatibility 1
