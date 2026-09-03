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

using Event = std::variant<RunStarted, RunAbandoned, RunEnded, WorldEntered,
			   LevelEntered, LevelCompleted>;

void report(const Event &event);

// The three below are the level's own story, told by three places that each
// know one part of it. The level being played is remembered between them, so
// that the code which knows a level has been beaten - deep in items.cpp, where
// the goal is touched and the dead boss is collected - does not have to know
// which level that was, or have it threaded down through a dozen handlers with
// no use for it.

// A level was started from the world map. Reports LevelEntered.
void entered_level(int world, int level);

// It has just been won.
//
// Reported from the moment of winning rather than from the map's check for it
// afterwards, because everything in between is the reward animation - the walk
// off the edge of the screen, the card, the boss flashing - and a split that
// lands before all that is one the player has a second to read before they have
// to act again. Only the first call for a level reports.
void cleared_level();

// It has returned to the map. `completed` is the game's own verdict on it: if
// that says the level was beaten and nothing inside it said so first - a way of
// winning that is not instrumented - the split lands here instead, late but not
// lost.
void left_level(bool completed);

}
