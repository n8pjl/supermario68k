#!/usr/bin/env python3
"""Write the 100% manifest out of levels/, for the speedrun timer to check.

The 100% category is "every stage and every overworld Hammer Bros.", which is
a claim about the level set rather than about any one run: the timer cannot
tell a complete route from a route that missed world 4's second fortress
unless it is told what there was to miss. That list is here, generated, rather
than written by hand in the timer - a stage added to a map would otherwise
leave a category quietly asking for less than it says.

Two things are counted, and both come out of the world maps:

  - Stages. Every map tile in the levels range is one, the level it enters
    being the tile less levels_low; a small_castle and Bowser's castle are
    tiles in that range like any other. The airship or castle that ends a
    world is not - it is the big_castle the map walks onto, entered by
    Enter_enemy_ship(), which loads level 7 - so a world with one of those
    gets level 7 added.

  - Overworld monsters. The map objects the game fights rather than walks
    past: Handle_map_objects() sends modes 2 to 8 to Fight_monster(), and
    everything else - the ships, the boats, the houses a map event adds - is
    something else. They are named by their index in the map's object list,
    which is what tells two Bros. in one world apart; the arena they load is a
    level of the common file and is shared.

The tile numbers and the enum come out of src/map.h, so the game stays the one
place they are written down.

Usage:
  mkstages.py <json dir> <out file>          write speedrun/stages.ts
  mkstages.py --check <json dir> <out file>  is that file still what levels/
                                             says? Run on every build.
"""
import os
import sys

from mklevels import SRC, enum_names, read_json, strip_comments, tile_ranges

# world1.json is world 0: Levelsetdata.CurrentWorld counts from zero, and every
# event the game reports does too.
WORLDS = ["world%d" % i for i in range(1, 9)]

# The one level index no map tile names, because the castle or airship that
# ends a world is entered by walking onto the big_castle rather than into a
# level tile. Kept in step with speedrun::entered_level()'s caller in map.cpp
# and with CASTLE_LEVEL in speedrun/events.ts.
CASTLE_LEVEL = 7

# The map object modes Handle_map_objects() answers with Fight_monster().
MONSTER_MODES = range(2, 9)

HEADER = '''\
// The stages and the overworld monsters of every world: what a 100% run has to
// have been through, and the whole of what the rules ask for.
//
// Generated from levels/ by tools/mkstages.py, and checked against it on every
// build - see the Makefile. Do not edit: an edit here is a claim about the
// level set that the level set does not make, and the category it feeds would
// then ask for a stage nobody can play or let a run past one it skipped.
//
// Worlds are numbered from zero, the way every event the game reports numbers
// them. A level is its index in the world file; a monster is its index in the
// map's object list, which is what tells two of them in one world apart - the
// arena they load is a level of the common file and is shared between them.

export interface WorldStages {
  readonly world: number;
  /** Every stage of that world, its castle or airship included. */
  readonly levels: readonly number[];
  /** Every Hammer Bros. and variant standing on its map. */
  readonly monsters: readonly number[];
}

export const STAGES: readonly WorldStages[] = [
'''


def map_constants():
    """(levels_low, levels_high, big_castle), read out of src/map.h."""
    src = strip_comments(open(os.path.join(SRC, "map.h")).read())
    ranges = {cls: (low, high) for low, high, cls in tile_ranges(src)}
    if "levels" not in ranges:
        raise ValueError("src/map.h: no levels_low/levels_high")
    names = enum_names(src, "Maptiles")
    castles = [tile for tile, name in names.items() if name == "big_castle"]
    if len(castles) != 1:
        raise ValueError("src/map.h: %d big_castle tiles" % len(castles))
    return ranges["levels"] + (castles[0],)


def stages_of(doc, low, high, big_castle):
    """One world's stage and monster lists, in order."""
    tiles = [int(cell, 16) for row in doc["map"]["tiles"] for cell in row.split()]
    levels = {tile - low for tile in tiles if low <= tile <= high}
    if big_castle in tiles:
        levels.add(CASTLE_LEVEL)

    # A stage on the map that the world file has no level for would crash the
    # game on the way in; here it would be a stage the category waits forever
    # to see completed, which is worth catching where it can still be read.
    missing = sorted(level for level in levels if str(level) not in doc["levels"])
    if missing:
        raise ValueError("%s: map enters levels %s, which the file does not have"
                         % (doc["name"], missing))

    monsters = [i for i, obj in enumerate(doc["map"]["objects"])
                if obj["mode"] in MONSTER_MODES]
    return sorted(levels), monsters


def render(jsondir):
    low, high, big_castle = map_constants()
    out = [HEADER]
    for world, name in enumerate(WORLDS):
        levels, monsters = stages_of(read_json(jsondir, name), low, high,
                                     big_castle)
        out.append("  { world: %d, levels: [%s], monsters: [%s] },\n"
                   % (world, ", ".join(map(str, levels)),
                      ", ".join(map(str, monsters))))
    out.append("];\n")
    return "".join(out)


def main():
    args = sys.argv[1:]
    check = bool(args) and args[0] == "--check"
    if check:
        args = args[1:]
    if len(args) != 2:
        sys.exit(__doc__)

    try:
        want = render(args[0])
    except (OSError, ValueError, KeyError) as e:
        sys.exit("%s: %s" % (type(e).__name__, e))

    if check:
        got = open(args[1]).read() if os.path.exists(args[1]) else ""
        if got != want:
            sys.exit("%s is not what %s says: run "
                     "`python3 tools/mkstages.py %s %s`"
                     % (args[1], args[0], args[0], args[1]))
        print("%s ok" % args[1])
        return

    open(args[1], "w").write(want)
    print("%s written" % args[1])


if __name__ == "__main__":
    main()
