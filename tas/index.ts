// The TAS page: the game's TAS build on the left, and on the right the panel a
// movie is played, stepped, rewound and edited in. See session.ts for how a
// movie and the game are kept in step, and src/tas.h for what the build does
// differently from the one the game ships as.
//
// The movie being worked on is saved in this browser as it changes, and comes
// back on the next visit. Exporting is the only way it leaves.

import { BOOKMARKS, BUTTONS, buildId, CALCS, emptyMovie, parse, serialize, type Calc, type Input, type Movie } from "./movie.ts";
import { Machine } from "./machine.ts";
import { FULL_SPEED, Roll } from "./roll.ts";
import { changed, length, type Segment } from "./segments.ts";
import { Session } from "./session.ts";
import { duration, formatDuration } from "../speedrun/times.ts";

// Beside this script, here and in dist/ - where tools/mkdist.py rewrites each
// to its hashed name. The glue is loaded by the game's worker, not here.
const GLUE_URL = new URL("./mario-tas.js", import.meta.url).href;
const WASM_URL = new URL("./mario-tas.wasm", import.meta.url).href;
const TEXTS_URL = new URL("./ma_texts.json", import.meta.url).href;

const MOVIE_KEY = "sm68k.tas.movie";
// Read, never written: the bindings are the game page's to edit.
const BINDINGS_KEY = "sm68k.bindings";
const CALC_KEY = "sm68k.calc";
const LANG_KEY = "sm68k.lang";

const SAVE_DELAY_MS = 500;

/** The largest scale the screen is drawn at. */
const MAX_SCALE = 4;

function element<T extends HTMLElement>(id: string): T {
  return document.getElementById(id) as T;
}

let canvas = element<HTMLCanvasElement>("canvas");
const status = element("status");
const form = element<HTMLFormElement>("movie");
const calcSelect = form.elements.namedItem("calc") as HTMLSelectElement;
const langSelect = form.elements.namedItem("lang") as HTMLSelectElement;
const fileInput = element<HTMLInputElement>("file");
const playButton = element<HTMLButtonElement>("play");
const recordButton = element<HTMLButtonElement>("record");
const speedSelect = element<HTMLSelectElement>("speed");
const bookmarkBar = element("bookmarks");
const levelSelect = element<HTMLSelectElement>("level");
const check = element("check");
const strip = element<HTMLCanvasElement>("strip");
const segmentBar = element("segments");

// ---------------------------------------------------------------------------
// The game's keys
//
// Bound as the game page binds them, keyboard only: a movie is made a frame
// at a time, and a key is what steps it.
// ---------------------------------------------------------------------------

const DEFAULT_KEYS: Record<string, string[]> = {
  left: ["ArrowLeft"],
  right: ["ArrowRight"],
  up: ["ArrowUp"],
  down: ["ArrowDown"],
  jump: ["Space", "KeyZ"],
  run: ["ShiftLeft", "ShiftRight", "KeyX"],
  enter: ["Enter"],
  esc: ["Escape"],
};

function loadKeys(): Map<string, number> {
  let saved: Record<string, unknown> = {};
  try {
    const parsed: unknown = JSON.parse(localStorage.getItem(BINDINGS_KEY) ?? "{}");
    if (parsed && typeof parsed === "object") saved = parsed as Record<string, unknown>;
  } catch {
    /* Unreadable: the defaults stand. */
  }

  const keys = new Map<string, number>();
  BUTTONS.forEach((button, i) => {
    const bound = saved[button];
    const codes = Array.isArray(bound)
      ? bound.filter((b): b is string => typeof b === "string" && !b.startsWith("gp:"))
      : DEFAULT_KEYS[button]!;
    for (const code of codes) keys.set(code, (keys.get(code) ?? 0) | (1 << i));
  });
  return keys;
}

const gameKeys = loadKeys();
const held = new Set<string>();

function live(): Input {
  let input = 0;
  for (const code of held) input |= gameKeys.get(code) ?? 0;
  return input;
}

// ---------------------------------------------------------------------------
// The session
// ---------------------------------------------------------------------------

