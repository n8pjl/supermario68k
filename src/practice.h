#pragma once

// The practice room: what lets the shell put the player in a world, move them
// around its map at will, write off a level they do not want to play, and hand
// them a powerup - without playing the game up to any of it.
//
// Everything here is off unless the shell installs the hook, which it only does
// when the player asked for practice mode in the settings menu. Nothing below
// calls into this except Gameloop(), and only from the world map: a level is
// several thousand lines of state that was built by walking into it, and
// dropping a rewritten player into the middle of one is how a practice mode
// stops being bug-for-bug the game it is practising. So a request that arrives
// while a level is being played is not lost, it is simply not asked for until
// the map is back - the shell holds it, and hands it over on the next poll.
//
// Named "practice" rather than "debug" because `debug` is a macro in this
// codebase: the original's own debug build is #ifdef debug, and a namespace of
// that name would stop compiling the moment anyone defined it.
namespace practice
{

// Asks the shell whether it wants anything, and does it. Called once per frame
// of the map loop, where the player is standing on the overworld and every
// piece of state this touches is between uses:
//
//   - the world and the map it loads, which the map loop reloads for itself
//     whenever a world is finished,
//   - the map position, which is the player's own to move,
//   - the square they are standing on, which the map rewrites itself whenever a
//     level is beaten,
//   - the powerup, which the map's item list already sets from here,
//   - the item list itself.
//
// While the shell asks for free roaming, this also takes the map's input away
// from the game for the frame - through Passified, which is the game's own way
// of saying the player does not act on this one - and moves them itself. Two
// things moving one player is how they end up off the end of a road.
//
// Costs one call into JS per map frame while practice mode is on, and one
// property lookup on the first frame while it is off.
void poll();

}
