// Routes: what a run is split on, as data rather than as code.
//
// A route is strictly linear - an ordered list of splits, each closed by one
// kind of event from the game. Where the game forks and either way will do,
// that is two routes rather than one split with two answers: the route says
// which way this run goes, and two routes are compared as wholes.
//
// A trigger is data so that a route survives a round trip through JSON, which
// is what lets one be recorded, exported, edited by hand and shared. It names
// an event kind and pins whichever of that event's fields matter; a field left
// out is one the trigger does not care about, so { kind: "level-completed",
// world: 0 } closes on any level of world 1.

import { type EventKind, type GameEvent, isEventKind } from "./events.ts";

export interface Trigger {
  readonly kind: EventKind;
  readonly world?: number;
  readonly level?: number;
}

export interface RouteSplit {
  /** Stable across edits: what a saved time is matched back up by. */
  readonly id: string;
  readonly name: string;
  readonly on: Trigger;
}

export interface Route {
  readonly id: string;
  /** The rules being run, shared by every route that is a way of running them. */
  readonly category: string;
  readonly name: string;
  readonly splits: readonly RouteSplit[];
  /** Absent on the routes that ship with the game, which cannot be edited. */
  readonly builtIn?: true;
}

export function triggered(on: Trigger, event: GameEvent): boolean {
  if (on.kind !== event.kind) return false;

  const seen: Partial<Record<"world" | "level", number>> = event as never;

  if (on.world !== undefined && on.world !== seen.world) return false;
  if (on.level !== undefined && on.level !== seen.level) return false;

  return true;
}

// ---------------------------------------------------------------------------
// Reading one back from a file
//
// Everything here is checked rather than trusted: a route can arrive from an
// export made by another version of the game, or from a file somebody edited.
// Anything that does not read as a route is rejected whole rather than half
// loaded, so a bad file cannot leave a route that silently never splits.
// ---------------------------------------------------------------------------

function asRecord(value: unknown): Record<string, unknown> | null {
  return typeof value === "object" && value !== null && !Array.isArray(value)
    ? (value as Record<string, unknown>)
    : null;
}

function asIndex(value: unknown): number | null {
  return typeof value === "number" && Number.isInteger(value) && value >= 0
    ? value
    : null;
}

function parseTrigger(value: unknown): Trigger | null {
  const raw = asRecord(value);
  if (raw === null || !isEventKind(raw["kind"])) return null;

  // Built one field at a time rather than spread: an absent field and one
  // present as undefined mean different things to a trigger, and only the
  // first of those is "do not care".
  const trigger: { kind: EventKind; world?: number; level?: number } = {
    kind: raw["kind"],
  };

  for (const field of ["world", "level"] as const) {
    if (raw[field] === undefined) continue;

    const n = asIndex(raw[field]);
    if (n === null) return null;

    trigger[field] = n;
  }

  return trigger;
}

function parseSplit(value: unknown): RouteSplit | null {
  const raw = asRecord(value);
  if (raw === null) return null;

  const { id, name } = raw;
  if (typeof id !== "string" || id === "") return null;
  if (typeof name !== "string") return null;

  const on = parseTrigger(raw["on"]);
  if (on === null) return null;

  return { id, name, on };
}

export function parseRoute(value: unknown): Route | null {
  const raw = asRecord(value);
  if (raw === null) return null;

  const { id, name, category, splits } = raw;
  if (typeof id !== "string" || id === "") return null;
  if (typeof name !== "string" || name === "") return null;
  if (typeof category !== "string" || category === "") return null;
  if (!Array.isArray(splits) || splits.length === 0) return null;

  const parsed: RouteSplit[] = [];
  const seen = new Set<string>();

  for (const split of splits) {
    const one = parseSplit(split);

    // Times are matched back to splits by id, so a route with the same id
    // twice has no answer to which of them a saved time belongs to.
    if (one === null || seen.has(one.id)) return null;

    seen.add(one.id);
    parsed.push(one);
  }

  return { id, name, category, splits: parsed };
}

/** A route as it goes to file: without the flag that marks the shipped ones. */
export function routeToJSON(route: Route): unknown {
  return {
    id: route.id,
    category: route.category,
    name: route.name,
    splits: route.splits.map((split) => ({
      id: split.id,
      name: split.name,
      on: split.on,
    })),
  };
}

// ---------------------------------------------------------------------------
// The routes that ship with the game
//
// Data like any other route, and the same shape an exported one has, so they
// double as the worked example for anyone writing one by hand.
// ---------------------------------------------------------------------------

/** Worlds in the shipped levelset - Levelsetdata.Nr_of_files in sm68k. */
const WORLDS = 8;

// A world is done the moment the next one's map comes up, which is the event
// carrying that world's own number: CurrentWorld counts from zero, so arriving
// at the world shown as "WORLD 2" reports 1 and closes World 1. The last world
// has no next one to arrive at - beating Bowser is what closes it.
function worldSplit(world: number): RouteSplit {
  return {
    id: `world-${world}`,
    name: `World ${world}`,
    on:
      world < WORLDS
        ? { kind: "world-entered", world }
        : { kind: "run-ended" },
  };
}

export const ANY_PERCENT: Route = {
  id: "any",
  category: "Any%",
  name: "Any% — by world",
  splits: Array.from({ length: WORLDS }, (_, i) => worldSplit(i + 1)),
  builtIn: true,
};

function levelSplit(world: number, level: number, name: string): RouteSplit {
  return {
    id: `w${world}-l${level}`,
    name,
    on: { kind: "level-completed", world: world - 1, level },
  };
}

// One world, split at every level in it: the route for trying the timer out
// without playing the whole game. The order is world 1's own map read outwards
// from where Mario starts on it - levels 0-3, the fortress at level 6, then 4
// and 5, and the castle at level 7. The run finishes when that last split
// closes, so it ends with world 1 rather than waiting on a Bowser it never
// reaches.
export const WORLD_1: Route = {
  id: "world-1",
  category: "World 1",
  name: "World 1 — every level",
  splits: [
    levelSplit(1, 0, "1-1"),
    levelSplit(1, 1, "1-2"),
    levelSplit(1, 2, "1-3"),
    levelSplit(1, 3, "1-4"),
    levelSplit(1, 6, "Fortress"),
    levelSplit(1, 4, "1-5"),
    levelSplit(1, 5, "1-6"),
    levelSplit(1, 7, "Castle"),
  ],
  builtIn: true,
};

export const BUILT_IN_ROUTES: readonly Route[] = [ANY_PERCENT, WORLD_1];
