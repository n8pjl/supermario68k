#!/usr/bin/env python3
"""Write the world maps out of levels/ as graphs: what the routing page plans on.

A route is a walk across eight maps, and what a walk can do on one is decided
by src/map.cpp, not by how the map looks. So this reads each map the way that
code moves on it, and writes down what came of it: the squares the player can
stop on, how they join, and what stands in the way. The routing page draws the
maps from it and searches them; neither has to know the tile numbers.

How the map is walked, as Handle_player_map() and the Free_*_map() functions
do it:

  - A press moves from one stopping square to the next in that direction. The
    walk carries on over any tile up to levels_high and ends on the first one
    that is walkable_node_low or more: a node, a house, a pipe, a dock, a
    castle, a level. Anything past levels_high - ground, rock, water, a locked
    door - is a wall.

  - A level not yet beaten can be walked onto but only left the way it was
    entered, so it is a dead end until it is beaten. That is a rule about
    state rather than about the map, so it is the search's to keep; the edges
    here go both ways regardless.

  - Beating a fortress (small_castle, small_castle_2) turns every locked_door
    (locked_door_2) on the map into road, so an edge through one is written
    down with the fortress it needs.

  - The hammer breaks a rock next to the square it is used on, where the
    square beyond the rock is walkable, and lays road in its place. Each rock
    that can be broken so is written down with the edge it opens.

  - A pipe on a world's own map (a trigger whose NewMap is -1) plays a short
    passage and comes out at NewX, NewY. A pipe to another world is a warp,
    and there are none on the world maps: warping is the whistle's.

  - A pipe that plays a whole level rather than a passage - 7-Pipe, picked out
    by the rule in mkstages.py - is a stage entered from the pipe's square.
    Unlike one on a level tile it blocks nothing: the square is walked past
    like any pipe, and it is the way through the pipe that is the level, to
    be played every time it is taken. The square is given the level and the
    square beating it comes out on.

  - A boat (map object mode 30) is stepped onto from beside it and carried
    over water to any dock on that water. The boat goes where the player
    does, so every dock on its water is reachable from every other.

And what the map hands out, which is what an item route is made of:

  - A mushroom house gives the pick of three chests, each of which turns out
    a mushroom, a flower or a leaf by the frame it is opened on (see
    Handle_treasure_all) - one in four, one in four, one in two.

  - An overworld Bros. drops the treasure tile its map object names.

  - A world's castle gives the item its level 7 names in `event`, and flies
    off as an airship when that level's `condition` is set - unless anchored.

  - Some levels add something to the map when beaten (Add_map_event): a card
    game, a money ship, or a hidden mushroom house with a set item in it,
    some of which are entered on the spot. What makes it happen is the
    level's `condition`: the coins collected in it, which run 1 to 100 (see
    the sum in Handle_player_map(), which gives 100 for none), so 0 is a
    number no clear can match; or anything from 240, which is every clear.
    The other way is the white-block secret: a level with the dark square
    Dark_gray_magic_mode_handler() answers to can be ended by walking into it
    from behind the scenery, which sets the condition to 240 on the way out.
    1-3's whistle house is only ever reached that way.

  - Some levels hold treasure chests of their own, in a bonus room or at the
    end of a hidden way: opening one hands its item over and ends the level
    (Handle_treasure_all sets Exit to 2), so a clear gets one of them at
    most. Each stage lists what its chests hold.

  - Game houses are the slot machine, which pays lives and nothing else, so
    they are marked on the map and nothing more.

And where a warp can go. The whistle, used anywhere on a world's map, drops
the player into the warp zone - the common file's map - at the square that
world's map names in warp_x, warp_y; the pipes there with a NewMap load that
world fresh, from its start. The zone's rows are walled off from each other,
so which pipes a world can reach is which row it drops the player on: it is
worked out here by walking the zone from there.

Usage:
  mkmaps.py <json dir> <out file>          write routing/maps.ts
  mkmaps.py --check <json dir> <out file>  is that file still what levels/
                                           says? Run on every build.
"""
import json
import os
import sys

