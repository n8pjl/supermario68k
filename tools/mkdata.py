#!/usr/bin/env python3
"""Turn the shipped TI graphics variables into the blobs the wasm build loads.

The level data no longer comes through here: the worlds and the levelset are
built from the JSON under levels/ by tools/mklevels.py, which owns their
MLEV and MLST tags. What is left is the three graphics blobs - ma_tiles,
ma_sprts and ma_bckgr - which are still converted straight from calc-data.

Two jobs:

1. Unwrap the container. A TI-92+/V200 variable file wraps the on-calc variable
   block in an 0x56-byte header plus a trailing 2-byte checksum, so the block is
   file[0x56 : size-2]. The block opens with the big-endian size word of its own
   contents, which is checked here and then dropped: what is written out is the
   contents alone, because the game reads these files as arrays whose size is
   sizeof (see src/compat/assets.cpp).

2. Byte-swap everything that is read as a 16-bit quantity. The data is
   big-endian m68k; wasm is little-endian. What must be left alone is whatever
   the game reads a byte at a time:

     - Inside the GFX blobs, the two byte-addressed regions - Smallsprites
       (8-pixel-wide sprites, one byte per row) and the Games blob - are NOT
       touched. Everything else in them is.

   Struct layouts assume TIGCC's ABI: 16-bit int and 2-byte alignment.

   The GFX region table is derived from the #defines in gfx.h rather than
   hardcoded here, so the two cannot drift apart, and the result is checked
   against the actual blob size (see gfx_regions).

The three calculators do not share data: the backgrounds, the common level and
some of the worlds were retuned for the TI-89's smaller screen, so each model
has its own set under Bin/ and the Makefile passes the directory matching the
build target.

Usage: mkdata.py <src dir> <out dir>
"""
import os
import re
import struct
import sys

# (offset, count) runs of unsigned short within each struct, TIGCC layout.
BGFILEDATA = [(2, 21)]           # char Nr_of_bgs + pad, then Backgrounds[20], Size
SIZEOF_BGFILEDATA = 44
BGDATA = [(0, 2)]                # Height, Width

# The tags this tool owns. Everything else in calc-data/ is level data and
# belongs to mklevels.py.
GRAPHICS_TAGS = frozenset(("GFX", "MBG"))


SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")
GFX_HEADER = os.path.join(SRC, "gfx.h")
ASSETS_SOURCE = os.path.join(SRC, "compat", "assets.cpp")


def declared_assets():
    """{name: tag} from the ASSET_LIST X-macro in src/compat/assets.cpp.

    That list is what the game compiles in, and it names every file by hand;
    this is the only thing that can tell it apart from what actually landed in
    data/. A mismatch either way is a build error - a file nobody embeds is
    dead weight, and an entry with no file would fail to compile anyway, but
    with a #embed error naming a path rather than anything about the levelset.

    Two tools fill data/ now, so each checks its own half: the GFX and MBG
    entries are this one's, the MLEV and MLST entries are mklevels.py's.
    Between them every entry is accounted for.
    """
    src = open(ASSETS_SOURCE).read()
    body = re.search(r"#define ASSET_LIST\(X\)(.*?)\n\n", src, re.S)
    if not body:
        raise ValueError("%s: no ASSET_LIST" % ASSETS_SOURCE)
    return dict(re.findall(r'X\((\w+),\s*"(\w+)"\)', body.group(1)))


def gfx_constants():
    """The Nr_of_* / Size_of_* #defines from gfx.h, as a dict."""
    src = open(GFX_HEADER).read()
    consts = {}
    for m in re.finditer(r"^#define\s+(Nr_of_\w+|Size_of_\w+)\s+(.+?)\s*(?://.*)?$",
                         src, re.M):
        consts[m.group(1)] = eval(m.group(2), {"__builtins__": {}}, dict(consts))
    return consts


def gfx_regions(name, datalen):
    """[(name, length, width)] covering one GFX blob's data area.

    width 2 means the region is read as 16-bit words and must be swapped;
    width 1 means it is byte-addressed and must not be. The offsets mirror the
    pointer arithmetic in Load_gfx_from_file() (gfx.c) exactly - note that the
    Size_of_* constants are counts of shorts despite gfx.h's comment, except
    Size_of_smallsprites, which gfx.c adds to a char* and so is a byte count.
    """
    c = gfx_constants()
    if name == "ma_tiles":
        regions = [
            ("Fg_plane.sprites", 32 * c["Nr_of_fg_tiles"] * 2, 2),
            ("Fg_mask.sprites", 16 * c["Nr_of_tilemasks"] * 2, 2),
            ("Bg_plane.sprites", 32 * c["Nr_of_bg_tiles"] * 2, 2),
            ("Fg_plane.tabanim", c["Nr_of_fg_animations"] * 4 * 2, 2),
            ("Fg_mask.tabanim", c["Nr_of_fg_animations"] * 4 * 2, 2),
            ("Map_plane.sprites", 32 * c["Nr_of_map_tiles"] * 2, 2),
            ("Map_plane.tabanim", c["Nr_of_map_animations"] * 4 * 2, 2),
        ]
    elif name == "ma_sprts":
        regions = [
            ("Mariosprites", c["Size_of_mariosprites"] * 2, 2),
            ("Mariomasks", c["Size_of_mariomasks"] * 2, 2),
            # Animation tables, not pixels, but still 16-bit: these are the
            # indices enemies.c feeds back into Mariosprites.
            ("Marioanimtab", c["Size_of_marioanimtab"] * 2, 2),
            ("Enemysprites", c["Size_of_enemysprites"] * 2, 2),
            ("Smallsprites", c["Size_of_smallsprites"], 1),
            ("Sprites", c["Size_of_sprites"] * 2, 2),
            ("Itemsprites", c["Size_of_itemsprites"] * 2, 2),
            ("Boss_sprites", c["Size_of_boss_sprites"] * 2, 2),
        ]
    else:
        raise ValueError("unknown GFX blob %r" % name)

    total = sum(length for _, length, _ in regions)
    if total > datalen:
        raise ValueError("%s: regions from gfx.h need %d bytes, blob has %d"
                         % (name, total, datalen))
    if name == "ma_tiles":
        # This one is fully described, so an exact match is the check that the
        # gfx.h constants still match the shipped data.
        if total != datalen:
            raise ValueError("%s: regions cover %d bytes, blob has %d"
                             % (name, total, datalen))
    else:
        # ma_sprts ends with the Games blob, whose size is not in gfx.h; it is
        # whatever is left, and it is char data, so it stays unswapped.
        regions.append(("Games", datalen - total, 1))
    return regions


