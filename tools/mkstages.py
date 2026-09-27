#!/usr/bin/env python3
"""Write the stage manifest out of levels/: what there is, and what it is called.

The 100% category is "every stage and every overworld Hammer Bros.", which is
a claim about the level set rather than about any one run: the timer cannot
tell a complete route from a route that missed world 4's second fortress
unless it is told what there was to miss. That list is here, generated, rather
than written by hand in the timer - a stage added to a map would otherwise
leave a category quietly asking for less than it says.

Three things are counted, and all of them come out of the world maps:

  - Stages. Every map tile in the levels range is one, the level it enters
    being the tile less levels_low; a small_castle and Bowser's castle are
    tiles in that range like any other. The airship or castle that ends a
    world is not - it is the big_castle the map walks onto, entered by
    Enter_enemy_ship(), which loads level 7 - so a world with one of those
    gets level 7 added.

  - Pipe stages. A pipe on a world's map plays whatever level its trigger
    names, from the common file under 20 and from the world's own file from
    20. Nearly all of them are a room of one screen - 15 tiles, the 92+'s 240
    pixels - with nothing in it but the way out, and are passages. One plays a
    level of the world's own file wider than that, and it is a stage: 7-Pipe.
    src/map.cpp decides it by the same rule, which is what makes it report.

  - Overworld monsters. The map objects the game fights rather than walks
    past: Handle_map_objects() sends modes 2 to 8 to Fight_monster(), and
    everything else - the ships, the boats, the houses a map event adds - is
    something else. They are named by their index in the map's object list,
    which is what tells two Bros. in one world apart; the arena they load is a
    level of the common file and is shared.

Each of them is also given the name a player knows it by, which the game
itself never spells out - a level is a numbered tile on the map, and the index
the events report is not that number. So the name is worked out the way the
player works it out, from what the map shows:

  - A level is named by its tile: 45 to 50 are drawn as 1 to 6 and 56 and 57
    as 7 and 8 (so 1-3 is the tile drawn with a 3, whatever its index), and
    the rest by what they are - Fortress, Pyramid, Quicksand, Bonus. A world
    with both fortress tiles has a Fortress 1 and a Fortress 2. Bowser's
    castle is Bowser, and a pipe stage is Pipe.

  - The world's end is the Castle, whether or not an airship flies off from
    it: the map shows the one castle tile either way.

  - A monster is named by the Bros. its arena holds - the common-file level
    Fight_monster() loads, mode less one - and numbered in object order where
    one world has two of a kind. Those numbers are only an order: the Bros.
    walk about, so nothing on the map says which is which.

The name is "<world>-<name>", Bowser's included: 8-Bowser.

The tile numbers and the enum come out of src/map.h, so the game stays the one
place they are written down. What the numbered and plain level tiles are drawn
as is not written anywhere in the source; it is original-docs/
map_tiles_newer.txt, restated in LEVEL_TILES below.

Usage:
  mkstages.py <json dir> <out file>          write speedrun/stages.ts
  mkstages.py --check <json dir> <out file>  is that file still what levels/
                                             says? Run on every build.
"""
import json
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

# A map pipe's LevelNr from which it names a level of the world's own file.
OWN_FILE_LEVELS = 20

# The width of a pipe room, in tiles: one screen. A pipe that plays a level of
# the world's own file wider than this is a stage; see the pipe branch of
# Handle_player_map() in src/map.cpp.
PASSAGE_WIDTH = 15

# The level tiles src/map.h has no name for, by what they are drawn as on the
# map; see original-docs/map_tiles_newer.txt. The fortresses and Bowser's
# castle are named from the enum instead.
LEVEL_TILES = {
    45: "1", 46: "2", 47: "3", 48: "4", 49: "5", 50: "6", 56: "7", 57: "8",
    53: "Pyramid", 54: "Quicksand", 60: "Bonus",
}

# The order a world's stages are listed in: its numbered levels first, as the
# map counts them, then the rest, ending with whatever ends the world.
LEVEL_ORDER = ["1", "2", "3", "4", "5", "6", "7", "8", "Fortress",
               "Fortress 1", "Fortress 2", "Pyramid", "Quicksand", "Bonus",
               "Pipe", "Castle", "Bowser"]

# What an arena's enemies say about which Bros. it is, by the model name's
# prefix. The suffixes are how one behaves, not what it is.
BROS = [("hammerman", "Hammer Bros."), ("boomerang_guy", "Boomerang Bros."),
        ("fireball_guy", "Fire Bros.")]

HEADER = '''\
// The stages and the overworld monsters of every world, and what each is
// called: what a 100% run has to have been through, and the names a player
// knows them by.
//
// Generated from levels/ by tools/mkstages.py, and checked against it on every
// build - see the Makefile. Do not edit: an edit here is a claim about the
// level set that the level set does not make, and the category it feeds would
// then ask for a stage nobody can play or let a run past one it skipped. How
// the names are worked out is in that script.
//
// Worlds are numbered from zero, the way every event the game reports numbers
// them. A level is its index in the world file; a monster is its index in the
// map's object list, which is what tells two of them in one world apart - the
// arena they load is a level of the common file and is shared between them.

export interface Stage {
  /** The level index the events report. */
  readonly level: number;
  /** As the map shows it: "1-3", "2-Pyramid", "7-Pipe", "8-Bowser". */
  readonly name: string;
}

export interface Monster {
  /** The map object index the events report. */
  readonly monster: number;
  /** As a player would say it: "3-Hammer Bros. 2". */
  readonly name: string;
}

export interface WorldStages {
  readonly world: number;
  /**
   * Every stage of that world, its castle or airship included: the numbered
   * levels in order, then the rest, ending with whatever ends the world.
   */
  readonly levels: readonly Stage[];
  /** Every Hammer Bros. and variant standing on its map. */
  readonly monsters: readonly Monster[];
}

export const STAGES: readonly WorldStages[] = [
'''


