#!/usr/bin/env python3
"""Convert between the game's level blobs and editable JSON source.

The world files are the game's level data: one map and up to twenty levels
each, plus common.bin, which holds the title screen and the small rooms the
map warps into. They shipped as opaque m68k structures with offsets threaded
through them, which is unreadable and unmodifiable by hand. This turns that
into JSON - one document per file, under levels/ - and back.

The JSON is the source. The blobs under data/ are build output: `encode` is
what the Makefile runs, and `--decode` is how levels/ was produced in the
first place and how it can be checked against the shipped originals again.

Bug-for-bug compatible, deliberately
------------------------------------

Encoding levels/ reproduces the shipped data/*.bin byte for byte. That is the
acceptance test for this tool (`--verify`), and it is worth more than a
tidier format would be: the game reads these files with pointer arithmetic
that has no bounds checks in it, so a byte that moves is a bug that does not
announce itself. Three consequences, none of them accidental:

  - leveldata.Size, the malloc size for a loaded level, is stored rather than
    computed. No formula reproduces all 82 shipped values: the original
    compressor omitted the odd-trigger padding that Load_level() applies in 9
    of them, and 3 more in world7 carry slack nobody can account for. The 9
    are a genuine 1-byte heap overflow - the last platform or boss lands one
    byte past the allocation - and are kept anyway, because "compatible with
    the original" is the requirement. Levels written from scratch get the
    computed value (see level_size), which does not have the bug.

  - The last platform of world8's level in slot 5 is a model 12 record stored
    in 7 bytes, one short. Load_level() reads Raw[7] regardless, picking up
    the high byte of the next level's Width - a zero, so the platform's Y
    adjustment is 0 by luck. "truncated": true preserves that, and only that
    record has it.

  - levelfiledata.Nr_of_levels, Total_size and Name are junk in every shipped
    file - Nr_of_levels says 1 for world6, which has eleven levels - and
    nothing reads them; only Levels[] and Mode are read. They are kept
    verbatim under "unused" rather than normalised, so that the byte compare
    stays a byte compare with no allowlist to maintain.

Layout
------

A world file is levelfiledata (a 20-slot offset table), then the map, then
the levels packed end to end. The levels are stored in an order that has
nothing to do with their slot numbers, and the table is sparse: world7 uses
slots 0-7, 11, 15, 18 and 19. Both matter, because map triggers name a level
by its slot number, so "levels" is keyed by slot and its key order is the
physical order in the file. Preserve both and the offsets fall out.

Usage:
  mklevels.py <json dir> <out dir>            JSON -> data/*.bin
  mklevels.py --decode <bin dir> <json dir>   data/*.bin -> JSON
  mklevels.py --verify <json dir> <bin dir>   encode and byte-compare
"""
import json
import os
import re
import struct
import sys

from mkdata import declared_assets

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")

# The files this tool owns. The graphics blobs stay with mkdata.py.
WORLDS = ["common"] + ["world%d" % i for i in range(1, 9)]
LEVELSET = "sm68k"

# The tags those carry, and so the half of ASSET_LIST this tool answers for.
LEVEL_TAGS = frozenset(("MLEV", "MLST"))

SIZEOF_LEVELFILEDATA = 66   # Nr_of_levels, Total_size, Levels[20], Mode, Name[20]
SIZEOF_MAPDATA = 28         # 14 shorts
SIZEOF_LEVELDATA = 38
SIZEOF_MAP_TRIGGER = 28     # 14 shorts
SIZEOF_MAP_OBJECT = 22      # see the layout note in map.h
SIZEOF_TRIGGER = 9          # 9 bytes, no padding
SIZEOF_BOSS_RECORD = 3      # model, x, y

# What Load_level() allocates per entry once the level is in memory. Only used
# to compute Size for a level that does not carry one; see the docstring.
SIZEOF_ENEMY = 32
SIZEOF_FLYING_PLATFORM = 24
SIZEOF_BOSS = 32

NR_OF_SLOTS = 20

# Levelsetdata, the "which files make up this levelset" header. 20 bytes, then
# a 21-byte name, then Nr_of_files+1 nine-byte filenames.
SIZEOF_LEVELSETDATA = 20
LEVELSET_NAME_LEN = 21
LEVELSET_FILENAME_LEN = 9


# ---------------------------------------------------------------------------
# TI variable containers
# ---------------------------------------------------------------------------

def tag_of(content):
    """Variables end with {0, tag..., 0, OTH_TAG}; return the tag string."""
    if len(content) < 3 or content[-1] != 0xF8 or content[-2] != 0:
        return None
    i = len(content) - 3
    while i >= 0 and content[i] != 0:
        i -= 1
    return content[i + 1:-2].decode("ascii", "replace")


def tail_of(tag):
    """The trailing type block a variable of this tag ends with."""
    return b"\0" + tag.encode("ascii") + b"\0\xf8"


# ---------------------------------------------------------------------------
# RLE, ported from src/rle.cpp
# ---------------------------------------------------------------------------

