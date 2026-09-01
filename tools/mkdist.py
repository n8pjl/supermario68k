#!/usr/bin/env python3
"""Assemble dist/ with a content hash in every served filename.

The point is cache headers. Everything here can be served immutable with a
year-long TTL, because a file's name changes whenever its bytes do; only
index.html is fetched every visit, and it is the one file that carries no hash.
That also makes a deploy atomic without any coordination: one revalidated
document names one consistent set of frozen URLs, so a browser can never pair
shell.js from one revision with mario.wasm from another - which it otherwise
can, and which is exactly the failure a plain TTL invites.

Hashing has to run leafwards-first, because rewriting a reference changes the
bytes of the file holding it, and so changes its hash:

    index.html -> shell.js -> mario.js -> mario.wasm
               -> shell.css         \\-> mario.data
                           \\-> ma_texts.json

So the leaves are hashed and renamed, then each referrer has the new names
substituted into it, and only then is the referrer itself hashed. Doing it the
other way round is circular.

Substituting into Emscripten's glue is the one fragile step: the wasm and data
filenames are baked in at link time as plain string literals, and nothing
promises they stay plain across an emcc upgrade. Every substitution here is
therefore checked against the number of occurrences expected, so a toolchain
that starts building those names some other way fails the build loudly instead
of shipping a dist/ full of 404s.

Everything is minified on the way through. dist/ is rebuilt from scratch each
time, so a stale hash cannot linger; deploys should upload without deleting, to
leave the previous revision's files reachable by pages already loaded.
"""

import hashlib
import json
import os
import shutil
import subprocess
import sys

ESBUILD = os.environ.get("ESBUILD", "./node_modules/.bin/esbuild")

# esnext rather than a particular year is load-bearing: at any older target
# esbuild drops the `with { type: "json" }` attribute from shell.js's ma_texts
# import, and a browser then rejects the module over its MIME type. Nothing is
# given up by it - index.html already asks for browsers far newer than any of
# the syntax involved.
ESBUILD_JS = ["--minify", "--format=esm", "--target=esnext"]

# Enough hex to make a collision irrelevant while keeping the names readable.
HASH_LEN = 12


def run(args):
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def minify_js(src, dst):
    run([ESBUILD, src, *ESBUILD_JS, f"--outfile={dst}"])
    return dst


def minify_css(src, dst):
    run([ESBUILD, src, "--minify", f"--outfile={dst}"])
    return dst


def compact_json(src, dst):
    """ma_texts.json is indented for whoever edits it; nothing reading it cares.

    ensure_ascii=False keeps the accented text as UTF-8 rather than doubling its
    size in \\u escapes.
    """
    with open(src, encoding="utf-8") as f:
        data = json.load(f)
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(data, f, separators=(",", ":"), ensure_ascii=False)
    return dst


def substitute(path, replacements):
    """Replace exact strings in a file, insisting on how many of each there are.

    replacements maps an old string to (new string, expected occurrences). A
    count that does not match means the file is not shaped the way this script
    believes, and continuing would ship a reference to a file nobody wrote.
    """
    with open(path, encoding="utf-8") as f:
        text = f.read()
    for old, (new, expected) in replacements.items():
        found = text.count(old)
        if found != expected:
            sys.exit(
                f"{path}: expected {expected} occurrence(s) of {old!r}, found "
                f"{found}. The toolchain changed how it embeds this name; "
                f"update tools/mkdist.py rather than loosening this check."
            )
        text = text.replace(old, new)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def freeze(path):
    """Rename a finished file to carry the hash of its bytes. Returns the name."""
    with open(path, "rb") as f:
        digest = hashlib.sha256(f.read()).hexdigest()[:HASH_LEN]
    stem, ext = os.path.splitext(os.path.basename(path))
    name = f"{stem}-{digest}{ext}"
    os.rename(path, os.path.join(os.path.dirname(path), name))
    return name


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: mkdist.py <builddir> <outdir>")
    build, out = sys.argv[1], sys.argv[2]

    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)

    def dst(name):
        return os.path.join(out, name)

    # Leaves: referenced by others, referencing nothing themselves.
    wasm = freeze(shutil.copy(os.path.join(build, "mario.wasm"), dst("mario.wasm")))
    data = freeze(shutil.copy(os.path.join(build, "mario.data"), dst("mario.data")))
    texts = freeze(compact_json("ma_texts.json", dst("ma_texts.json")))
    css = freeze(minify_css("shell.css", dst("shell.css")))

    # Emscripten's glue. PACKAGE_NAME and the run-dependency key carry the data
    # file's name alongside the one actually fetched; they are only ever used as
    # labels, but they are replaced too so the file stays self-consistent.
    glue = minify_js(os.path.join(build, "mario.js"), dst("mario.js"))
    substitute(glue, {"mario.wasm": (wasm, 2), "mario.data": (data, 4)})
    mario = freeze(glue)

    shell = minify_js("shell.js", dst("shell.js"))
    substitute(shell, {'"./mario.js"': (f'"./{mario}"', 1),
                       '"./ma_texts.json"': (f'"./{texts}"', 1)})
    shell = freeze(shell)

    # The entry point, and the only file without a hash: it is what a browser
    # revalidates in order to discover the current set of hashed names.
    html = shutil.copy("index.html", dst("index.html"))
    substitute(html, {'"shell.js"': (f'"{shell}"', 1),
                      '"shell.css"': (f'"{css}"', 1)})

    for name in sorted(os.listdir(out)):
        print(f"  {os.path.getsize(dst(name)):>7}  {name}")


if __name__ == "__main__":
    main()
