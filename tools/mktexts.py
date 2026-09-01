#!/usr/bin/env python3
"""Decode the shipped ma_texts variable files into editable JSON.

ma_texts holds every string the game displays. Its layout is the one
reconstructed in FIELDS below: a table of 24 big-endian offsets, followed by a
pool of NUL-terminated strings that the offsets index. Each
offset is the base of one screen's worth of text - doMenu() takes a pointer to
a title and walks forward through the pool for the entries below it - so a slot
owns the title it points at plus every string up to the next slot's offset.
That is how the strings are grouped here: one JSON list per field, in the order
FIELDS lists them.

Two consequences of decoding rather than parsing a real format:

  - Slots may share a base. Statusbar and Statusbar2 point at the same string
    in all five languages, so their lists come out identical; nothing is lost,
    because the game only ever reads forward from an offset.
  - A group can end with empty strings. Those are stray NULs left in the pool
    between one screen's last entry and the next screen's title (Deutsch's
    load_menu has two, Espanol's GfXErr one); they are past the entry count the
    caller passes to doMenu(), so the game never shows them, and they are
    dropped. Never the last string of a group, though: English's ThankU3 really
    is an empty line on the world-completed screen.

The groups then split in two, because they are used two different ways. A menu
is a title plus the entries doMenu() draws under it, so it stays a list; every
other slot is one string the game blits on its own, so it becomes a plain
string. Which is which is fixed by MENU_FIELDS rather than by counting, and
checked against every file: with the padding gone, all five languages agree
exactly - the same slots are menus, with the same number of entries.

Text is decoded as Latin-1: the 68k TIOS font matches ISO-8859-1 over the
printable range, which is what makes every accented byte in the German,
Spanish, French and Norwegian files land on the letter it is meant to be. A
carriage return (\\r, the only control byte used) is the line break.

The field names are FIELDS below, the reconstruction that used to live in a
struct gametextdata in gfx.h; the struct went away with the 68k build, so the
names are kept here, where the only consumer of them is.

Both calculator models ship the same payload - only the variable file's header
differs - so one entry per language covers both. All five go into a single
ma_texts.json keyed by language code, so a consumer loads one file and picks a
language out of it rather than fetching a different file per language.

Usage: mktexts.py <src dir> <out file>
"""
import json
import os
import struct
import sys

from mkdata import tag_of, unwrap

# Endonyms for the language folders the game shipped, keyed by the file stem
# used under the source directory. The originals were spelled in ASCII for
# filesystems of the day; the display names are not.
LANGUAGES = {
    "en": "English",
    "de": "Deutsch",
    "es": "Español",
    "fr": "Français",
    "no": "Norsk",
}

NR_OF_TEXTS = 24        # slots in the offset table
ENCODING = "latin-1"    # see the module docstring

# The slots of the offset table, in file order. Each name was matched to its
# slot by decoding the string it points at; the menu entry counts corroborate
# the mapping independently - Options is called as doMenu(...,3) and its slot is
# followed by exactly 3 entries, main_menu as doMenu(...,4) by 4. Slots with no
# reference in the code keep descriptive names and are listed only to hold the
# layout together.
#
# The two ending screens are told apart by the code that draws them, not by
# their wording, which opens both with "thank you Mario". ThankU* is the
# per-world screen in map.c: three lines blitted at once into a rect 120px wide,
# which is what "PRINCESS IS IN ANOTHER CASTLE!" needs. Ending* is Bowser's, in
# bosses.c: a rect 68px wide - 16 characters of the 4x6 font, the length of
# "THANK YOU MARIO!" and "YOU ARE MY HERO!" - with the third line held back
# until Mario and the princess have run into each other, for "*KISS*".
FIELDS = (
    "main_menu",     # "Main menu:"
    "Save",          # "Save game:"
    "load_menu",     # "Load game:"
    "Options",       # "Options:"
    "Statusbar",     # "Statusbar:"     (unused)
    "Statusbar2",    # "Statusbar:"     (unused, same offset)
    "MidGameMap",    # "Mid-game menu:" (3 entries, unused)
    "MidGameLevel",  # "Mid-game menu:" (2 entries)
    "GameOver",      # "Game over:"     (unused)
    "ThankU1",       # "THANK YOU MARIO, BUT OUR"
    "ThankU2",       # "PRINCESS IS IN ANOTHER CASTLE!"
    "ThankU3",
    "Ending1",       # "THANK YOU MARIO!"
    "Ending2",       # "YOU ARE MY HERO!"
    "Ending3",       # "*KISS*"
    "MemError",      # "Out of memory!"
    "GfXErr",        # "Failed to open GFX file"
    "MapError",      # "Could not load map"
    "LevelError",    # "Could not load level"
    "LevelsetError", # "No levelset found!"
    "GrayError",     # "Grayscale failed"
    "LevelSetIncompatible", # "Levelset incompatible"
    "OverWrite",     # "Overwrite?"
    "KeyConfigTexts", # "Buttons:"
)

