// The TAS page: the game's TAS build on the left, and on the right the panel a
// movie is played, stepped, rewound and edited in. See session.ts for how a
// movie and the game are kept in step, and src/tas.h for what the build does
// differently from the one the game ships as.
//
// The movie being worked on is saved in this browser as it changes, and comes
// back on the next visit. Exporting is the only way it leaves.

import { BOOKMARKS, BUTTONS, buildId, CALCS, emptyMovie, parse, serialize, type Calc, type Input, type Movie } from "./movie.ts";
import { Machine, type Factory } from "./machine.ts";
import { Roll } from "./roll.ts";
import { Session } from "./session.ts";

// Beside this script, here and in dist/ - where tools/mkdist.py rewrites each
// to its hashed name.
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

const canvas = element<HTMLCanvasElement>("canvas");
const status = element("status");
const form = element<HTMLFormElement>("movie");
const calcSelect = form.elements.namedItem("calc") as HTMLSelectElement;
const langSelect = form.elements.namedItem("lang") as HTMLSelectElement;
const fileInput = element<HTMLInputElement>("file");
const playButton = element<HTMLButtonElement>("play");
const recordButton = element<HTMLButtonElement>("record");
const speedSelect = element<HTMLSelectElement>("speed");
const bookmarkBar = element("bookmarks");

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

const [factory, wasm, texts] = await Promise.all([
  import(GLUE_URL).then((m: { default: Factory }) => m.default),
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

function fitCanvas(): void {
  const room = (canvas.parentElement!.clientWidth - 24) / canvas.width;
  const scale = Math.max(1, Math.min(MAX_SCALE, room));
  canvas.style.width = `${Math.floor(canvas.width * scale)}px`;
  canvas.style.height = `${Math.floor(canvas.height * scale)}px`;
}

addEventListener("resize", fitCanvas);

async function open(movie: Movie, why: string): Promise<void> {
  session?.pause();
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
  canvas.width = CALCS[movie.calc].width;
  canvas.height = CALCS[movie.calc].height;
  fitCanvas();

  const machine = await Machine.boot({
    factory,
    wasm,
    canvas,
    calc: movie.calc,
    texts: texts[lang]!.texts,
    input: (frame) => movie.inputs[frame] ?? 0,
    onCanvasResize: fitCanvas,
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

function draw(): void {
  const s = session;
  if (!s) return;

  const movie = s.movie;
  element("frame").textContent = String(s.frame);
  element("length").textContent = `${movie.inputs.length} frames`;
  element("lag").textContent = String(s.lag.filter(Boolean).length);
  element("rerecords").textContent = String(movie.rerecords);
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

element("start").addEventListener("click", toStart);
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
