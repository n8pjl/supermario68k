// What the route search is asked, and what it answers: the settings the page
// keeps, and the plan it lays out.
//
// The search itself is Rust, under search/, run as wasm on a worker (see
// worker.ts); this is its interface, said once for both sides of the worker.
// Everything here crosses from the worker to the page, so it is plain data.

import { type CategoryId } from "../speedrun/category.ts";
import { type Item, type Power } from "../speedrun/events.ts";
import { type Entry } from "./model.ts";

export type Objective = "median" | "best";

export const OBJECTIVES: readonly Objective[] = ["best", "median"];

export interface Settings {
  readonly category: CategoryId;
  readonly objective: Objective;
  /** Spend items from the list: power-ups, stars, P-wings, clouds, hammers. */
  readonly items: boolean;
  /** Walk out of the way for a Bros.' item. */
  readonly detours: boolean;
  /** A stage with no history at all: assume `unknownMs` for it, or avoid it. */
  readonly unknown: "assume" | "avoid";
  readonly unknownMs: number;
  /**
   * Where the history has nothing for a stage walked in as something: borrow
   * from a stronger power too, or only ever from a weaker one, so that no
   * figure is faster than what was walked in with has earned.
   */
  readonly borrowUp: boolean;
  /** One square of map, walked or sailed. */
  readonly msPerTile: number;
  /** Going into anything and coming back out to the map. */
  readonly overheadMs: number;
  /** A pipe's passage from one side of a map to the other. */
  readonly pipeMs: number;
  /** A whistle, from the item list to the warp zone and down a pipe. */
  readonly warpMs: number;
}

/**
 * The map moves at 4 pixels a frame at 30 frames a second (map_speed and
 * MAP_FPS), which is 16 pixels, one square, in 4 frames. The rest are guesses
 * the page lets the player correct.
 */
export const DEFAULTS: Settings = {
  category: "any-warpless",
  objective: "best",
  items: true,
  detours: true,
  unknown: "assume",
  unknownMs: 60_000,
  borrowUp: true,
  msPerTile: (4 / 30) * 1000,
  overheadMs: 1500,
  pipeMs: 8000,
  warpMs: 6000,
};

/** The items a route spends, in the order the item list is kept. */
export type Slot = "mushroom" | "fire-flower" | "leaf" | "star" | "p-wing" | "cloud" | "hammer" | "whistle";

export type Source = "data" | "borrowed" | "assumed";

export type ActionKind = "stage" | "cloud" | "bros" | "rock" | "warp";

/**
 * One way the search can take a stage walked into as something: going for one
 * of the ways the history has seen it come out (see Variant in model.ts).
 */
export interface Costing {
  readonly ms: number;
  /** What is aimed for: leaving as this, holding these. */
  readonly exit: Power;
  readonly gains: readonly Item[];
  readonly source: Source;
  /** Whose figures were used, where they were borrowed. */
  readonly from: string | null;
  /** Clears of the variant aimed for. */
  readonly clears: number;
}

export interface Alternative {
  readonly kind: ActionKind;
  readonly warp: number | null;
  readonly node: number;
  readonly place: string | null;
  readonly use: readonly Slot[];
  readonly entry: Entry | null;
  /** How much longer the whole run takes for choosing it. */
  readonly deltaMs: number;
  readonly source: Source | null;
  /** The way out it goes for, where it is a stage or a fight. */
  readonly exit: Power | null;
  readonly gains: readonly string[];
}

export interface Step {
  readonly world: number;
  readonly kind: ActionKind;
  /** A warp: the world it goes to. */
  readonly warp: number | null;
  readonly node: number;
  /**
   * Where the player ends up: the thing itself, past a clouded stage, or out
   * the far end of a pipe stage.
   */
  readonly to: number;
  readonly place: string | null;
  readonly use: readonly Slot[];
  readonly entry: Entry | null;
  readonly walkMs: number;
  /** Everything else it takes: the stage, the fight. */
  readonly doMs: number;
  readonly costing: Costing | null;
  /** The item list on the way in, before anything is spent. */
  readonly holding: readonly Slot[];
  /** The time left for the run as this step begins. */
  readonly left: number;
  /** What the player is after it. */
  readonly power: Power;
  /** What it put in the item list: a chest, a Bros.' treasure, a castle's. */
  readonly got: readonly string[];
  /** The next best things to have done instead, best first. */
  readonly alternatives: readonly Alternative[];
}

/** One stage walked in as one thing, as the search costed it. */
export interface Costed {
  readonly place: string;
  readonly entry: string;
  readonly ms: number;
  readonly source: Source;
  readonly from: string | null;
  readonly clears: number;
}

export interface Plan {
  /** Time for the whole run. */
  readonly total: number;
  readonly steps: readonly Step[];
  /** Every stage and entry the search found a use for, and what it made of it. */
  readonly costed: readonly Costed[];
  readonly states: number;
  readonly ms: number;
}