const [wasm, texts] = await Promise.all([
  fetch(WASM_URL).then((r) => {
    if (!r.ok) throw new Error(`${WASM_URL}: ${r.status}`);
    return r.arrayBuffer();
  }),
  fetch(TEXTS_URL).then((r) => r.json() as Promise<Record<string, { language: string; texts: unknown }>>),
]);
const build = await buildId(wasm);

let session: Session | null = null;
let selected = 0;
let lastFrame = -1;
let notice = "";

const roll = new Roll(element("roll"), {
  setInput: (frame, input) => session?.setInput(frame, input),
  seek: (frame) => {
    if (!session) return;
    session.pause();
    session.seek(frame);
  },
  select: (frame) => {
    selected = frame;
    render();
  },
});

// The size of the game's screen. The canvas is drawn on from the game's
// worker, so the element's own width and height no longer say.
let screenSize = { width: 0, height: 0 };

function fitCanvas(): void {
  const room = (canvas.parentElement!.clientWidth - 24) / screenSize.width;
  const scale = Math.max(1, Math.min(MAX_SCALE, room));
  canvas.style.width = `${Math.floor(screenSize.width * scale)}px`;
  canvas.style.height = `${Math.floor(screenSize.height * scale)}px`;
}

addEventListener("resize", () => {
  fitCanvas();
  render();
});

/**
 * A new canvas in place of the old, handed over to be drawn on from a worker.
 * A canvas can be handed over only once, and each game the page starts -
 * including one started again after the last hung - takes one for good.
 */
function freshCanvas(): OffscreenCanvas {
  const fresh = canvas.cloneNode(false) as HTMLCanvasElement;
  fresh.width = screenSize.width;
  fresh.height = screenSize.height;
  canvas.replaceWith(fresh);
  canvas = fresh;
  fitCanvas();
  return canvas.transferControlToOffscreen();
}

async function open(movie: Movie, why: string): Promise<void> {
  session?.pause();
  session?.machine.close();
  session = null;

  const lang = Object.hasOwn(texts, movie.lang) ? movie.lang : "en";
  notice = why;
  if (lang !== movie.lang) {
    notice += ` The game has no texts in "${movie.lang}"; English stands in, and the movie may not play out the same.`;
  }
  if (movie.build !== build) {
    notice += ` This movie was made on game build ${movie.build}, and this is ${build}: it may play out differently here.`;
  }

  calcSelect.value = movie.calc;
  langSelect.value = lang;
  screenSize = { width: CALCS[movie.calc].width, height: CALCS[movie.calc].height };

  const machine = await Machine.boot({
    glue: GLUE_URL,
    wasm,
    calc: movie.calc,
    texts: texts[lang]!.texts,
    canvas: freshCanvas,
    onCanvasResize: (width, height) => {
      screenSize = { width, height };
      fitCanvas();
    },
  });

  const opened = new Session(machine, movie, live);
  opened.addEventListener("change", render);
  opened.addEventListener("movie", save);
  session = opened;
  selected = 0;
  lastFrame = -1;
  save();
  render();
}

// ---------------------------------------------------------------------------
// Saving
// ---------------------------------------------------------------------------

let saveTimer: ReturnType<typeof setTimeout> | undefined;
let saveFailed = false;

function save(): void {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => {
    if (!session) return;
    try {
      localStorage.setItem(MOVIE_KEY, serialize(session.movie));
      saveFailed = false;
    } catch {
      saveFailed = true;
    }
    render();
  }, SAVE_DELAY_MS);
}

function savedMovie(): Movie | null {
  try {
    const text = localStorage.getItem(MOVIE_KEY);
    return text === null ? null : parse(text);
  } catch {
    return null;
  }
}

function defaultMovie(): Movie {
  let calc: Calc = "ti92";
  let lang = "en";
  try {
    const savedCalc = localStorage.getItem(CALC_KEY);
    if (savedCalc === "ti89" || savedCalc === "ti92") calc = savedCalc;
    const savedLang = localStorage.getItem(LANG_KEY);
    if (savedLang && Object.hasOwn(texts, savedLang)) lang = savedLang;
  } catch {
    /* Storage unavailable: the defaults stand. */
  }
  return emptyMovie(calc, lang, build);
}

