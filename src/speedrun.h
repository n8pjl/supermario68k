#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

// What the game tells the shell's timer, and the whole of what it tells it.
// Nothing on this side knows what a split is: the game reports what happened
// and speedrun.ts decides, from the category being timed, whether that closes
// one. That is the split which keeps a new route or category from needing a
// change down here.
//
// Each event is a struct carrying its own payload and the `kind` it travels
// under. speedrun.cpp registers them with Embind, so reaching JS is a
// conversion of a value rather than a hand-marshalled block of heap writes -
// see the discriminated union at the top of speedrun.ts for the other end.
namespace speedrun
{

// "New game" chosen in the main menu, before the first world is loaded. Loading
// a save deliberately does not raise this: there is no run to time.
struct RunStarted {
	static constexpr std::string_view kind = "run-started";
};

// The game is back at the main menu. Raised however a run got there - quit,
// game over, or the ending - so a run that did not finish stops being timed. A
// run that did has already reported RunEnded and keeps the time it earned.
struct RunAbandoned {
	static constexpr std::string_view kind = "run-abandoned";
};

// Bowser is down and the ending has taken over. Raised on the last frame the
// player had control on, before the victory animation, which is not the
// player's time to lose.
struct RunEnded {
	static constexpr std::string_view kind = "run-ended";
};

// A world's map is loaded and its title card is up. `world` is
// Levelsetdata.CurrentWorld, counted from zero, so arriving at the world shown
// as "WORLD 2" reports 1 - and a warp pipe reports whichever world it jumped
// to, which is how a warp route tells itself apart from a warpless one.
struct WorldEntered {
	static constexpr std::string_view kind = "world-entered";

	int world;
};

// A warp was taken: a pipe that jumps to another world, or the whistle that
// goes to the warp zone in the first place. `world` is the one it lands in -
// Levelsetdata.Commonfile for the whistle, since the warp zone is a map like
// any other.
//
// Reported because a warp cannot always be told from the world numbers either
// side of it: a warp to the very next world looks exactly like having finished
// the one before. A category that forbids warping has to be told, rather than
// left to infer it from a run that is careful enough to hide it.
struct WarpTaken {
	static constexpr std::string_view kind = "warp-taken";

	int world;
};

// A level was started from the world map. `level` is its index within the
// world's file - the map tile it was entered from, less levels_low - so the
// castle that ends a world is level 7 and Bowser's is level 19. Bonus rooms
// reached from the map (the mushroom and game houses) and the passages behind
// the pipes are not levels and do not report.
struct LevelEntered {
	static constexpr std::string_view kind = "level-entered";

	int world;
	int level;
};

// That level was beaten, rather than left by dying or by quitting out. Raised
// only where the game itself concluded as much, so the two can never disagree.
struct LevelCompleted {
	static constexpr std::string_view kind = "level-completed";

	int world;
	int level;
};

// An overworld monster was walked into: the Hammer Bros. and the Boomerang,
// Fire and Sledge Bros. that share their handler. They are map objects rather
// than map tiles, so they are entered from Fight_monster() rather than from the
// map's level path, and `monster` is the object's index in the world's map
// file. That index, and not the level, is what tells two of them apart: the
// arena they drop into is a level of the common file, shared between every
// monster of that kind in the game.
struct MonsterFought {
	static constexpr std::string_view kind = "monster-fought";

	int world;
	int monster;
};

// That monster was beaten, rather than run from or died to. Raised where the
// game concluded as much, the same as a level.
struct MonsterDefeated {
	static constexpr std::string_view kind = "monster-defeated";

	int world;
	int monster;
};

using Event = std::variant<RunStarted, RunAbandoned, RunEnded, WorldEntered,
			   WarpTaken, LevelEntered, LevelCompleted,
			   MonsterFought, MonsterDefeated>;

// Thrown out of report() when the shell says the run it was timing is over
// while the game is not: a route being recorded reached the end of the category
// it was being recorded for, so there is nothing left to record and the split
// panel is showing a finished route the player is meant to read.
//
// Thrown rather than flagged because there is nowhere to check a flag. The
// event that ends a recording is reported from the middle of a level, several
// frames deep in code that has no way of saying "and stop", so the stack is
// unwound out from under it and main() catches it. The game keeps nothing that
// has to be released on the way out - the buffers it allocated are freed when
// the page drops the whole runtime - so unwinding is enough.
struct Stopped {};

// Hands one event to the shell. Throws Stopped if the shell answers that the
// game should stop, which it only does for a recording that has just finished.
void report(const Event &event);

// The four below are the level's own story, told by four places that each know
// one part of it. What is being played is remembered between them, so that the
// code which knows it has been beaten - deep in items.cpp, where the goal is
// touched and the dead boss is collected - does not have to know what it was,
// or have it threaded down through a dozen handlers with no use for it.
//
// A monster fight is one of those things being played: it is a level of the
// common file, entered from the map and returned from the same way, so the two
// below it are shared with it and report whichever of the two was entered.

// A level was started from the world map. Reports LevelEntered.
void entered_level(int world, int level);

// An overworld monster was walked into. Reports MonsterFought.
void entered_monster(int world, int monster);

// It has just been won. Reports LevelCompleted, or MonsterDefeated where what
// is being played is a monster fight.
//
// Reported from the moment of winning rather than from the map's check for it
// afterwards, because everything in between is the reward animation - the walk
// off the edge of the screen, the card, the boss flashing - and a split that
// lands before all that is one the player has a second to read before they have
// to act again. Only the first call for a level reports.
void cleared_level();

// It has returned to the map. `completed` is the game's own verdict on it: if
// that says it was beaten and nothing inside it said so first - a way of
// winning that is not instrumented - the split lands here instead, late but not
// lost.
void left_level(bool completed);

}