def rle_decompress(data, off, length):
    """Expand `length` bytes starting at `off`; returns (bytes, consumed)."""
    out = bytearray()
    p = off
    while len(out) < length:
        count = data[p]
        p += 1
        count = count - 256 if count > 127 else count
        if count > 0:                       # replicate run
            out += bytes([data[p]]) * count
            p += 1
        elif count < 0:                     # literal run
            out += data[p:p - count]
            p += -count
        else:
            raise ValueError("RLE count of 0 at %d would not terminate" % (p - 1))
    if len(out) != length:
        raise ValueError("RLE produced %d bytes, wanted %d" % (len(out), length))
    return bytes(out), p - off


def rle_compress(data):
    """The encoder from rle.cpp, kept exact so the shipped streams round-trip.

    This reproduces all 82 shipped level matrices byte for byte, including the
    two-token lookback that stops a replicate run of 2 from splitting a
    literal run. Do not "improve" it without re-running --verify.
    """
    length = len(data)
    out = bytearray()
    count = 0
    while count < length:
        index = count
        token = data[index]
        index += 1
        while index < length and index - count < 127 and data[index] == token:
            index += 1
        if index - count == 1:
            while (index < length and index - count < 127
                   and (data[index] != data[index - 1]
                        or (index > 1 and data[index] != data[index - 2]))):
                index += 1
            while index < length and data[index] == data[index - 1]:
                index -= 1
            out.append((count - index) & 0xFF)
            out += data[count:index]
        else:
            out.append(index - count)
            out.append(token)
        count = index
    return bytes(out)


# ---------------------------------------------------------------------------
# Tile legends
# ---------------------------------------------------------------------------

def strip_comments(src):
    """Drop /* */ and // comments.

    level.h and map.h both carry two or three superseded copies of their tile
    constants inside comment blocks, using the same names as the live ones, so
    nothing can be scraped out of them without this first.
    """
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return re.sub(r"//[^\n]*", "", src)


def enum_names(src, name):
    """{value: identifier} for `enum <name>`, which must use explicit values."""
    m = re.search(r"enum\s+%s\s*\{(.*?)\}" % name, src, re.S)
    if not m:
        raise ValueError("no enum %s" % name)
    out = {}
    for ident, value in re.findall(r"(\w+)\s*=\s*(\d+)", m.group(1)):
        out.setdefault(int(value), ident)
    return out


def tile_ranges(src):
    """[(low, high, class)] from the *_low/*_high pairs in level.h."""
    consts = {k: int(v) for k, v in
              re.findall(r"#define\s+(\w+_(?:low|high))\s+(\d+)", src)}
    out = []
    for key in consts:
        if not key.endswith("_low"):
            continue
        base = key[:-4]
        if base + "_high" in consts:
            out.append((consts[key], consts[base + "_high"], base))
    # Most specific first: the narrow interactive ranges sit inside `solid`.
    out.sort(key=lambda r: r[1] - r[0])
    return out


def load_legend_tables():
    level_h = strip_comments(open(os.path.join(SRC, "level.h")).read())
    map_h = strip_comments(open(os.path.join(SRC, "map.h")).read())
    return (enum_names(level_h, "Tiles"), tile_ranges(level_h),
            enum_names(map_h, "Maptiles"), tile_ranges(map_h))


def legend_for(matrix, names, ranges, fallback):
    """{"7e": "brick"} for the ids `matrix` uses.

    Regenerated on every decode and ignored on encode: it is a reading aid for
    whoever opens the file, not data. A tile the enum does not name falls back
    to the behaviour class level.h puts it in, which is what actually decides
    whether the player walks through it. `fallback` covers the ids below the
    lowest range - decoration, in both files.
    """
    legend = {}
    for tile in sorted(set(matrix)):
        if tile in names:
            legend["%02x" % tile] = names[tile]
            continue
        for low, high, cls in ranges:
            if low <= tile <= high:
                legend["%02x" % tile] = cls
                break
        else:
            legend["%02x" % tile] = fallback
    return legend


def rows_to_hex(matrix, width):
    return [" ".join("%02x" % b for b in matrix[y:y + width])
            for y in range(0, len(matrix), width)]


def rows_from_hex(rows, width, height, what):
    matrix = bytearray()
    for y, row in enumerate(rows):
        cells = row.split()
        if len(cells) != width:
            raise ValueError("%s: row %d has %d tiles, width is %d"
                             % (what, y, len(cells), width))
        for cell in cells:
            value = int(cell, 16)
            if not 0 <= value <= 255:
                raise ValueError("%s: tile %r out of range" % (what, cell))
            matrix.append(value)
    if len(rows) != height:
        raise ValueError("%s: %d rows, height is %d" % (what, len(rows), height))
    return bytes(matrix)


# ---------------------------------------------------------------------------
# Enemies
#
# An enemy record is model, x, y and then a model-dependent tail; the model
# also decides the record's length, which is why this table has to agree with
# Load_level() and Generate_handler_1_enemy() exactly. A negative model is the
# same record with respawning turned on.
#
# Shapes below name the tail bytes. Models 13-20, 23, 24, 27, 28, 33, 37, 38,
# 39, 41, 49 and 9 are legal but appear in no shipped level; they are still
# encodable, under their enum name where comp.h has one.
# ---------------------------------------------------------------------------

