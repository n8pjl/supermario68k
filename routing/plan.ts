// A search, read out as a route: what the page shows.
//
// The search answers "what is best from here" for any state; a route is the
// answers strung together from a new game, following at each chance - a chest,
// a hit taken or not - the outcome most likely to happen. Each step keeps the
// options it beat and by how much, which is what says whether a choice is a
// clear one or a coin toss the data could turn either way.
//
// Everything here crosses from the worker to the page, so it is plain data.

import { type Power } from "../speedrun/events.ts";
import { MAPS } from "./maps.ts";
import { type Entry, POWERS, type Stats, spent } from "./model.ts";
import {
  type Action,
  type Costing,
  type Settings,
  type Slot,
  type Source,
  type State,
  inventory,
  search,
} from "./search.ts";

export interface Alternative {
  readonly kind: Action["kind"];
  readonly warp: number | null;
  readonly node: number;
  readonly place: string | null;
  readonly use: readonly Slot[];
  readonly entry: Entry | null;
  /** How much longer the whole run is expected to take for choosing it. */
  readonly deltaMs: number;
  readonly source: Source | null;
  /** The way out it goes for, where it is a stage or a fight. */
  readonly exit: Power | null;
  readonly gains: readonly string[];
}

export interface Outcome {
  readonly p: number;
  /** What the player is after it. */
  readonly power: Power;
  /** What it put in the item list: a chest, a Bros.' treasure, a castle's. */
  readonly got: readonly string[];
}

export interface Step {
  readonly world: number;
  readonly kind: Action["kind"];
  /** A warp: the world it goes to. */
  readonly warp: number | null;
  readonly node: number;
  readonly to: number;
  readonly place: string | null;
  readonly use: readonly Slot[];
  readonly entry: Entry | null;
  readonly walkMs: number;
  readonly doMs: number;
  readonly costing: Costing | null;
  /** The item list on the way in, before anything is spent. */
  readonly holding: readonly Slot[];
  /** The expected time left for the run as this step begins. */
  readonly left: number;
  readonly outcomes: readonly Outcome[];
  /** Which of those the route goes on from: the likeliest. */
  readonly then: number;
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
  /** Expected time for the whole run. */
  readonly total: number;
  /**
   * Whether `total` is only an upper bound. With detours it is worked out by
   * following the route through every chance outcome, and where that is too
   * much it is the first step's figure instead, which the route can only
   * better.
   */
  readonly bound: boolean;
  readonly steps: readonly Step[];
  /** Every stage and entry the search found a use for, and what it made of it. */
  readonly costed: readonly Costed[];
  readonly states: number;
  readonly ms: number;
}

/**
 * How many states more than the route took the whole run's expected time may
 * cost. With detours, following every chance outcome to the end can take
 * several times the route itself, for a figure the route's first step already
 * bounds.
 */
const FOLLOW_BUDGET = 100_000;

/** How many options each step keeps besides the one taken. */
const ALTERNATIVES = 3;

export function plan(stats: Stats, settings: Settings): Plan {
  const began = performance.now();
  const found = search(stats, settings);
  const steps: Step[] = [];

  let state: State = found.start;
  while (state.world < MAPS.length) {
    const ranked = found.choices(state, 1 + ALTERNATIVES);
    const best = ranked[0];
    if (best === undefined || best.total === Infinity) {
      throw new Error(
        `No way through world ${state.world + 1} under these rules: every way through ` +
          "it goes by a stage with no history, and those are set to be avoided.",
      );
    }

    const a = best.action;
    const kept = inventory(state.inv).slice();
    for (const item of a.use) kept.splice(kept.indexOf(item), 1);

    const outcomes = a.outcomes.map(([p, next]) => ({
      p,
      power: POWERS[next.power]!,
      got: spent(inventory(next.inv), kept),
    }));
    const then = outcomes.reduce((most, o, i) => (o.p > outcomes[most]!.p ? i : most), 0);

    steps.push({
      world: state.world,
      kind: a.kind,
      warp: a.warp,
      node: a.node,
      to: a.to,
      place: a.place,
      use: a.use,
      entry: a.entry,
      walkMs: a.walkMs,
      doMs: a.doMs,
      costing: a.costing,
      holding: inventory(state.inv),
      left: best.total,
      outcomes,
      then,
      alternatives: ranked.slice(1, 1 + ALTERNATIVES).map(({ action, total }) => ({
        kind: action.kind,
        warp: action.warp,
        node: action.node,
        place: action.place,
        use: action.use,
        entry: action.entry,
        // Never below nothing: a tie can add up a hair either way.
        deltaMs: Math.max(0, total - best.total),
        source: action.costing?.source ?? null,
        exit: action.costing?.exit ?? null,
        gains: action.costing?.gains ?? [],
      })),
    });

    state = a.outcomes[then]![1];
  }

  // The exact figure, if it can be had for a little more work than the route
  // took; otherwise the first step's, which the route can only better.
  const exact = found.total(FOLLOW_BUDGET);
  const total = exact ?? steps[0]?.left ?? 0;
  const bound = exact === null;

  const costed: Costed[] = [];
  for (const [id, list] of found.coster.costed) {
    const [place, entry] = id.split("|") as [string, string];
    const c = list[0];
    if (c === undefined) continue;
    costed.push({
      place,
      entry,
      ms: c.ms,
      source: c.source,
      from: c.from,
      clears: list.reduce((n, v) => n + v.clears, 0),
    });
  }

  return {
    total,
    bound,
    steps,
    costed,
    states: found.states(),
    ms: performance.now() - began,
  };
}