def swap_gfx(b, name, tag):
    """Swap the 16-bit regions of a GFX blob in place."""
    datalen = len(b) - (len(tag) + 3)   # trailing {0, tag, 0, OTH_TAG}
    off = 0
    for rname, length, width in gfx_regions(name, datalen):
        if width == 2:
            if length % 2:
                raise ValueError("%s: %s has odd length %d"
                                 % (name, rname, length))
            swap(b, off, [(0, length // 2)])
        off += length


def swap_backgrounds(b, name, datalen):
    """Swap each background's bg_data header. Returns how many there were.

    The count cannot come from bg_filedata.Nr_of_bgs: it is 0 in every shipped
    file, because nothing writes it and the game never reads it - SetBg()
    indexes Backgrounds[] directly. So it comes from the data instead. A
    background is a Height/Width pair followed by Height*Width matrix bytes, so
    the offsets chain: each one lands exactly where the previous background
    ends. Walking that chain and requiring every step to match the matching
    Backgrounds[] entry both counts the backgrounds and checks the layout, and
    the walk has to consume the data area exactly.

    Getting this wrong is quiet rather than loud - the game reads a byte-swapped
    Width as a plausible-looking number and renders the map from far outside the
    matrix - so the checks here are worth more than usual.
    """
    offs = struct.unpack_from("<20H", b, 2)
    off = SIZEOF_BGFILEDATA
    n = 0
    while n < len(offs) and off + 4 <= datalen and off == offs[n]:
        swap(b, off, BGDATA)
        height, width = struct.unpack_from("<hh", b, off)
        off += 4 + height * width
        n += 1
    if not n:
        raise ValueError("%s: no backgrounds found" % name)
    if off != datalen:
        raise ValueError("%s: %d backgrounds cover %d bytes, blob has %d"
                         % (name, n, off, datalen))
    return n


def swap(buf, base, runs):
    """Byte-swap unsigned short runs in place, relative to base."""
    for off, count in runs:
        for i in range(count):
            p = base + off + i * 2
            if p + 2 > len(buf):
                raise ValueError("swap past end of blob at %d" % p)
            buf[p], buf[p + 1] = buf[p + 1], buf[p]


def u16(buf, p):
    return struct.unpack_from(">H", buf, p)[0]


def tag_of(content):
    """Variables end with {0, tag..., 0, OTH_TAG}; return the tag string."""
    if len(content) < 3 or content[-1] != 0xF8 or content[-2] != 0:
        return None
    i = len(content) - 3
    while i >= 0 and content[i] != 0:
        i -= 1
    return content[i + 1:-2].decode("ascii", "replace")


def convert(content, tag, name):
    b = bytearray(content)
    if tag == "MBG":
        swap(b, 0, BGFILEDATA)
        swap_backgrounds(b, name, len(content) - (len(tag) + 3))
    elif tag == "GFX":
        swap_gfx(b, name, tag)
    return bytes(b)


def unwrap(raw, what="variable file"):
    """Strip a TI variable file's container, returning (size word, content).

    See the module docstring: the on-calc variable block is raw[0x56 : -2], and
    it opens with the big-endian size word of everything after it.
    """
    blob = raw[0x56:len(raw) - 2]
    size, content = blob[:2], blob[2:]
    if struct.unpack(">H", size)[0] != len(content):
        raise ValueError("%s: size word does not match payload" % what)
    return size, content


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "calc-data"
    out = sys.argv[2] if len(sys.argv) > 2 else "data"
    os.makedirs(out, exist_ok=True)
    declared = {name: tag for name, tag in declared_assets().items()
                if tag in GRAPHICS_TAGS}
    converted = {}
    # Data variables: .89y/.89z on the TI-89, .9xy/.9xz on the 92+ and V200.
    # The .89p/.9xp/.v2y/.v2z files are the calculator executable itself.
    for fn in sorted(os.listdir(src)):
        if not fn.lower().endswith((".9xy", ".9xz", ".89y", ".89z")):
            continue
        raw = open(os.path.join(src, fn), "rb").read()
        try:
            _, content = unwrap(raw, fn)
        except ValueError as e:
            sys.exit(str(e))
        name = os.path.splitext(fn)[0]
        tag = tag_of(content)
        # The worlds and the levelset live in the same directory and are built
        # from levels/ instead; skip them rather than converting them twice.
        if tag not in GRAPHICS_TAGS:
            continue
        body = convert(content, tag, name)
        converted[name] = tag
        open(os.path.join(out, name + ".bin"), "wb").write(body)
        print("%-10s %-5s %6d bytes" % (name, tag or "-", len(body)))

    if converted != declared:
        sys.exit("%s: ASSET_LIST graphics entries are %s, data/ has %s"
                 % (ASSETS_SOURCE, sorted(declared.items()),
                    sorted(converted.items())))


if __name__ == "__main__":
    main()