ENEMY_NAMES = {
    1: "goomba_free", 2: "goomba_limited",
    3: "turtle_free", 4: "turtle_limited",
    5: "hedgehog_free", 6: "hedgehog_limited",
    7: "beetle_free", 8: "beetle_limited",
    9: "fish_free", 10: "fish_limited",
    11: "skel_tur_free", 12: "skel_tur_limited",
    25: "flower1_upside_down", 26: "flower1",
    29: "flower2_upside_down", 30: "flower2",
    31: "boomerang_guy_s", 32: "boomerang_guy_b_t",
    33: "boomerang_guy_b", 34: "boomerang_guy_t",
    35: "fireball_guy_s", 36: "fireball_guy_b_t",
    37: "fireball_guy_b", 38: "fireball_guy_t",
    39: "hammerman_s", 40: "hammerman_b_t",
    41: "hammerman_b", 42: "hammerman_t",
    47: "lavaball", 48: "orb",
    50: "falling_brick", 51: "jumping_brick",
    52: "horizontal_cannon", 53: "diagonal_cannon",
    54: "ghost_free", 55: "ghost_limited",
    56: "mad_flower", 57: "mad_flower_walking",
    58: "flying_turtle", 59: "bottom_flower",
    60: "flying_fish", 61: "flying_goomba",
    62: "jellyfish", 63: "bomb",
}

ENEMY_NUMBERS = {v: k for k, v in ENEMY_NAMES.items()}


def enemy_shape(model):
    """The tail shape for an absolute model number."""
    if 1 <= model <= 20:
        # Generate_handler_1_enemy: the even models are the range-limited ones
        # and carry the extra byte.
        return "h1_limited" if model % 2 == 0 else "h1_free"
    if 31 <= model <= 42:
        # The %4==3 models are neither treasure carriers nor "brothers", so
        # they have no fourth byte at all.
        return "plain" if model % 4 == 3 else "boomerang"
    return {
        47: "lavaball", 52: "horizontal_cannon", 53: "diagonal_cannon",
        55: "ghost_limited", 58: "flying_turtle", 60: "flying_fish",
        63: "bomb",
    }.get(model, "plain")


ENEMY_TAIL_LEN = {
    "plain": 0, "h1_free": 1, "h1_limited": 2, "boomerang": 1,
    "lavaball": 3, "horizontal_cannon": 2, "diagonal_cannon": 2,
    "ghost_limited": 2, "flying_turtle": 2, "flying_fish": 4, "bomb": 3,
}


def s8(v):
    return v - 256 if v > 127 else v


def u8(v, what):
    if not -128 <= v <= 255:
        raise ValueError("%s: %d does not fit in a byte" % (what, v))
    return v & 0xFF


def decode_enemy_tail(shape, tail):
    if shape == "plain":
        return {}
    if shape == "h1_free":
        return {"face": s8(tail[0])}
    if shape == "h1_limited":
        # Data0/Data1, the patrol distance, in tiles; 0 means "walk to the edge
        # and turn" rather than "no range".
        return {"face": s8(tail[0]), "range": tail[1]}
    if shape == "boomerang":
        # Bit 7 makes a "brother" chase the player instead of holding station;
        # the low bits are the treasure count for the treasure-carrying models.
        return {"life": tail[0] & 0x7F, "chases": bool(tail[0] & 0x80)}
    if shape == "lavaball":
        flags = tail[0]
        return {
            "nudge_left": bool(flags & 0x80), "nudge_right": bool(flags & 0x40),
            "nudge_up": bool(flags & 0x20), "nudge_down": bool(flags & 0x10),
            "direction": (flags & 0x0F) - 8,
            "data0": tail[1], "data1": tail[2],
        }
    if shape == "horizontal_cannon":
        return {"face": s8(tail[0]), "interval": tail[1]}
    if shape == "diagonal_cannon":
        return {"interval": tail[0],
                "dx": (tail[1] >> 4) - 8, "dy": (tail[1] & 0x0F) - 8}
    if shape == "ghost_limited":
        return {"life": tail[0], "range": tail[1]}
    if shape == "flying_turtle":
        return {"mode": tail[0], "range": tail[1]}
    if shape == "flying_fish":
        return {"face": s8(tail[0]), "life": tail[1],
                "mode": tail[2], "range": tail[3]}
    if shape == "bomb":
        return {"face": s8(tail[0]), "range": tail[1], "life": tail[2]}
    raise ValueError("unknown enemy shape %r" % shape)


def encode_enemy_tail(shape, e):
    def need(name):
        if name not in e:
            raise ValueError("enemy %r: missing field %r" % (e.get("model"), name))
        return e[name]

    if shape == "plain":
        return b""
    if shape == "h1_free":
        return bytes([u8(need("face"), "face")])
    if shape == "h1_limited":
        return bytes([u8(need("face"), "face"), u8(need("range"), "range")])
    if shape == "boomerang":
        return bytes([(need("life") & 0x7F) | (0x80 if need("chases") else 0)])
    if shape == "lavaball":
        flags = ((0x80 if need("nudge_left") else 0)
                 | (0x40 if need("nudge_right") else 0)
                 | (0x20 if need("nudge_up") else 0)
                 | (0x10 if need("nudge_down") else 0)
                 | ((need("direction") + 8) & 0x0F))
        return bytes([flags, u8(need("data0"), "data0"), u8(need("data1"), "data1")])
    if shape == "horizontal_cannon":
        return bytes([u8(need("face"), "face"), u8(need("interval"), "interval")])
    if shape == "diagonal_cannon":
        return bytes([u8(need("interval"), "interval"),
                      (((need("dx") + 8) & 0x0F) << 4) | ((need("dy") + 8) & 0x0F)])
    if shape == "ghost_limited":
        return bytes([u8(need("life"), "life"), u8(need("range"), "range")])
    if shape == "flying_turtle":
        return bytes([u8(need("mode"), "mode"), u8(need("range"), "range")])
    if shape == "flying_fish":
        return bytes([u8(need("face"), "face"), u8(need("life"), "life"),
                      u8(need("mode"), "mode"), u8(need("range"), "range")])
    if shape == "bomb":
        return bytes([u8(need("face"), "face"), u8(need("range"), "range"),
                      u8(need("life"), "life")])
    raise ValueError("unknown enemy shape %r" % shape)