if len(FIELDS) != NR_OF_TEXTS:
    raise ValueError("%d field names, format has %d slots"
                     % (len(FIELDS), NR_OF_TEXTS))

# The slots doMenu() draws: a title followed by its entries. Everything else is
# a single string. See the module docstring.
MENU_FIELDS = frozenset((
    "main_menu", "Save", "load_menu", "Options", "Statusbar", "Statusbar2",
    "MidGameMap", "MidGameLevel", "GameOver", "OverWrite", "KeyConfigTexts",
))


def string_pool(content, tag):
    """{offset: string} for the pool after the offset table.

    The data area stops short of the trailing {0, tag, 0, OTH_TAG} that ends
    every TI variable, and the pool's own final NUL is that leading 0's
    neighbour, so the area divides evenly into terminated strings.
    """
    datalen = len(content) - (len(tag) + 3)
    pool = content[NR_OF_TEXTS * 2:datalen]
    if not pool.endswith(b"\0"):
        raise ValueError("string pool is not NUL-terminated")
    strings = {}
    off = 0
    for s in pool[:-1].split(b"\0"):
        strings[off] = s.decode(ENCODING)
        off += len(s) + 1
    return strings, off


def decode(content, tag):
    """{"menus": {field: [strings]}, "strings": {field: string}}.

    Each field owns the pool from its offset up to the next one, minus the
    padding NULs that run can end with.
    """
    offsets = struct.unpack_from(">%dH" % NR_OF_TEXTS, content, 0)
    strings, end = string_pool(content, tag)
    for off in offsets:
        if off not in strings:
            raise ValueError("offset %d is not the start of a string" % off)
    bases = sorted(set(offsets))
    menus, singles = {}, {}
    for field, off in zip(FIELDS, offsets):
        stop = min([b for b in bases if b > off], default=end)
        group = []
        while off < stop:
            group.append(strings[off])
            off += len(strings[off].encode(ENCODING)) + 1
        while len(group) > 1 and not group[-1]:
            group.pop()
        if field in MENU_FIELDS:
            if len(group) < 2:
                raise ValueError("%s is a menu but has no entries" % field)
            menus[field] = group
        else:
            if len(group) != 1:
                raise ValueError("%s is one string but the file has %d"
                                 % (field, len(group)))
            singles[field] = group[0]
    return {"menus": menus, "strings": singles}


def convert(raw, name):
    """One variable file's contents as the JSON document to write."""
    _, content = unwrap(raw, name)
    tag = tag_of(content)
    if tag != "MTXT":
        raise ValueError("%s: tag is %r, not MTXT" % (name, tag))
    return decode(content, tag)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "calc-data/languages"
    out = sys.argv[2] if len(sys.argv) > 2 else "ma_texts.json"
    doc = {}
    for code, language in LANGUAGES.items():
        path = os.path.join(src, code + ".9xy")
        try:
            texts = convert(open(path, "rb").read(), code)
        except (OSError, ValueError) as e:
            sys.exit(str(e))
        doc[code] = {"language": language, "texts": texts}
        print("%-3s %-8s %2d menus, %2d strings"
              % (code, language, len(texts["menus"]), len(texts["strings"])))
    with open(out, "w") as f:
        json.dump(doc, f, ensure_ascii=False, indent=2)
        f.write("\n")


if __name__ == "__main__":
    main()