from mklevels import SRC, enum_names, read_json, strip_comments, tile_ranges
from mkstages import CASTLE_LEVEL, MONSTER_MODES, WORLDS, pipe_stages

# The item list's numbering, as the switch in Handle_player_map() spends it,
# named the way src/speedrun.cpp reports it.
ITEMS = {1: "mushroom", 2: "fire-flower", 3: "leaf", 4: "star", 5: "whistle",
         6: "hammer", 7: "p-wing", 8: "cloud", 9: "anchor"}

# The treasure tiles, by what Handle_treasure_all() adds for each. The random
# one is written as "random": which of the three it is is decided on opening.
TREASURES = {"treasure_rand": "random", "treasure_star": "star",
             "treasure_whistle": "whistle", "treasure_hammer": "hammer",
             "treasure_pwing": "p-wing", "treasure_cloud": "cloud",
             "treasure_anchor": "anchor"}

BOAT_MODE = 30

# The non_solid_interactive tile Dark_gray_magic_mode_handler() is for: its
# place in Interactive_non_solid_tile_handlers in src/player.cpp.
DARK_GRAY_MAGIC = 15

# Coins collected in a level, as Handle_player_map() adds them up: 1 to 100.
COINS = range(1, 101)

# Add_map_event()'s cases.
CARD_GAME_EVENT = 1
MONEY_SHIP_EVENT = 5

HEADER = '''\
// The world maps as graphs: where the player can stop, how those places join,
// what stands between them, and what each world hands out.
//
// Generated from levels/ by tools/mkmaps.py, and checked against it on every
// build - see the Makefile. Do not edit; how the maps are read, and which rule
// in src/map.cpp each part of this comes from, is in that script.
//
// Coordinates are map squares, not pixels. Worlds are numbered from zero, as
// every event the game reports numbers them, and a node's `level` is the level
// index those events carry.

/** What a stopping square is. */
export type NodeKind =
  | "junction"
  | "stage"
  | "fortress"
  | "castle"
  | "bowser"
  | "house"
  | "game-house"
  | "pipe"
  | "dock";

export interface MapNode {
  readonly id: number;
  readonly x: number;
  readonly y: number;
  readonly kind: NodeKind;
  /** The level it enters: stages, fortresses, the castle, Bowser's, 7-Pipe. */
  readonly level?: number;
  /**
   * A pipe stage: where beating it comes out. Its square is walked past like
   * any other pipe's - the level is the way through the pipe, not the square.
   */
  readonly exit?: number;
  /** A fortress: which of the two kinds of locked door beating it opens. */
  readonly opens?: 1 | 2;
  /** What the chests inside the level hold, one item a clear at most. */
  readonly chests?: readonly TreasureItem[];
}

export interface MapEdge {
  readonly a: number;
  readonly b: number;
  readonly by: "walk" | "pipe" | "boat";
  /** Squares crossed, which is what a walk or a boat ride costs. */
  readonly tiles: number;
  /** A locked door on the way: the fortress kind that opens it. */
  readonly door?: 1 | 2;
  /** A rock on the way, by its index in `rocks`: the hammer opens it. */
  readonly rock?: number;
  /** The squares from a to b, both ends included, for drawing. */
  readonly path: readonly (readonly [number, number])[];
}

/** A rock the hammer can break, used from the node beside it. */
export interface Rock {
  readonly x: number;
  readonly y: number;
  readonly from: number;
}

/** An overworld Bros., where it starts: they wander between nodes after it. */
export interface MapBros {
  /** The map object index the events report. */
  readonly monster: number;
  readonly node: number;
  readonly treasure: TreasureItem;
}

/** What a level adds to the map when beaten the right way. */
export interface CoinEvent {
  readonly level: number;
  readonly kind: "card-game" | "money-ship" | "house";
  /**
   * What sets it off: beating the level with just so many coins collected,
   * beating it at all, or ending it by the white-block secret.
   */
  readonly by: "coins" | "always" | "white-block";
  /** The coins, where it is by coins. */
  readonly coins?: number;
  /** A hidden house's item. */
  readonly item?: TreasureItem;
  /** Where it appears; null for a house entered on the spot. */
  readonly at: readonly [number, number] | null;
}

export interface Warp {
  /** The world the pipe loads, counted from zero. */
  readonly world: number;
  /** Squares walked in the warp zone, from where the whistle drops the player. */
  readonly tiles: number;
}

export type TreasureItem =
  | "random"
  | "star"
  | "whistle"
  | "hammer"
  | "p-wing"
  | "cloud"
  | "anchor";

export interface WorldMap {
  readonly world: number;
  readonly width: number;
  readonly height: number;
  /**
   * The map as it is drawn, a row a string: " " land, "~" water, "=" road,
   * "o" a stopping square, "1" and "2" the two kinds of locked door, "r" a
   * rock the hammer can break.
   */
  readonly cells: readonly string[];
  readonly start: number;
  readonly nodes: readonly MapNode[];
  readonly edges: readonly MapEdge[];
  readonly rocks: readonly Rock[];
  readonly bros: readonly MapBros[];
  /** The item beating the castle gives; null where there is none, as in 8. */
  readonly reward: string | null;
  /** Whether the castle flies off as an airship on a failed attempt. */
  readonly airship: boolean;
  readonly events: readonly CoinEvent[];
  /** Where the whistle can take the player from here, and the walk to each pipe. */
  readonly warps: readonly Warp[];
}

export const MAPS: readonly WorldMap[] = [
'''