def enemy_name(model):
    return ENEMY_NAMES.get(model, "model_%d" % model)


def enemy_number(name):
    if name in ENEMY_NUMBERS:
        return ENEMY_NUMBERS[name]
    m = re.fullmatch(r"model_(\d+)", name)
    if not m:
        raise ValueError("unknown enemy model %r" % name)
    return int(m.group(1))


def decode_enemies(data, p, count):
    """Read `count` enemy *slots*, which is not the same as `count` records.

    bottom_flower is one record that Load_level() expands into two enemies -
    the left half and the right half - so it advances the slot counter twice.
    Nr_of_enemies counts slots, so the loop has to as well.
    """
    out = []
    slot = 0
    while slot < count:
        model = s8(data[p])
        absmodel = abs(model)
        shape = enemy_shape(absmodel)
        tail = ENEMY_TAIL_LEN[shape]
        e = {"model": enemy_name(absmodel), "x": data[p + 1], "y": data[p + 2]}
        if model < 0:
            e["respawn"] = True
        e.update(decode_enemy_tail(shape, data[p + 3:p + 3 + tail]))
        out.append(e)
        p += 3 + tail
        slot += 1
        if absmodel == 59:
            slot += 1
    return out, p


def encode_enemies(enemies):
    """Returns (bytes, slot count)."""
    out = bytearray()
    slots = 0
    for e in enemies:
        model = enemy_number(e["model"])
        shape = enemy_shape(model)
        out.append((-model if e.get("respawn") else model) & 0xFF)
        out.append(u8(e["x"], "enemy x"))
        out.append(u8(e["y"], "enemy y"))
        out += encode_enemy_tail(shape, e)
        slots += 2 if model == 59 else 1
    return bytes(out), slots


# ---------------------------------------------------------------------------
# Level triggers, flying platforms, bosses
# ---------------------------------------------------------------------------

TRIGGER_ANIMS = {
    0: "none", 1: "pipe_up", 2: "pipe_down",
    3: "pipe_left", 4: "pipe_right", 5: "fly_up",
}
TRIGGER_ANIM_NUMBERS = {v: k for k, v in TRIGGER_ANIMS.items()}


def decode_trigger(data, p):
    x, y, nx, ny, bl, br, anim, bg, bgoff = data[p:p + SIZEOF_TRIGGER]
    return {
        "x": x, "y": y,
        "new_x": nx, "new_y": ny,
        "border_left": bl, "border_right": br,
        "anim": TRIGGER_ANIMS.get(anim, anim),
        "new_bg": s8(bg), "new_bg_offset": s8(bgoff),
    }


def encode_trigger(t):
    anim = t["anim"]
    if isinstance(anim, str):
        if anim not in TRIGGER_ANIM_NUMBERS:
            raise ValueError("unknown trigger anim %r" % anim)
        anim = TRIGGER_ANIM_NUMBERS[anim]
    return bytes([u8(t["x"], "trigger x"), u8(t["y"], "trigger y"),
                  u8(t["new_x"], "new_x"), u8(t["new_y"], "new_y"),
                  u8(t["border_left"], "border_left"),
                  u8(t["border_right"], "border_right"),
                  u8(anim, "anim"), u8(t["new_bg"], "new_bg"),
                  u8(t["new_bg_offset"], "new_bg_offset")])


# Model 11 sweeps horizontally, 12 vertically with a starting Y adjustment,
# 21 stands still. Anything <= 0 is a placeholder occupying one byte.
PLATFORM_NAMES = {11: "sweep", 12: "sweep_offset", 21: "static"}
PLATFORM_NUMBERS = {v: k for k, v in PLATFORM_NAMES.items()}


def decode_platform(data, p, end):
    model = s8(data[p])
    if model <= 0:
        return {"model": "dummy", "value": model}, p + 1
    if model not in PLATFORM_NAMES:
        raise ValueError("unknown platform model %d at %d" % (model, p))
    pl = {"model": PLATFORM_NAMES[model], "x": data[p + 1], "y": data[p + 2],
          "width": data[p + 3]}
    if model == 21:
        return pl, p + 4
    pl["range"] = data[p + 4]
    pl["data2"] = s8(data[p + 5])
    pl["data3"] = s8(data[p + 6])
    if model == 11:
        return pl, p + 7
    # Model 12 also carries a starting Y adjustment. world8's slot 5 stores
    # only 7 bytes for one of these and Load_level() reads the eighth anyway;
    # see the module docstring.
    if p + 7 >= end:
        pl["truncated"] = True
        return pl, p + 7
    pl["y_adjust"] = s8(data[p + 7])
    return pl, p + 8