def map_constants():
    """The levels range and the named tiles, read out of src/map.h."""
    src = strip_comments(open(os.path.join(SRC, "map.h")).read())
    ranges = {cls: (low, high) for low, high, cls in tile_ranges(src)}
    if "levels" not in ranges:
        raise ValueError("src/map.h: no levels_low/levels_high")
    names = enum_names(src, "Maptiles")
    tiles = {}
    for want in ("big_castle", "small_castle", "small_castle_2",
                 "bowser_castle"):
        found = [tile for tile, name in names.items() if name == want]
        if len(found) != 1:
            raise ValueError("src/map.h: %d %s tiles" % (len(found), want))
        tiles[want] = found[0]
    return ranges["levels"], tiles


def pipe_stages(doc):
    """The map pipes that play a stage: their triggers, by the level they
    play."""
    found = {}
    for trig in doc["map"]["triggers"]:
        if trig["new_map"] >= 0 or trig["level_nr"] < OWN_FILE_LEVELS:
            continue
        level = trig["level_nr"] - OWN_FILE_LEVELS
        inside = doc["levels"].get(str(level))
        if inside is None:
            raise ValueError("%s: a pipe enters level %d, which the file does "
                             "not have" % (doc["name"], level))
        if inside["width"] > PASSAGE_WIDTH:
            found[level] = trig
    return found


def bros_of(common, mode):
    """Which Bros. a monster of this mode fights: whatever its arena holds."""
    arena = common["levels"].get(str(mode - 1))
    kinds = set()
    for enemy in arena["enemies"] if arena else []:
        kinds |= {name for prefix, name in BROS
                  if enemy["model"].startswith(prefix)}
    if len(kinds) != 1:
        raise ValueError("common: arena %d for monster mode %d holds %s, "
                         "not one kind of Bros." % (mode - 1, mode,
                                                    sorted(kinds) or "none"))
    return kinds.pop()


def stages_of(doc, common, world, levels_range, tiles):
    """One world's stages and monsters, each as (index, name), in order."""
    low, high = levels_range
    cells = [int(cell, 16) for row in doc["map"]["tiles"] for cell in row.split()]
    special = {tiles["small_castle"]: "Fortress",
               tiles["small_castle_2"]: "Fortress 2",
               tiles["bowser_castle"]: "Bowser"}

    named = {}
    for tile in cells:
        if not low <= tile <= high:
            continue
        name = LEVEL_TILES.get(tile, special.get(tile))
        if name is None:
            raise ValueError("%s: map has level tile %d, which nothing names; "
                             "add it to LEVEL_TILES" % (doc["name"], tile))
        if named.get(tile - low, name) != name:
            raise ValueError("%s: level %d is entered as both %s and %s"
                             % (doc["name"], tile - low, named[tile - low], name))
        named[tile - low] = name

    for level in pipe_stages(doc):
        if named.get(level, "Pipe") != "Pipe":
            raise ValueError("%s: level %d is entered as both %s and Pipe"
                             % (doc["name"], level, named[level]))
        named[level] = "Pipe"

    if tiles["big_castle"] in cells:
        named[CASTLE_LEVEL] = "Castle"

    # A stage on the map that the world file has no level for would crash the
    # game on the way in; here it would be a stage the category waits forever
    # to see completed, which is worth catching where it can still be read.
    missing = sorted(level for level in named if str(level) not in doc["levels"])
    if missing:
        raise ValueError("%s: map enters levels %s, which the file does not have"
                         % (doc["name"], missing))

    if "Fortress 2" in named.values():
        named = {level: "Fortress 1" if name == "Fortress" else name
                 for level, name in named.items()}

    def full(name):
        return "%d-%s" % (world + 1, name)

    levels = [(level, full(named[level]))
              for level in sorted(named,
                                  key=lambda level: LEVEL_ORDER.index(named[level]))]

    fights = [(i, bros_of(common, obj["mode"]))
              for i, obj in enumerate(doc["map"]["objects"])
              if obj["mode"] in MONSTER_MODES]
    kinds = [kind for _, kind in fights]
    monsters, seen = [], {}
    for i, kind in fights:
        seen[kind] = seen.get(kind, 0) + 1
        suffix = " %d" % seen[kind] if kinds.count(kind) > 1 else ""
        monsters.append((i, full(kind + suffix)))

    return levels, monsters


def render(jsondir):
    levels_range, tiles = map_constants()
    common = read_json(jsondir, "common")
    out = [HEADER]
    for world, name in enumerate(WORLDS):
        levels, monsters = stages_of(read_json(jsondir, name), common, world,
                                     levels_range, tiles)
        out.append("  {\n    world: %d,\n    levels: [\n" % world)
        out.extend("      { level: %d, name: %s },\n" % (level, json.dumps(title))
                   for level, title in levels)
        out.append("    ],\n    monsters: [")
        if monsters:
            out.append("\n")
            out.extend("      { monster: %d, name: %s },\n"
                       % (monster, json.dumps(title))
                       for monster, title in monsters)
            out.append("    ")
        out.append("],\n  },\n")
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
