#!/usr/bin/env python3
"""Assemble dist/ with a content hash in every served filename.

The point is cache headers. Everything here can be served immutable with a
year-long TTL, because a file's name changes whenever its bytes do; only
the four pages - index.html, data/index.html, routing/index.html and
tas/index.html - are
fetched every visit, and they are the only files that carry no hash.
That also makes a deploy atomic without any coordination: one revalidated
document names one consistent set of frozen URLs, so a browser can never pair
shell.js from one revision with mario.wasm from another - which it otherwise
can, and which is exactly the failure a plain TTL invites.

Hashing has to run leafwards-first, because rewriting a reference changes the
bytes of the file holding it, and so changes its hash:

    index.html -> shell.js -> mario.js -> mario.wasm
               |          \\-> ma_texts.json
               |          \\-> speedrun.js
               |          \\-> practice.js
               \\-> shell.css

    data/index.html -> analysis.js
                    \\-> data.css

    routing/index.html -> routing.js -> routing-worker.js -> routing-search.wasm
                       \\-> data.css
                       \\-> routing.css

    tas/index.html -> tas.js -> mario-tas.js -> mario-tas.wasm
                   |        \\-> mario-tas.wasm
                   |        \\-> tas-worker.js
                   |        \\-> ma_texts.json
                   \\-> data.css
                   \\-> tas.css

So the leaves are hashed and renamed, then each referrer has the new names
substituted into it, and only then is the referrer itself hashed. Doing it the
other way round is circular.

Substituting into Emscripten's glue is the one fragile step: the wasm filename
is baked in at link time as a plain string literal, and nothing promises it
stays plain across an emcc upgrade. Every substitution here is
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
    """Minify one module, leaving its imports as imports.

    esbuild reads TypeScript by extension and only strips the types, which the
    build has already had tsc check, so a .ts source arrives here the same as a
    .js one - see the practice panel, which imports nothing and so needs no
    bundling.
    """
    run([ESBUILD, src, *ESBUILD_JS, f"--outfile={dst}"])
    return dst


def bundle_js(src, dst):
    """Minify a module and everything it imports into one file.

    Only the speedrun timer, which is several TypeScript modules and has to
    arrive as a single hashed leaf - see the dependency order in the docstring
    above. esbuild reads TypeScript by extension and only strips the types,
    which the build has already had tsc check. shell.js is deliberately NOT
    bundled: its imports are the separately hashed mario.js and ma_texts.json.
    """
    run([ESBUILD, src, "--bundle", *ESBUILD_JS, f"--outfile={dst}"])
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
    texts = freeze(compact_json("ma_texts.json", dst("ma_texts.json")))
    css = freeze(minify_css("shell.css", dst("shell.css")))
    speedrun = freeze(bundle_js("speedrun/index.ts", dst("speedrun.js")))
    practice = freeze(minify_js("practice.ts", dst("practice.js")))
    data_css = freeze(minify_css("data.css", dst("data.css")))
    analysis = freeze(bundle_js("analysis/index.ts", dst("analysis.js")))
    routing_css = freeze(minify_css("routing.css", dst("routing.css")))
    # The route search, built by cargo beside the sources (see the Makefile).
    search = freeze(shutil.copy("routing-search.wasm", dst("routing-search.wasm")))

    # The worker names the search, which it fetches from beside itself.
    worker = bundle_js("routing/worker.ts", dst("routing-worker.js"))
    substitute(worker, {'"./routing-search.wasm"': (f'"./{search}"', 1)})
    worker = freeze(worker)

    # The routing page's script names its worker, which it loads by URL from
    # beside itself rather than importing.
    routing = bundle_js("routing/index.ts", dst("routing.js"))
    substitute(routing, {'"./routing-worker.js"': (f'"./{worker}"', 1)})
    routing = freeze(routing)

    # Emscripten's glue.
    glue = minify_js(os.path.join(build, "mario.js"), dst("mario.js"))
    substitute(glue, {"mario.wasm": (wasm, 2)})
    mario = freeze(glue)

    # The TAS build of the game, and the page script that loads it. The script
    # fetches the wasm itself, to name the build a movie was made on by its
    # bytes, and hands it to the glue - which still names it, and is rewritten
    # to match all the same.
    tas_wasm = freeze(shutil.copy(os.path.join(build, "mario-tas.wasm"),
                                  dst("mario-tas.wasm")))
    tas_glue = minify_js(os.path.join(build, "mario-tas.js"), dst("mario-tas.js"))
    substitute(tas_glue, {"mario-tas.wasm": (tas_wasm, 2)})
    tas_glue = freeze(tas_glue)
    tas_css = freeze(minify_css("tas.css", dst("tas.css")))
    # The worker loads the glue from whatever URL the page hands it.
    tas_worker = freeze(bundle_js("tas/worker.ts", dst("tas-worker.js")))
    tas = bundle_js("tas/index.ts", dst("tas.js"))
    substitute(tas, {'"./mario-tas.js"': (f'"./{tas_glue}"', 1),
                     '"./tas-worker.js"': (f'"./{tas_worker}"', 1),
                     '"./mario-tas.wasm"': (f'"./{tas_wasm}"', 1),
                     '"./ma_texts.json"': (f'"./{texts}"', 1)})
    tas = freeze(tas)

    shell = minify_js("shell.js", dst("shell.js"))
    substitute(shell, {'"./mario.js"': (f'"./{mario}"', 1),
                       '"./ma_texts.json"': (f'"./{texts}"', 1),
                       '"./speedrun.js"': (f'"./{speedrun}"', 1),
                       '"./practice.js"': (f'"./{practice}"', 1)})
    shell = freeze(shell)

    # The entry point, and the only file without a hash: it is what a browser
    # revalidates in order to discover the current set of hashed names.
    html = shutil.copy("index.html", dst("index.html"))
    substitute(html, {'"shell.js"': (f'"{shell}"', 1),
                      '"shell.css"': (f'"{css}"', 1),
                      '"data.html"': ('"data/"', 1)})

    # The data page, the other unhashed entry point. It is served as data/ so
    # that it is reached as /data, which puts it a directory down from the
    # files it names; the source tree serves it as data.html beside them,
    # because data/ there is the game's converted blobs.
    os.makedirs(dst("data"))
    page = shutil.copy("data.html", os.path.join(out, "data", "index.html"))
    substitute(page, {'"analysis.js"': (f'"../{analysis}"', 1),
                      '"data.css"': (f'"../{data_css}"', 1),
                      '"index.html"': ('"../"', 1),
                      '"routing.html"': ('"../routing/"', 1)})

    # The routing page, served as routing/ for the same reason.
    os.makedirs(dst("routing"))
    route_page = shutil.copy("routing.html", os.path.join(out, "routing", "index.html"))
    substitute(route_page, {'"routing.js"': (f'"../{routing}"', 1),
                            '"data.css"': (f'"../{data_css}"', 1),
                            '"routing.css"': (f'"../{routing_css}"', 1),
                            '"index.html"': ('"../"', 1),
                            '"data.html"': ('"../data/"', 1)})

    # The TAS page, served as tas/ for the same reason.
    os.makedirs(dst("tas"))
    tas_page = shutil.copy("tas.html", os.path.join(out, "tas", "index.html"))
    substitute(tas_page, {'"tas.js"': (f'"../{tas}"', 1),
                          '"data.css"': (f'"../{data_css}"', 1),
                          '"tas.css"': (f'"../{tas_css}"', 1),
                          '"index.html"': ('"../"', 1)})

    for name in sorted(os.listdir(out)):
        if os.path.isfile(dst(name)):
            print(f"  {os.path.getsize(dst(name)):>7}  {name}")
    print(f"  {os.path.getsize(page):>7}  data/index.html")
    print(f"  {os.path.getsize(route_page):>7}  routing/index.html")
    print(f"  {os.path.getsize(tas_page):>7}  tas/index.html")


if __name__ == "__main__":
    main()