def encode_platform(pl):
    name = pl["model"]
    if name == "dummy":
        return bytes([u8(pl.get("value", 0), "platform value")])
    if name not in PLATFORM_NUMBERS:
        raise ValueError("unknown platform model %r" % name)
    model = PLATFORM_NUMBERS[name]
    out = bytearray([model, u8(pl["x"], "platform x"), u8(pl["y"], "platform y"),
                     u8(pl["width"], "platform width")])
    if model == 21:
        return bytes(out)
    out += bytes([u8(pl["range"], "range"), u8(pl["data2"], "data2"),
                  u8(pl["data3"], "data3")])
    if model == 12 and not pl.get("truncated"):
        out.append(u8(pl["y_adjust"], "y_adjust"))
    return bytes(out)


BOSS_NAMES = {10: "boom_boom", 20: "koopa_kid", 30: "bowser"}
BOSS_NUMBERS = {v: k for k, v in BOSS_NAMES.items()}


# ---------------------------------------------------------------------------
# Map
#
# The map is stored once per world, between levelfiledata and the levels: a
# map_data header, the tile matrix, the map triggers, then the map objects.
# Everything from the matrix onwards is big-endian - mkdata.py could not reach
# it, because where it starts depends on fields of the header it was swapping,
# so Swap_map_payload() does it at load time instead. map_data itself is
# swapped at build time and so is little-endian here, as is levelfiledata.
# ---------------------------------------------------------------------------

MAP_FIELDS = ("width", "height", "player_x", "player_y", "warp_x", "warp_y",
              "fg_x", "fg_y", "border_left", "border_right",
              "nr_of_objects", "nr_of_triggers", "color", "spare")

MAP_TRIGGER_FIELDS = ("x", "y", "new_x", "new_y", "level_x", "level_y",
                      "border_left", "border_right", "level_nr", "exit",
                      "new_map", "new_border_left", "new_border_right", "color")

# Load_map() turns these tags into real function pointers once the payload is
# swapped. Only 2 and 3 appear in the shipped maps.
MAP_HANDLERS = {0: "none", 1: "ship", 2: "monster", 3: "boat"}
MAP_HANDLER_NUMBERS = {v: k for k, v in MAP_HANDLERS.items()}


def decode_map_object(data, p):
    x, y = struct.unpack_from(">2h", data, p)
    mode, data0, data1, movex, movey = struct.unpack_from(">5b", data, p + 4)
    treasure = data[p + 9]
    handler, sprite, mask = struct.unpack_from(">3i", data, p + 10)
    return {
        "x": x, "y": y, "mode": mode, "data0": data0, "data1": data1,
        "move_x": movex, "move_y": movey, "treasure": treasure,
        "handler": MAP_HANDLERS.get(handler, handler),
        # Offsets into the Sprites blob in ma_sprts, which this tool does not
        # own; they stay numeric.
        "sprite": sprite, "mask": mask,
    }


def encode_map_object(o):
    handler = o["handler"]
    if isinstance(handler, str):
        if handler not in MAP_HANDLER_NUMBERS:
            raise ValueError("unknown map object handler %r" % handler)
        handler = MAP_HANDLER_NUMBERS[handler]
    return (struct.pack(">2h", o["x"], o["y"])
            + struct.pack(">5b", o["mode"], o["data0"], o["data1"],
                          o["move_x"], o["move_y"])
            + bytes([u8(o["treasure"], "treasure")])
            + struct.pack(">3i", handler, o["sprite"], o["mask"]))


def decode_map(data, map_names, map_ranges):
    fields = dict(zip(MAP_FIELDS, struct.unpack_from("<14h", data,
                                                     SIZEOF_LEVELFILEDATA)))
    width, height = fields["width"], fields["height"]
    p = SIZEOF_LEVELFILEDATA + SIZEOF_MAPDATA
    matrix = data[p:p + width * height]
    p += width * height

    triggers = []
    for _ in range(fields["nr_of_triggers"]):
        values = struct.unpack_from(">14h", data, p)
        triggers.append(dict(zip(MAP_TRIGGER_FIELDS, values)))
        p += SIZEOF_MAP_TRIGGER

    objects = []
    for _ in range(fields["nr_of_objects"]):
        objects.append(decode_map_object(data, p))
        p += SIZEOF_MAP_OBJECT

    out = {k: fields[k] for k in MAP_FIELDS
           if k not in ("nr_of_objects", "nr_of_triggers")}
    out["legend"] = legend_for(matrix, map_names, map_ranges, "walkable")
    out["tiles"] = rows_to_hex(matrix, width)
    out["triggers"] = triggers
    out["objects"] = objects
    return out, p


def encode_map(m):
    width, height = m["width"], m["height"]
    matrix = rows_from_hex(m["tiles"], width, height, "map")
    header = struct.pack(
        "<14h", width, height, m["player_x"], m["player_y"],
        m["warp_x"], m["warp_y"], m["fg_x"], m["fg_y"],
        m["border_left"], m["border_right"],
        len(m["objects"]), len(m["triggers"]), m["color"], m["spare"])
    out = bytearray(header)
    out += matrix
    for t in m["triggers"]:
        out += struct.pack(">14h", *[t[f] for f in MAP_TRIGGER_FIELDS])
    for o in m["objects"]:
        out += encode_map_object(o)
    return bytes(out)


