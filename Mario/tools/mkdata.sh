#!/bin/sh
# Extract TI variable payloads from the shipped .9xy/.9xz files into raw blobs.
#
# A TI-92+/V200 variable file wraps the on-calc variable block in an 0x56-byte
# container header plus a trailing 2-byte checksum. The block itself starts with
# the 2-byte big-endian size word that the game skips via HeapDeref(h)+2, so the
# extracted blob is byte-identical to what the calculator would hand back and
# needs no fixups on the C side.
set -e

SRC="${1:-../../Bin/Voyage 200}"
OUT="${2:-data}"

mkdir -p "$OUT"
for f in "$SRC"/*.9xy "$SRC"/*.9xz; do
	[ -e "$f" ] || continue
	base=$(basename "$f")
	name=${base%.*}
	size=$(stat -c%s "$f")
	# payload = file[0x56 : size-2]
	dd if="$f" of="$OUT/$name.bin" bs=1 skip=86 count=$((size - 88)) status=none
	printf '%-10s %6d bytes\n' "$name" "$((size - 88))"
done
