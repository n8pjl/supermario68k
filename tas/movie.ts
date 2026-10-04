// A movie: what a TAS is, as opposed to how it was made. The game is
// deterministic from power-on given the same build, the same calculator, the
// same language and the same input on every frame, so those are the whole of
// it - the snapshots that make rewinding cheap are the tool's, rebuilt from
// this whenever they are needed, and never saved.
//
// On disk it is text, one line per frame, so that two takes of a movie can be
// diffed and a frame can be fixed by hand:
//
//   SM68K-TAS 1
//   calc ti92
//   lang en
//   build 3f9c0a17be42
//   rerecords 118
//   bookmark 1 340
//   input
//   |........|
//   |.R..J.F.|
//
// The header is a line each, in any order, before `input`; lines starting with
// # are comments anywhere. After `input`, line n is frame n's buttons, in the
// fixed order of BUTTONS below - a letter where it is held and a dot where it
// is not. The letters are only there to be read: a parser goes by position.

/** The game's eight actions, in the order a frame line lists them. */
export const BUTTONS = [
  "left",
  "right",
  "up",
  "down",
  "jump",
  "run",
  "enter",
  "esc",
] as const;

export type Button = (typeof BUTTONS)[number];

/** The column letters: F for run, which also throws fire, and X for esc. */
export const BUTTON_LETTERS = "LRUDJFEX";

export const BUTTON_LABELS: Record<Button, string> = {
  left: "Left",
  right: "Right",
  up: "Up",
  down: "Down",
  jump: "Jump",
  run: "Run / fire",
  enter: "Enter",
  esc: "Esc",
};

/** One frame's buttons, bit i held for BUTTONS[i]. */
export type Input = number;

export type Calc = "ti92" | "ti89";

export const CALCS: Record<Calc, { name: string; width: number; height: number }> = {
  ti92: { name: "TI-92 Plus / Voyage 200", width: 240, height: 128 },
  ti89: { name: "TI-89", width: 160, height: 100 },
};

/** How many bookmark slots there are: the digit keys, 1 to 9 and then 0. */
export const BOOKMARKS = 10;

export interface Movie {
  calc: Calc;
  /** A key of ma_texts.json. */
  lang: string;
  /**
   * The game build the movie was made on, as buildId() names it. A movie
   * played on any other build is played on a different game, which may well
   * act differently on the same input: it is loaded anyway, and warned about.
   */
  build: string;
  /** How many times the movie was rewound to be recorded over. */
  rerecords: number;
  /** A frame per slot, or null where the slot is empty. */
  bookmarks: (number | null)[];
  inputs: Input[];
}

const MAGIC = "SM68K-TAS";
const VERSION = 1;

export function emptyMovie(calc: Calc, lang: string, build: string): Movie {
  return {
    calc,
    lang,
    build,
    rerecords: 0,
    bookmarks: Array(BOOKMARKS).fill(null),
    inputs: [],
  };
}

export function held(input: Input, button: number): boolean {
  return (input & (1 << button)) !== 0;
}

/** The shape Module.gameActions() hands the game. */
export function actions(input: Input): Record<Button, boolean> {
  const out = {} as Record<Button, boolean>;
  BUTTONS.forEach((button, i) => {
    out[button] = held(input, i);
  });
  return out;
}

export function frameLine(input: Input): string {
  let line = "|";
  for (let i = 0; i < BUTTONS.length; i++) {
    line += held(input, i) ? BUTTON_LETTERS[i] : ".";
  }
  return line + "|";
}

export function serialize(movie: Movie): string {
  const lines = [
    `${MAGIC} ${VERSION}`,
    `calc ${movie.calc}`,
    `lang ${movie.lang}`,
    `build ${movie.build}`,
    `rerecords ${movie.rerecords}`,
  ];
  movie.bookmarks.forEach((frame, slot) => {
    if (frame !== null) lines.push(`bookmark ${slot} ${frame}`);
  });
  lines.push("input");
  for (const input of movie.inputs) lines.push(frameLine(input));
  return lines.join("\n") + "\n";
}

export class MovieError extends Error {
  constructor(line: number, message: string) {
    super(`line ${line}: ${message}`);
  }
}

function count(text: string, line: number, what: string): number {
  const n = Number(text);
  if (!Number.isSafeInteger(n) || n < 0) {
    throw new MovieError(line, `${what} is not a count: ${text}`);
  }
  return n;
}

export function parse(text: string): Movie {
  const lines = text.split(/\r?\n/);
  const movie = emptyMovie("ti92", "en", "");
  const seen = new Set<string>();
  let n = 0;

  const first = lines[0]?.trim().split(/\s+/) ?? [];
  if (first[0] !== MAGIC) {
    throw new MovieError(1, "not a Super Mario 68K movie");
  }
  if (Number(first[1]) !== VERSION) {
    throw new MovieError(1, `format version ${first[1]} is not one this reads`);
  }

  for (n = 1; n < lines.length; n++) {
    const line = lines[n]!.trim();
    if (line === "" || line.startsWith("#")) continue;
    if (line === "input") break;

    const [key, ...rest] = line.split(/\s+/);
    const value = rest.join(" ");
    const at = n + 1;

    switch (key) {
      case "calc":
        if (value !== "ti92" && value !== "ti89") {
          throw new MovieError(at, `unknown calculator ${value}`);
        }
        movie.calc = value;
        break;
      case "lang":
        movie.lang = value;
        break;
      case "build":
        movie.build = value;
        break;
      case "rerecords":
        movie.rerecords = count(value, at, "rerecords");
        break;
      case "bookmark": {
        const slot = count(rest[0] ?? "", at, "the bookmark slot");
        if (slot >= BOOKMARKS) {
          throw new MovieError(at, `there is no bookmark slot ${slot}`);
        }
        movie.bookmarks[slot] = count(rest[1] ?? "", at, "the bookmark frame");
        break;
      }
      default:
        throw new MovieError(at, `unknown header ${key}`);
    }
    if (key !== "bookmark") seen.add(key!);
  }

  for (const key of ["calc", "lang", "build"]) {
    if (!seen.has(key)) throw new MovieError(n + 1, `no ${key} in the header`);
  }
  if (n === lines.length) throw new MovieError(n, "no input section");

  for (n++; n < lines.length; n++) {
    const line = lines[n]!.trim();
    if (line === "" || line.startsWith("#")) continue;

    const m = /^\|(.{8})\|$/.exec(line);
    if (!m) {
      throw new MovieError(n + 1, `not a frame: ${line}`);
    }

    let input = 0;
    for (let i = 0; i < BUTTONS.length; i++) {
      if (m[1]![i] !== ".") input |= 1 << i;
    }
    movie.inputs.push(input);
  }

  return movie;
}

/**
 * Names a game build by the bytes of its wasm: the first twelve hex digits of
 * their SHA-256, the same length tools/mkdist.py puts in the served names.
 */
export async function buildId(wasm: ArrayBuffer): Promise<string> {
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", wasm));
  return Array.from(digest.subarray(0, 6), (b) =>
    b.toString(16).padStart(2, "0"),
  ).join("");
}