# ---------------------------------------------------------------------------
# Levels
# ---------------------------------------------------------------------------

# The 16-bit fields of struct leveldata, in order. Big-endian: Load_level()
# swaps a level header as it copies it, so mkdata.py never touched these and
# neither does this.
LEVEL_HEAD = ("width", "height", "bg_height", "border_left", "border_right",
              "player_x", "player_y")


def level_size(width, height, slots, triggers, platforms, bosses):
    """What Load_level() needs to allocate, by its own pointer arithmetic.

    Only used for a level whose JSON carries no explicit "size"; the shipped
    levels all do, because no single formula reproduces them. See the module
    docstring.
    """
    area = width * height
    return (area + area % 2
            + SIZEOF_ENEMY * slots
            + SIZEOF_TRIGGER * triggers + triggers % 2
            + SIZEOF_FLYING_PLATFORM * platforms
            + SIZEOF_BOSS * bosses)


def decode_level(data, p, end, tile_names, tile_ranges):
    head = struct.unpack_from(">12h", data, p)
    width, height, bg_height, border_left, border_right = head[:5]
    player_x, player_y, nr_bosses, nr_enemies, nr_triggers = head[5:10]
    nr_platforms, bg_offset = head[10], head[11]
    scrollrate = s8(data[p + 24])
    background = s8(data[p + 25])
    size, tcsize = struct.unpack_from(">2H", data, p + 26)
    event, condition, ex, ey = data[p + 30:p + 34]
    spare1, spare2 = struct.unpack_from(">2h", data, p + 34)

    q = p + SIZEOF_LEVELDATA
    matrix, consumed = rle_decompress(data, q, width * height)
    if consumed != tcsize:
        raise ValueError("RLE stream is %d bytes, TCSize says %d"
                         % (consumed, tcsize))
    q += tcsize

    enemies, q = decode_enemies(data, q, nr_enemies)

    triggers = []
    for _ in range(nr_triggers):
        triggers.append(decode_trigger(data, q))
        q += SIZEOF_TRIGGER

    platforms = []
    for _ in range(nr_platforms):
        pl, q = decode_platform(data, q, end)
        platforms.append(pl)

    bosses = []
    for _ in range(nr_bosses):
        model = data[q]
        if model not in BOSS_NAMES:
            raise ValueError("unknown boss model %d" % model)
        bosses.append({"model": BOSS_NAMES[model],
                       "x": data[q + 1], "y": data[q + 2]})
        q += SIZEOF_BOSS_RECORD

    level = {
        "width": width, "height": height,
        "bg_height": bg_height,
        "border_left": border_left, "border_right": border_right,
        "player_x": player_x, "player_y": player_y,
        "background": background, "bg_scrollrate": scrollrate,
        "bg_offset": bg_offset,
        "event": event, "condition": condition,
        "event_x": ex, "event_y": ey,
        "spare1": spare1, "spare2": spare2,
        # Stored, not computed: the shipped values disagree with every
        # formula, and nine of them are one byte short of what the loader
        # needs. See the module docstring.
        "size": size,
        "legend": legend_for(matrix, tile_names, tile_ranges, "non_solid"),
        "tiles": rows_to_hex(matrix, width),
        "enemies": enemies,
        "triggers": triggers,
        "platforms": platforms,
        "bosses": bosses,
    }
    return level, q


def encode_level(level, what):
    width, height = level["width"], level["height"]
    matrix = rows_from_hex(level["tiles"], width, height, what)
    stream = rle_compress(matrix)

    enemy_bytes, slots = encode_enemies(level["enemies"])
    trigger_bytes = b"".join(encode_trigger(t) for t in level["triggers"])
    platform_bytes = b"".join(encode_platform(p) for p in level["platforms"])

    boss_bytes = bytearray()
    for b in level["bosses"]:
        if b["model"] not in BOSS_NUMBERS:
            raise ValueError("%s: unknown boss model %r" % (what, b["model"]))
        boss_bytes += bytes([BOSS_NUMBERS[b["model"]],
                             u8(b["x"], "boss x"), u8(b["y"], "boss y")])

    size = level.get("size")
    if size is None:
        size = level_size(width, height, slots, len(level["triggers"]),
                          len(level["platforms"]), len(level["bosses"]))

    # Size and TCSize are 16-bit fields, and Load_level() mallocs Size and
    # RLE-expands Width*Height into it. A level that outgrows either of those
    # would be silently truncated on the calculator's own terms, so it is
    # rejected here rather than written out.
    if not 0 <= size <= 0xFFFF:
        raise ValueError("%s: needs %d bytes, more than the 16-bit Size field "
                         "holds" % (what, size))
    if len(stream) > 0xFFFF:
        raise ValueError("%s: compressed to %d bytes, more than the 16-bit "
                         "TCSize field holds" % (what, len(stream)))
    if width <= 0 or height <= 0:
        raise ValueError("%s: %dx%d is not a level" % (what, width, height))

    header = struct.pack(
        ">12h", width, height, level["bg_height"],
        level["border_left"], level["border_right"],
        level["player_x"], level["player_y"],
        len(level["bosses"]), slots, len(level["triggers"]),
        len(level["platforms"]), level["bg_offset"])
    header += bytes([u8(level["bg_scrollrate"], "bg_scrollrate"),
                     u8(level["background"], "background")])
    header += struct.pack(">2H", size, len(stream))
    header += bytes([u8(level["event"], "event"),
                     u8(level["condition"], "condition"),
                     u8(level["event_x"], "event_x"),
                     u8(level["event_y"], "event_y")])
    header += struct.pack(">2h", level["spare1"], level["spare2"])
    if len(header) != SIZEOF_LEVELDATA:
        raise AssertionError("level header is %d bytes" % len(header))

    return (header + stream + enemy_bytes + trigger_bytes
            + platform_bytes + bytes(boss_bytes))