// ---------------------------------------------------------------------------
// Drawing the panel
// ---------------------------------------------------------------------------

let renderQueued = false;

function render(): void {
  if (renderQueued) return;
  renderQueued = true;
  requestAnimationFrame(() => {
    renderQueued = false;
    draw();
  });
}

/** The segment a frame is in: the last, for one past them all. */
function segmentAt(segments: readonly Segment[], frame: number): number {
  let lo = 0;
  let hi = segments.length - 1;
  while (lo < hi) {
    const mid = (lo + hi) >> 1;
    if (segments[mid]!.end <= frame) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

function signed(n: number): string {
  return n > 0 ? `+${n}` : n < 0 ? `−${-n}` : "";
}

/** How many frames a level takes now against when the movie was opened. */
function delta(segment: Segment): number {
  return segment.base === null || segment.name === "" ? 0 : length(segment) - segment.base;
}

function levelName(segment: Segment): string {
  return segment.name || "Unfinished";
}

/** What the current level's edits did to the levels after it. */
function checkLevel(s: Session, segments: readonly Segment[], i: number): { text: string; state: string } {
  const segment = segments[i]!;
  if (!segment.checked) {
    return { text: "Not played again since the last edit yet.", state: "waiting" };
  }
  if (segment.name === "") {
    return { text: "The movie ends before this level is beaten.", state: "" };
  }

  const later = segments.slice(i + 1).filter((t) => t.name !== "");
  const played = later.filter((t) => t.checked);
  const off = played.find(changed);
  if (off) {
    const prior = off.prior!;
    const text =
      prior.name !== off.name
        ? `✗ ${levelName(off)} is now beaten where ${prior.name} was: the levels after this one have changed.`
        : `✗ ${off.name} now takes ${length(off)} frames, ${signed(length(off) - prior.length)} since the last edit.`;
    return { text, state: "changed" };
  }
  if (played.length === later.length) {
    return { text: "✓ The levels after this one play as they did.", state: "" };
  }
  const stopped = s.aheadStopped ? ` ${s.aheadStopped}` : "";
  return {
    text: `✓ The levels after this one play as they did, of those played again: ${played.length} of ${later.length}.${stopped}`,
    state: s.aheadStopped ? "changed" : "",
  };
}

let levelOptions = "";

/** The real time a level takes, as the game page would let its frames go; null until played whole. */
function levelTime(s: Session, segment: Segment): string | null {
  const from = s.clock[segment.start];
  const to = s.clock[segment.end];
  return from === undefined || to === undefined ? null : formatDuration(duration(to - from));
}

function drawLevels(s: Session, segments: readonly Segment[], current: number): void {
  const times = segments.map((t) => levelTime(s, t));
  const key = segments
    .map((t, i) => `${t.name}:${length(t)}:${delta(t)}:${t.checked}:${changed(t)}:${times[i]}`)
    .join(",");
  if (key !== levelOptions) {
    levelOptions = key;
    levelSelect.replaceChildren(
      ...segments.map((t, i) => {
        const d = delta(t);
        return new Option(`${levelName(t)} · ${length(t)} f${d ? ` ${signed(d)}` : ""}`, String(i));
      }),
    );
    segmentBar.replaceChildren(
      ...segments.map((t, i) => {
        const button = document.createElement("button");
        button.type = "button";
        button.dataset["index"] = String(i);
        button.dataset["state"] = !t.checked ? "waiting" : changed(t) ? "changed" : "";
        const name = document.createElement("strong");
        name.textContent = levelName(t);
        const frames = document.createElement("span");
        frames.textContent = `${length(t)} f `;
        const d = delta(t);
        const moved = document.createElement("span");
        moved.className = d > 0 ? "delta more" : "delta";
        moved.textContent = signed(d);
        frames.append(moved);
        const time = document.createElement("span");
        time.textContent = times[i] ?? "–";
        button.append(name, frames, time);
        button.title = `Frames ${t.start}–${t.end - 1}, in real time as a run with these inputs would take them. Click to go to its start.`;
        return button;
      }),
    );
  }

  levelSelect.value = String(current);
  [...segmentBar.children].forEach((child, i) => {
    const on = i === current;
    if (child.classList.contains("current") !== on) {
      child.classList.toggle("current", on);
      if (on) child.scrollIntoView({ block: "nearest", inline: "nearest" });
    }
  });

  const segment = segments[current]!;
  const { text, state } = checkLevel(s, segments, current);
  check.textContent = text;
  check.dataset["state"] = state;

  const d = delta(segment);
  element("level-frame").textContent =
    `${s.frame - segment.start} / ${length(segment)}${d ? ` (${signed(d)})` : ""}`;
}

function show(id: string, value: number | null | undefined): void {
  element(id).textContent = value === null || value === undefined ? "–" : String(value);
}

function drawPlayer(s: Session): void {
  const player = s.player;
  show("speed", player ? s.moved(s.frame - 1) : null);
  element("walkspeed").textContent = player
    ? `Walkspeed ${player.walkspeed}, else ${player.walkspeed2}`
    : "Not in a level";
  show("player-x", player?.x);
  show("player-y", player?.y);
  element("runcount").textContent = player ? `${player.runcount}` : "–";
  show("jumpspeed", player?.jumpspeed);
  show("fallspeed", player?.fallspeed);
}

/**
 * The level's speed across the strip, a column of pixels to a few frames:
 * as tall as the mean, and coloured by the slowest frame in it - full speed,
 * less, or none. Grey is outside a level, and nothing at all not played yet.
 */
function drawStrip(s: Session, segment: Segment): void {
  const scale = devicePixelRatio;
  const width = Math.max(1, Math.round(strip.clientWidth * scale));
  const height = Math.max(1, Math.round(strip.clientHeight * scale));
  if (strip.width !== width || strip.height !== height) {
    strip.width = width;
    strip.height = height;
  }

  const style = getComputedStyle(document.documentElement);
  const colour = (name: string) => style.getPropertyValue(name).trim();
  const full = colour("--held");
  const slow = colour("--slow");
  const stopped = colour("--record");
  const outside = colour("--lag");

  const g = strip.getContext("2d")!;
  g.clearRect(0, 0, width, height);

  const frames = Math.max(1, length(segment));
  let under = 0;
  for (let f = segment.start; f < segment.end && f < s.known; f++) {
    const moved = s.moved(f);
    if (moved !== null && moved < FULL_SPEED) under++;
  }

  for (let x = 0; x < width; x++) {
    const from = segment.start + Math.floor((x * frames) / width);
    const to = Math.max(from + 1, segment.start + Math.floor(((x + 1) * frames) / width));
    let sum = 0;
    let count = 0;
    let slowest = Infinity;
    let any = false;
    for (let f = from; f < to && f < s.known; f++) {
      any = true;
      const moved = s.moved(f);
      if (moved === null) continue;
      sum += moved;
      count++;
      slowest = Math.min(slowest, moved);
    }
    if (!any) continue;
    if (count === 0) {
      g.fillStyle = outside;
      g.fillRect(x, height * 0.75, 1, height * 0.25);
      continue;
    }
    const mean = Math.max(0, sum / count);
    const h = Math.max(0.15, Math.min(1, mean / FULL_SPEED)) * height;
    g.fillStyle = slowest >= FULL_SPEED ? full : slowest > 0 ? slow : stopped;
    g.fillRect(x, height - h, 1, h);
  }

  const at = ((s.frame - segment.start) / frames) * width;
  g.fillStyle = colour("--ink");
  g.fillRect(Math.round(at) - scale, 0, 2 * scale, height);

  element("strip-level").textContent = `Speed through ${levelName(segment)}`;
  element("strip-stats").textContent = `${under} frames under ${FULL_SPEED} px/f`;
}

function draw(): void {
  const s = session;
  if (!s) return;

  const movie = s.movie;
  const segments = s.segments();
  const current = segmentAt(segments, s.frame);
  drawLevels(s, segments, current);
  drawPlayer(s);
  drawStrip(s, segments[current]!);

  element("frame").textContent = String(s.frame);
  element("length").textContent = `${movie.inputs.length} frames`;
  element("lag").textContent = String(s.lag.filter(Boolean).length);
  element("rerecords").textContent = String(movie.rerecords);
  const run = s.runTime();
  const time = element("time");
  time.textContent = run ? formatDuration(duration(run.ms)) : "–";
  time.classList.toggle("finished", run?.finished ?? false);
  const snapshots = s.greenzone.frames();
  element("snapshots").textContent =
    `${snapshots.length} (${(s.greenzone.bytes / 1048576).toFixed(1)} MB)`;

  playButton.textContent = s.playing ? "❚❚" : "▶︎";
  recordButton.setAttribute("aria-pressed", String(s.recording));

  const bookmarks = new Map<number, number>();
  [...bookmarkBar.children].forEach((child, slot) => {
    const button = child as HTMLButtonElement;
    const frame = movie.bookmarks[slot] ?? null;
    const key = (slot + 1) % 10;
    if (frame !== null) bookmarks.set(frame, slot);
    button.classList.toggle("empty", frame === null);
    button.textContent = frame === null ? String(key) : `${key}: ${frame}`;
    button.title =
      (frame === null ? `Bookmark ${key} is empty.` : `Bookmark ${key} is frame ${frame}.`) +
      ` Click to go there, Shift+click to set it to the current frame.`;
  });

  const problems = [
    s.error,
    s.machine.ended,
    saveFailed ? "The movie could not be saved in this browser: export it to keep it." : null,
  ].filter(Boolean);
  status.textContent = [...problems, notice.trim()].filter(Boolean).join(" ") ||
    `Movie for the ${CALCS[movie.calc].name}, made on build ${movie.build}.`;

  roll.render({
    inputs: movie.inputs,
    frame: s.frame,
    lag: s.lag,
    keyframes: new Set(snapshots),
    bookmarks,
    selected,
    label: (frame) => {
      const segment = segments[segmentAt(segments, frame)]!;
      return {
        text: String(frame - segment.start),
        title: `Frame ${frame} of the movie, ${frame - segment.start} of ${levelName(segment)}`,
      };
    },
    speed: (frame) => s.moved(frame),
  });
  if (s.frame !== lastFrame) {
    roll.follow(s.frame);
    lastFrame = s.frame;
  }
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------

for (const [calc, { name }] of Object.entries(CALCS)) {
  calcSelect.add(new Option(name, calc));
}
for (const [lang, { language }] of Object.entries(texts)) {
  langSelect.add(new Option(language, lang));
}

for (let slot = 0; slot < BOOKMARKS; slot++) {
  const button = document.createElement("button");
  button.type = "button";
  button.addEventListener("click", (e) => bookmark(slot, e.shiftKey));
  bookmarkBar.append(button);
}

function bookmark(slot: number, set: boolean): void {
  if (!session) return;
  if (set) session.setBookmark(slot);
  else session.gotoBookmark(slot);
}

function togglePlay(): void {
  if (!session) return;
  if (session.playing) session.pause();
  else session.play();
}

function toggleRecord(): void {
  if (!session) return;
  session.recording = !session.recording;
  render();
}

function back(): void {
  session?.back();
}

function toStart(): void {
  if (!session) return;
  session.pause();
  session.seek(0);
}

/** Goes to the start of a level, counted from the one the game is in. */
function toLevel(by: number): void {
  const s = session;
  if (!s) return;
  const segments = s.segments();
  const i = Math.max(0, Math.min(segments.length - 1, segmentAt(segments, s.frame) + by));
  s.pause();
  s.seek(segments[i]!.start);
}

element("start").addEventListener("click", () => toLevel(0));
element("prev-level").addEventListener("click", () => toLevel(-1));
element("next-level").addEventListener("click", () => toLevel(1));
levelSelect.addEventListener("change", () => {
  const segment = session?.segments()[Number(levelSelect.value)];
  if (!session || !segment) return;
  session.pause();
  session.seek(segment.start);
});
segmentBar.addEventListener("click", (e) => {
  const button = (e.target as Element).closest<HTMLElement>("button[data-index]");
  const segment = session?.segments()[Number(button?.dataset["index"])];
  if (!session || !segment) return;
  session.pause();
  session.seek(segment.start);
});
strip.addEventListener("click", (e) => {
  const s = session;
  if (!s) return;
  const segments = s.segments();
  const segment = segments[segmentAt(segments, s.frame)]!;
  const at = (e.clientX - strip.getBoundingClientRect().left) / strip.clientWidth;
  s.pause();
  s.seek(segment.start + Math.floor(at * length(segment)));
});
element("back").addEventListener("click", back);
playButton.addEventListener("click", togglePlay);
element("advance").addEventListener("click", () => session?.advance());
recordButton.addEventListener("click", toggleRecord);
speedSelect.addEventListener("change", () => {
  if (session) session.speed = Number(speedSelect.value);
});
element("insert").addEventListener("click", () => session?.insert(selected));
element("delete").addEventListener("click", () => session?.remove(selected));
element("truncate").addEventListener("click", () => session?.truncate());

form.addEventListener("submit", (e) => {
  e.preventDefault();
  const length = session?.movie.inputs.length ?? 0;
  if (
    length > 0 &&
    !confirm(`Start a new movie? The one open now (${length} frames) is gone unless it has been exported.`)
  ) {
    return;
  }
  open(emptyMovie(calcSelect.value as Calc, langSelect.value, build), "New movie.");
});

element("import").addEventListener("click", () => fileInput.click());
fileInput.addEventListener("change", async () => {
  const file = fileInput.files?.[0];
  fileInput.value = "";
  if (!file) return;

  try {
    const movie = parse(await file.text());
    await open(movie, `Opened ${file.name}.`);
  } catch (e) {
    status.textContent = `${file.name} could not be opened: ${e instanceof Error ? e.message : String(e)}`;
  }
});

element("export").addEventListener("click", () => {
  if (!session) return;
  const blob = new Blob([serialize(session.movie)], { type: "text/plain" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = "super-mario-68k.tas.txt";
  link.click();
  URL.revokeObjectURL(link.href);
});

// ---------------------------------------------------------------------------
// Keys
//
// The tool's own keys come first; a game key bound to one of them cannot be
// held, and the help says which.
// ---------------------------------------------------------------------------

const HOTKEYS: Record<string, (e: KeyboardEvent) => void> = {
  KeyP: togglePlay,
  KeyF: () => session?.advance(),
  KeyR: back,
  KeyT: toggleRecord,
  Home: toStart,
  PageUp: () => toLevel(-1),
  PageDown: () => toLevel(1),
  Insert: () => session?.insert(selected),
  Delete: () => session?.remove(selected),
};
for (let slot = 0; slot < BOOKMARKS; slot++) {
  HOTKEYS[`Digit${(slot + 1) % 10}`] = (e) => bookmark(slot, e.shiftKey);
}

const conflicts = [...gameKeys.keys()].filter((code) => code in HOTKEYS);
element("conflicts").textContent = conflicts.length
  ? `Bound to the game and taken by the tool here: ${conflicts.join(", ")}.`
  : "";

function typing(e: KeyboardEvent): boolean {
  return e.target instanceof Element && e.target.matches("input, select, textarea");
}

addEventListener("keydown", (e) => {
  if (typing(e) || e.ctrlKey || e.metaKey || e.altKey) return;

  const hotkey = HOTKEYS[e.code];
  if (hotkey) {
    e.preventDefault();
    hotkey(e);
    return;
  }
  if (gameKeys.has(e.code)) {
    e.preventDefault();
    held.add(e.code);
  }
});

// Space is the jump, and a focused button would take its keyup as a click.
addEventListener("keyup", (e) => {
  held.delete(e.code);
  if (!typing(e) && (gameKeys.has(e.code) || e.code in HOTKEYS)) e.preventDefault();
});

addEventListener("blur", () => held.clear());

// ---------------------------------------------------------------------------

const saved = savedMovie();
await open(saved ?? defaultMovie(), saved ? "Picked up where this browser left off." : "New movie.");
