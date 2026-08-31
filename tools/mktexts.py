#!/usr/bin/env python3
"""Decode the shipped ma_texts variable files into editable JSON.

ma_texts holds every string the game displays. Its layout is the one
reconstructed in gfx.h: a struct gametextdata of 24 big-endian offsets,
followed by a pool of NUL-terminated strings that the offsets index. Each
offset is the base of one screen's worth of text - doMenu() takes a pointer to
a title and walks forward through the pool for the entries below it - so a slot
owns the title it points at plus every string up to the next slot's offset.
That is how the strings are grouped here: one JSON list per field, in the order
gfx.h declares them.

Two consequences of decoding rather than parsing a real format:

  - Slots may share a base. Statusbar and Statusbar2 point at the same string
    in all five languages, so their lists come out identical; nothing is lost,
    because the game only ever reads forward from an offset.
  - A group can end with empty strings. Those are stray NULs left in the pool
    between one screen's last entry and the next screen's title (Deutsch's
    load_menu has two, Espanol's GfXErr one); they are past the entry count the
    caller passes to doMenu(), so the game never shows them, and they are
    dropped. Never the last string of a group, though: English's Ending3 really
    is an empty line on the ending screen.

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

The field names come from struct gametextdata in gfx.h rather than a copy kept
here, so the two cannot drift apart, and the struct's field count is checked
against the 24 the format has room for.

Both calculator models ship the same payload - only the variable file's header
differs - so one entry per language covers both. All five go into a single
ma_texts.json keyed by language code, so a consumer loads one file and picks a
language out of it rather than fetching a different file per language.

Usage: mktexts.py <src dir> <out file>
"""
import json
import os
import re
import struct
import sys

from mkdata import GFX_HEADER, tag_of, unwrap

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

NR_OF_TEXTS = 24        # slots in struct gametextdata
ENCODING = "latin-1"    # see the module docstring

# The slots doMenu() draws: a title followed by its entries. Everything else in
# struct gametextdata is a single string. See the module docstring.
MENU_FIELDS = frozenset((
    "main_menu", "Save", "load_menu", "Options", "Statusbar", "Statusbar2",
    "MidGameMap", "MidGameLevel", "GameOver", "OverWrite", "KeyConfigTexts",
))


def text_fields():
    """The struct gametextdata field names from gfx.h, in declaration order."""
    src = open(GFX_HEADER).read()
    m = re.search(r"struct\s+gametextdata\s*\{(.*?)\n\};", src, re.S)
    if not m:
        raise ValueError("%s: no struct gametextdata" % GFX_HEADER)
    fields = re.findall(r"uint16_t\s+(\w+)\s*;", m.group(1))
    if len(fields) != NR_OF_TEXTS:
        raise ValueError("gfx.h declares %d text fields, format has %d"
                         % (len(fields), NR_OF_TEXTS))
    return fields


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


def decode(content, tag, fields):
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
    for field, off in zip(fields, offsets):
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


def convert(raw, name, fields):
    """One variable file's contents as the JSON document to write."""
    _, content = unwrap(raw, name)
    tag = tag_of(content)
    if tag != "MTXT":
        raise ValueError("%s: tag is %r, not MTXT" % (name, tag))
    return decode(content, tag, fields)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "calc-data/languages"
    out = sys.argv[2] if len(sys.argv) > 2 else "ma_texts.json"
    fields = text_fields()
    doc = {}
    for code, language in LANGUAGES.items():
        path = os.path.join(src, code + ".9xy")
        try:
            texts = convert(open(path, "rb").read(), code, fields)
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