# ---------------------------------------------------------------------------
# Whole world files
# ---------------------------------------------------------------------------

def decode_world(name, content, tag):
    tile_names, ranges, map_names, map_ranges = load_legend_tables()
    data = content[:len(content) - len(tail_of(tag))]

    nr_of_levels, total_size = struct.unpack_from("<2h", data, 0)
    offsets = struct.unpack_from("<20H", data, 4)
    mode = data[44]
    if mode != 1:
        raise ValueError("%s: Mode is %d, only map mode (1) is understood"
                         % (name, mode))

    doc = {
        "format": 1,
        "name": name,
        "tag": tag,
        # Junk in every shipped file and read by nothing; kept so that the
        # encoder reproduces the original bytes exactly. See the docstring.
        "unused": {
            "nr_of_levels": nr_of_levels,
            "total_size": total_size,
            "name": data[45:65].rstrip(b"\0").decode("latin-1"),
        },
    }

    doc["map"], map_end = decode_map(data, map_names, map_ranges)

    # Slots holding something that is not a level offset. common has two, left
    # over from whatever last wrote the file; Levels[] is read by index, so
    # they are carried rather than dropped.
    real = [(slot, off) for slot, off in enumerate(offsets)
            if off and map_end <= off < len(data)]
    dead = {str(slot): off for slot, off in enumerate(offsets)
            if off and not (map_end <= off < len(data))}
    if dead:
        doc["dead_slots"] = dead

    # Key order is the order the levels sit in the file, which is not slot
    # order and has to be preserved to reproduce the offsets.
    by_offset = sorted(real, key=lambda so: so[1])
    ends = {off: nxt for (_, off), (_, nxt)
            in zip(by_offset, by_offset[1:])}
    levels = {}
    for slot, off in by_offset:
        end = ends.get(off, len(data))
        level, stop = decode_level(data, off, end, tile_names, ranges)
        if stop != end:
            raise ValueError("%s: level in slot %d ends at %d, next starts at %d"
                             % (name, slot, stop, end))
        levels[str(slot)] = level
    doc["levels"] = levels
    return doc


def encode_world(doc):
    name = doc["name"]
    map_bytes = encode_map(doc["map"])

    offsets = [0] * NR_OF_SLOTS
    for slot, off in doc.get("dead_slots", {}).items():
        offsets[int(slot)] = off

    body = bytearray()
    base = SIZEOF_LEVELFILEDATA + len(map_bytes)
    for slot, level in doc["levels"].items():
        slot = int(slot)
        if not 0 <= slot < NR_OF_SLOTS:
            raise ValueError("%s: slot %d is outside the %d-slot table"
                             % (name, slot, NR_OF_SLOTS))
        if offsets[slot]:
            raise ValueError("%s: slot %d used twice" % (name, slot))
        offset = base + len(body)
        if offset > 0xFFFF:
            raise ValueError("%s: level in slot %d starts at %d, past the "
                             "16-bit offset table" % (name, slot, offset))
        offsets[slot] = offset
        body += encode_level(level, "%s slot %d" % (name, slot))

    unused = doc.get("unused", {})
    header = struct.pack("<2h", unused.get("nr_of_levels", len(doc["levels"])),
                         unused.get("total_size", 0))
    header += struct.pack("<20H", *offsets)
    header += bytes([1])                            # Mode: map mode
    header += unused.get("name", "").encode("latin-1").ljust(20, b"\0")
    if len(header) != SIZEOF_LEVELFILEDATA - 1:
        raise AssertionError("levelfiledata is %d bytes" % len(header))
    header += b"\0"                                 # pad to an even size

    return bytes(header) + map_bytes + bytes(body) + tail_of(doc["tag"])


# ---------------------------------------------------------------------------
# The levelset file
# ---------------------------------------------------------------------------

