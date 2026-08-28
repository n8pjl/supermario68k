#!/usr/bin/env python3
"""Turn the shipped TI variable files into the blobs the wasm build loads.

Two jobs:

1. Unwrap the container. A TI-92+/V200 variable file wraps the on-calc variable
   block in an 0x56-byte header plus a trailing 2-byte checksum, so the block is
   file[0x56 : size-2]. It already starts with the 2-byte big-endian size word
   the game skips via HeapDeref(h)+2, so nothing downstream needs adjusting.

2. Byte-swap the structs. The data is big-endian m68k; wasm is little-endian.
   Only the structures the game memcpy's or casts a pointer at are swapped -
   everything else is byte-addressed and must be left alone:

     - Level payloads (enemies, flying platforms, triggers) are read one
       unsigned char at a time by level.c, so they are NOT touched.
     - Tile and sprite pixel data is NOT touched; the graphics primitives in
       compat/ read those rows big-endian on purpose.

   Struct layouts assume TIGCC's ABI: 16-bit int and 2-byte alignment.

Usage: mkdata.py <src dir> <out dir>
"""
import os
import struct
import sys

# (offset, count) runs of unsigned short within each struct, TIGCC layout.
LEVELSETDATA = [(4, 8)]          # Compatibility, Savegames[3], Savegame_size[3], Spare
GAMETEXTDATA = [(0, 24)]         # 24 string offsets
LEVELFILEDATA = [(0, 22)]        # Nr_of_levels, Total_size, Levels[20]
SIZEOF_LEVELFILEDATA = 66        # + char Mode + char Name[20], padded to even
MAPDATA = [(0, 15)]              # 15 shorts
SIZEOF_MAPDATA = 30
# leveldata: shorts, then two chars, then two shorts, then four chars, then two
LEVELDATA = [(0, 12), (26, 2), (34, 2)]
SIZEOF_LEVELDATA = 38
BGFILEDATA = [(2, 21)]           # char Nr_of_bgs + pad, then Backgrounds[20], Size
BGDATA = [(0, 2)]                # Height, Width


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
    if tag == "MLST":
        swap(b, 0, LEVELSETDATA)
    elif tag == "MTXT":
        swap(b, 0, GAMETEXTDATA)
    elif tag == "MLEV":
        swap(b, 0, LEVELFILEDATA)          # header first, so the offsets below
        n = struct.unpack_from("<H", b, 0)[0]   # read back native-endian
        swap(b, SIZEOF_LEVELFILEDATA, MAPDATA)
        for i in range(n):
            off = struct.unpack_from("<H", b, 4 + i * 2)[0]
            if off + SIZEOF_LEVELDATA > len(b):
                raise ValueError("%s: level %d offset %d out of range" % (name, i, off))
            swap(b, off, LEVELDATA)
    elif tag == "MBG":
        swap(b, 0, BGFILEDATA)
        n = b[0]
        for i in range(n):
            off = struct.unpack_from("<H", b, 2 + i * 2)[0]
            if off + 4 <= len(b):
                swap(b, off, BGDATA)
    # Everything else - notably GFX, which is ma_tiles/ma_sprts pixel data -
    # is passed through untouched and read big-endian by the graphics code.
    return bytes(b)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "../../Bin/Voyage 200"
    out = sys.argv[2] if len(sys.argv) > 2 else "data"
    os.makedirs(out, exist_ok=True)
    for fn in sorted(os.listdir(src)):
        if not fn.lower().endswith((".9xy", ".9xz")):
            continue
        raw = open(os.path.join(src, fn), "rb").read()
        blob = raw[0x56:len(raw) - 2]
        size, content = blob[:2], blob[2:]
        if struct.unpack(">H", size)[0] != len(content):
            sys.exit("%s: size word does not match payload" % fn)
        name = os.path.splitext(fn)[0]
        tag = tag_of(content)
        body = convert(content, tag, name)
        open(os.path.join(out, name + ".bin"), "wb").write(size + body)
        print("%-10s %-5s %6d bytes" % (name, tag or "-", len(body)))


if __name__ == "__main__":
    main()