def map_constants():
    src = strip_comments(open(os.path.join(SRC, "map.h")).read())
    ranges = {cls: (low, high) for low, high, cls in tile_ranges(src)}
    tiles = {name: tile for tile, name in enum_names(src, "Maptiles").items()}
    level_h = strip_comments(open(os.path.join(SRC, "level.h")).read())
    treasure = {tile: TREASURES[name]
                for tile, name in enum_names(level_h, "Tiles").items()
                if name in TREASURES}
    ranges["non_solid_interactive"] = next(
        (low, high) for low, high, cls in tile_ranges(level_h) if cls == "non_solid_interactive")
    return ranges, tiles, treasure


DIRECTIONS = [(1, 0), (-1, 0), (0, 1), (0, -1)]


def warps_from(doc, zone, ranges):
    """The warp zone's pipes reachable from where this world's whistle drops
    the player, and how far each is, as [{"world", "tiles"}]."""
    m = zone["map"]
    grid = [[int(c, 16) for c in row.split()] for row in m["tiles"]]
    high = ranges["levels"][1]
    start = (doc["map"]["warp_x"] // 16, doc["map"]["warp_y"] // 16)
    pipes = {(t["x"] // 16, t["y"] // 16): t["new_map"]
             for t in m["triggers"] if t["new_map"] >= 0}

    far = {start: 0}
    queue = [start]
    for x, y in queue:
        for dx, dy in DIRECTIONS:
            nxt = (x + dx, y + dy)
            if nxt in far or not (0 <= nxt[0] < m["width"] and 0 <= nxt[1] < m["height"]):
                continue
            if grid[nxt[1]][nxt[0]] <= high:
                far[nxt] = far[(x, y)] + 1
                queue.append(nxt)

    return sorted(({"world": world, "tiles": far[at]}
                   for at, world in pipes.items() if at in far),
                  key=lambda w: w["world"])


def read_world(doc, world, ranges, tiles, treasure, magic):
    m = doc["map"]
    width, height = m["width"], m["height"]
    grid = [[int(c, 16) for c in row.split()] for row in m["tiles"]]
    node_low = ranges["walkable_node"][0]
    walk_high = ranges["walkable"][1]
    levels_low, levels_high = ranges["levels"]

    def tile(x, y):
        return grid[y][x] if 0 <= x < width and 0 <= y < height else None

    doors = {tiles["locked_door"]: 1, tiles["locked_door_2"]: 2}
    rock, water, dock = tiles["rock"], tiles["water_map"], tiles["dock"]

    def stops(t):
        return t is not None and node_low <= t <= levels_high

    def walk(x, y, dx, dy):
        """One press from (x, y), as Free_*_map() makes it: the squares
        crossed and where it ends, or None where it goes nowhere.

        A door is walked through as the road it becomes, and a rock beside the
        start as the road the hammer leaves. The walk ends on the first
        stopping square, or on the last road square before a wall - the game
        stops the player wherever the road runs out, node or not.
        """
        path = [(x, y)]
        door = None
        broke = None
        while True:
            nx, ny = x + dx, y + dy
            t = tile(nx, ny)
            if t is None:
                break
            if t in doors:
                door = doors[t]
            elif t == rock and len(path) == 1:
                beyond = tile(nx + dx, ny + dy)
                if beyond is None or beyond > walk_high:
                    break
                broke = (nx, ny)
            elif t > levels_high:
                break
            x, y = nx, ny
            path.append((x, y))
            if stops(t):
                break
        if len(path) == 1:
            return None
        return path, door, broke

    # Stopping squares: the node tiles, and the corners where a road runs
    # into a wall - found by walking from what is already known to stop, until
    # nothing new turns up. Numbered in reading order, so the numbering does
    # not depend on the order they were found in.
    stopping = {(x, y) for y in range(height) for x in range(width)
                if stops(grid[y][x])}
    corners = set()
    queue = list(stopping)
    for x, y in queue:
        for dx, dy in DIRECTIONS:
            went = walk(x, y, dx, dy)
            if went is None:
                continue
            end = went[0][-1]
            if end not in stopping and end not in corners:
                corners.add(end)
                queue.append(end)

    stages = pipe_stages(doc)
    ids = {}
    nodes = []
    for y in range(height):
        for x in range(width):
            if (x, y) not in stopping and (x, y) not in corners:
                continue
            t = grid[y][x]
            node = {"id": len(nodes), "x": x, "y": y}
            if (x, y) in corners:
                node["kind"] = "junction"
            elif t == tiles["big_castle"]:
                node["kind"], node["level"] = "castle", CASTLE_LEVEL
            elif t == tiles["bowser_castle"]:
                node["kind"], node["level"] = "bowser", t - levels_low
            elif t in (tiles["small_castle"], tiles["small_castle_2"]):
                node["kind"], node["level"] = "fortress", t - levels_low
                node["opens"] = 1 if t == tiles["small_castle"] else 2
            elif t >= levels_low:
                node["kind"], node["level"] = "stage", t - levels_low
            elif t == tiles["mushrom_house"]:
                node["kind"] = "house"
            elif t == tiles["game_house"]:
                node["kind"] = "game-house"
            elif t == tiles["pipe"]:
                node["kind"] = "pipe"
                for level, trig in stages.items():
                    if (trig["x"] // 16, trig["y"] // 16) == (x, y):
                        node["level"] = level
            elif t == dock:
                node["kind"] = "dock"
            else:
                node["kind"] = "junction"
            ids[(x, y)] = node["id"]
            nodes.append(node)

    for node in nodes:
        if "level" in node and str(node["level"]) not in doc["levels"]:
            raise ValueError("%s: map enters level %d, which the file does "
                             "not have" % (doc["name"], node["level"]))
        if "level" in node:
            inside = doc["levels"][str(node["level"])]["tiles"]
            chests = sorted(treasure[int(c, 16)] for row in inside
                            for c in row.split() if int(c, 16) in treasure)
            if chests:
                node["chests"] = chests

    edges = {}
    rocks = []

    def add(edge):
        key = (edge["a"], edge["b"], edge["by"])
        if key not in edges or edges[key]["tiles"] > edge["tiles"]:
            edges[key] = edge

    for node in nodes:
        for dx, dy in DIRECTIONS:
            went = walk(node["x"], node["y"], dx, dy)
            if went is None:
                continue
            path, door, broke = went
            edge = {"a": node["id"], "b": ids[path[-1]], "by": "walk",
                    "tiles": len(path) - 1, "path": path}
            if door is not None:
                edge["door"] = door
            if broke is not None:
                edge["rock"] = broke
            add(edge)

    # Rocks are numbered once their edges are known. A broken rock is road,
    # so its edge goes back the other way too, although the hammer only
    # reaches it from the one side.
    for edge in sorted(edges.values(), key=lambda e: (e["a"], e["b"])):
        if "rock" not in edge:
            continue
        add({**edge, "a": edge["b"], "b": edge["a"], "path": edge["path"][::-1]})
    for edge in sorted(edges.values(), key=lambda e: (e["a"], e["b"])):
        if "rock" not in edge or not isinstance(edge["rock"], tuple):
            continue
        where = edge["rock"]
        found = next((i for i, r in enumerate(rocks)
                      if (r["x"], r["y"]) == where), None)
        if found is None:
            found = len(rocks)
            rocks.append({"x": where[0], "y": where[1], "from": edge["a"]})
        edge["rock"] = found

    # Pipes within the world.
    for trig in m["triggers"]:
        if trig["new_map"] >= 0:
            raise ValueError("%s: a pipe warps to world %d; the map graph has "
                             "no way to say so" % (doc["name"], trig["new_map"]))
        a = ids.get((trig["x"] // 16, trig["y"] // 16))
        b = ids.get((trig["new_x"] // 16, trig["new_y"] // 16))
        if a is None or b is None:
            raise ValueError("%s: a pipe at %d,%d does not join two stopping "
                             "squares" % (doc["name"], trig["x"], trig["y"]))
        add({"a": a, "b": b, "by": "pipe", "tiles": 0,
             "path": [(nodes[a]["x"], nodes[a]["y"]),
                      (nodes[b]["x"], nodes[b]["y"])]})
        if nodes[a]["kind"] == "pipe" and "level" in nodes[a]:
            nodes[a]["exit"] = b

    # Boats: every dock on the boat's water, to every other, by the shortest
    # way over the water.
    def over_water(a, b):
        """The squares from dock a to dock b by water, or None."""
        came = {a: None}
        queue = [a]
        for cell in queue:
            for dx, dy in DIRECTIONS:
                nxt = (cell[0] + dx, cell[1] + dy)
                if nxt in came:
                    continue
                if nxt == b:
                    came[nxt] = cell
                    path = [nxt]
                    while came[path[-1]] is not None:
                        path.append(came[path[-1]])
                    return path[::-1]
                if tile(*nxt) == water:
                    came[nxt] = cell
                    queue.append(nxt)
        return None

    for obj in m["objects"]:
        if obj["mode"] != BOAT_MODE:
            continue
        boat = (obj["x"] // 16, obj["y"] // 16)
        docks = [(n["x"], n["y"]) for n in nodes if n["kind"] == "dock"]
        # The docks on the boat's water: those it can sail to.
        docks = [d for d in docks if over_water(d, boat) is not None]
        for a in docks:
            for b in docks:
                path = over_water(a, b) if a != b else None
                if path is not None:
                    add({"a": ids[a], "b": ids[b], "by": "boat",
                         "tiles": len(path) - 1, "path": path})

    bros = []
    for i, obj in enumerate(m["objects"]):
        if obj["mode"] not in MONSTER_MODES:
            continue
        at = (obj["x"] // 16, obj["y"] // 16)
        if at not in ids:
            raise ValueError("%s: Bros. %d starts off the paths" % (doc["name"], i))
        bros.append({"monster": i, "node": ids[at],
                     "treasure": treasure[obj["treasure"]]})

    castle = doc["levels"].get(str(CASTLE_LEVEL))
    has_castle = any(n["kind"] == "castle" for n in nodes)
    reward = ITEMS.get(castle["event"]) if castle and has_castle else None

    events = []
    for level, data in sorted(doc["levels"].items(), key=lambda kv: int(kv[0])):
        level = int(level)
        event = data["event"]
        if level == CASTLE_LEVEL or event == 0:
            continue
        condition = data["condition"]
        secret = any(int(c, 16) == magic for row in data["tiles"] for c in row.split())
        if condition >= 240:
            how = {"by": "always"}
        elif condition in COINS:
            how = {"by": "coins", "coins": condition}
        elif secret:
            how = {"by": "white-block"}
        else:
            continue  # nothing can set it off
        at = [data["event_x"], data["event_y"]]
        if event == CARD_GAME_EVENT:
            events.append({"level": level, "kind": "card-game", **how, "at": at})
        elif event == MONEY_SHIP_EVENT:
            events.append({"level": level, "kind": "money-ship", **how, "at": at})
        elif 240 <= event <= 250:
            events.append({"level": level, "kind": "house", **how,
                           "item": treasure[event - 187], "at": None})
        elif event > 50:
            events.append({"level": level, "kind": "house", **how,
                           "item": treasure[event], "at": at})
        else:
            raise ValueError("%s: level %d has event %d, which Add_map_event() "
                             "does nothing with" % (doc["name"], level, event))
        # A level with the secret sets its event off by it, whatever else does.
        if secret and how["by"] != "white-block":
            events.append({**events[-1], "by": "white-block"})
            events[-1].pop("coins", None)

    rock_cells = {(r["x"], r["y"]) for r in rocks}
    cells = []
    for y in range(height):
        row = ""
        for x in range(width):
            t = grid[y][x]
            if (x, y) in ids:
                row += "o"
            elif (x, y) in rock_cells:
                row += "r"
            elif t in doors:
                row += str(doors[t])
            elif t == water:
                row += "~"
            elif t <= walk_high:
                row += "="
            else:
                row += " "
        cells.append(row)

    start = ids.get((m["player_x"] // 16, m["player_y"] // 16))
    if start is None:
        raise ValueError("%s: the player starts off the paths" % doc["name"])

    return {
        "world": world,
        "width": width,
        "height": height,
        "cells": cells,
        "start": start,
        "nodes": nodes,
        "edges": sorted(edges.values(), key=lambda e: (e["a"], e["b"], e["by"])),
        "rocks": rocks,
        "bros": bros,
        "reward": reward,
        "airship": bool(castle and has_castle and castle["condition"]),
        "events": events,
        "warps": [],
    }


def ts(value):
    """One value as TypeScript: JSON, which it already is, on one line."""
    return json.dumps(value, separators=(", ", ": "))


def keyed(obj):
    """An object literal with bare keys, the way the rest of the source reads."""
    return "{ " + ", ".join("%s: %s" % (k, ts(v)) for k, v in obj.items()) + " }"


def render(jsondir):
    ranges, tiles, treasure = map_constants()
    magic = ranges["non_solid_interactive"][0] + DARK_GRAY_MAGIC
    zone = read_json(jsondir, "common")
    out = [HEADER]
    for world, name in enumerate(WORLDS):
        doc = read_json(jsondir, name)
        w = read_world(doc, world, ranges, tiles, treasure, magic)
        w["warps"] = warps_from(doc, zone, ranges)
        out.append("  {\n")
        for key in ("world", "width", "height"):
            out.append("    %s: %s,\n" % (key, ts(w[key])))
        out.append("    cells: [\n")
        out.extend("      %s,\n" % ts(row) for row in w["cells"])
        out.append("    ],\n    start: %d,\n" % w["start"])
        for key in ("nodes", "edges", "rocks", "bros", "events", "warps"):
            if not w[key]:
                out.append("    %s: [],\n" % key)
                continue
            out.append("    %s: [\n" % key)
            out.extend("      %s,\n" % keyed(item) for item in w[key])
            out.append("    ],\n")
        out.append("    reward: %s,\n    airship: %s,\n"
                   % (ts(w["reward"]), ts(w["airship"])))
        out.append("  },\n")
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
                     "`python3 tools/mkmaps.py %s %s`"
                     % (args[1], args[0], args[0], args[1]))
        print("%s ok" % args[1])
        return

    open(args[1], "w").write(want)
    print("%s written" % args[1])


if __name__ == "__main__":
    main()