def decode_levelset(name, content, tag):
    data = content[:len(content) - len(tail_of(tag))]
    nr_of_files, commonfile, mode, current_world = data[:4]
    compatibility, = struct.unpack_from("<H", data, 4)
    savegames = struct.unpack_from("<3H", data, 6)
    sizes = struct.unpack_from("<3H", data, 12)
    spare, = struct.unpack_from("<H", data, 18)
    title = data[SIZEOF_LEVELSETDATA:SIZEOF_LEVELSETDATA + LEVELSET_NAME_LEN]
    p = SIZEOF_LEVELSETDATA + LEVELSET_NAME_LEN
    files = []
    for i in range(nr_of_files + 1):
        entry = data[p:p + LEVELSET_FILENAME_LEN]
        files.append(entry.split(b"\0")[0].decode("latin-1"))
        p += LEVELSET_FILENAME_LEN
    if p != len(data):
        raise ValueError("%s: %d bytes left over" % (name, len(data) - p))
    return {
        "format": 1, "name": name, "tag": tag,
        "mode": mode, "current_world": current_world,
        "compatibility": compatibility,
        "title": title.split(b"\0")[0].decode("latin-1"),
        # A save slot is a byte offset into this file, 0 when empty; the game
        # rewrites the whole file through Asset_save when it saves.
        "savegames": list(savegames), "savegame_sizes": list(sizes),
        "spare": spare,
        # The last entry is the common file; Commonfile indexes it.
        "files": files,
    }


def encode_levelset(doc):
    files = doc["files"]
    nr_of_files = len(files) - 1
    out = bytearray([nr_of_files, nr_of_files, doc["mode"], doc["current_world"]])
    out += struct.pack("<H", doc["compatibility"])
    out += struct.pack("<3H", *doc["savegames"])
    out += struct.pack("<3H", *doc["savegame_sizes"])
    out += struct.pack("<H", doc["spare"])
    out += doc["title"].encode("latin-1").ljust(LEVELSET_NAME_LEN, b"\0")
    for entry in files:
        out += entry.encode("latin-1").ljust(LEVELSET_FILENAME_LEN, b"\0")
    return bytes(out) + tail_of(doc["tag"])


# ---------------------------------------------------------------------------
# Driver
# ---------------------------------------------------------------------------

def decode_file(name, content):
    tag = tag_of(content)
    if name == LEVELSET:
        if tag != "MLST":
            raise ValueError("%s: tag is %r, not MLST" % (name, tag))
        return decode_levelset(name, content, tag)
    if tag != "MLEV":
        raise ValueError("%s: tag is %r, not MLEV" % (name, tag))
    return decode_world(name, content, tag)


def encode_file(doc):
    if doc["name"] == LEVELSET:
        return encode_levelset(doc)
    return encode_world(doc)


def read_json(jsondir, name):
    with open(os.path.join(jsondir, name + ".json")) as f:
        return json.load(f)


def write_json(jsondir, name, doc):
    with open(os.path.join(jsondir, name + ".json"), "w") as f:
        json.dump(doc, f, ensure_ascii=False, indent=1)
        f.write("\n")


def do_decode(bindir, jsondir):
    os.makedirs(jsondir, exist_ok=True)
    for name in WORLDS + [LEVELSET]:
        content = open(os.path.join(bindir, name + ".bin"), "rb").read()
        doc = decode_file(name, content)
        write_json(jsondir, name, doc)
        levels = len(doc.get("levels", {}))
        print("%-8s %6d bytes  %2d levels" % (name, len(content), levels))


def do_encode(jsondir, outdir):
    os.makedirs(outdir, exist_ok=True)
    written = {}
    for name in WORLDS + [LEVELSET]:
        doc = read_json(jsondir, name)
        blob = encode_file(doc)
        open(os.path.join(outdir, name + ".bin"), "wb").write(blob)
        written[name] = doc["tag"]
        print("%-8s %6d bytes" % (name, len(blob)))

    # The other half of the check mkdata.py makes: between the two tools every
    # entry of ASSET_LIST has to be backed by a file in data/, and every file
    # by an entry. See declared_assets() there.
    declared = {name: tag for name, tag in declared_assets().items()
                if tag in LEVEL_TAGS}
    if written != declared:
        sys.exit("src/compat/assets.cpp: ASSET_LIST level entries are %s, "
                 "levels/ has %s"
                 % (sorted(declared.items()), sorted(written.items())))


def do_verify(jsondir, bindir):
    bad = 0
    for name in WORLDS + [LEVELSET]:
        want = open(os.path.join(bindir, name + ".bin"), "rb").read()
        got = encode_file(read_json(jsondir, name))
        if got == want:
            print("%-8s ok   %6d bytes" % (name, len(got)))
            continue
        bad += 1
        if len(got) != len(want):
            print("%-8s SIZE %d, want %d" % (name, len(got), len(want)))
            continue
        first = next(i for i, (a, b) in enumerate(zip(got, want)) if a != b)
        differ = sum(1 for a, b in zip(got, want) if a != b)
        print("%-8s DIFF %d bytes, first at %d" % (name, differ, first))
    return bad


def main():
    args = sys.argv[1:]
    # A hand edit that does not fit the format is the expected failure here, so
    # report it the way the other tools do rather than as a traceback.
    try:
        if args and args[0] == "--decode":
            do_decode(args[1], args[2])
        elif args and args[0] == "--verify":
            if do_verify(args[1], args[2]):
                sys.exit("encoded output does not match the shipped blobs")
        else:
            do_encode(args[0] if args else "levels",
                      args[1] if len(args) > 1 else "data")
    except (OSError, ValueError, KeyError, struct.error) as e:
        sys.exit("%s: %s" % (type(e).__name__, e))


if __name__ == "__main__":
    main()
