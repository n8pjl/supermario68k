#pragma once

// The TAS build's half of tool-assisted play: the one place the game stops and
// hands control to the page between frames. Compiled only into the build the
// Makefile links with Asyncify (SM68K_TAS), and reached only from the handful
// of places that waited on the browser before - see tas.ts for the other half.
//
// Why Asyncify rather than the JSPI the game ships with: while the game is
// suspended under Asyncify, the whole of its call stack has been unwound into
// linear memory. Copying that memory, the stack pointer and the one pointer the
// runtime keeps to the unwound stack is then a complete snapshot, and writing
// them back is a complete restore. Under JSPI the suspended stack belongs to
// the engine, and nothing can copy it.
//
// Every suspension goes through suspend() below, and the JS that runs there
// does nothing after its await. That is load-bearing: a restore resumes the
// game at whichever suspend() the snapshot was taken in, through whichever
// one happens to be pending now, so a continuation in JS would run on behalf
// of the wrong one.
#include <string_view>

namespace tas
{

// One frame of the movie: the game has done all it does with this frame's
// input and is waiting to be let on to the next. `period` is the time the
// scene asked a frame to take, in milliseconds, which the page uses to play
// back at the game's own pace when it is not being stepped.
void frame(double period);

// The power-on state: suspends once at the top of main(), before the game has
// read anything, so that frame 0 is a snapshot like any other and a movie can
// be rewound all the way to its start without recreating the runtime.
void power_on();

// Whether any of the eight actions is held this frame, for the waits in
// scankeys.cpp, which poll a frame at a time here rather than in JS.
bool any_action();

// One of speedrun.h's events, by its kind alone, for the page's clock: the
// frame a run starts and ends on is what its real time is read from. A plain
// call rather than Embind's, for the reason speedrun::report() gives, and with
// nothing after it - see above.
void event(std::string_view kind);

// Whether this frame is being drawn. A seek replays frames as fast as they
// run, and only the one it lands on needs to reach the canvas.
bool drawing();

}
