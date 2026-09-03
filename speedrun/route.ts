// Routes: what a run is split on, as data rather than as code.
//
// A route is strictly linear - an ordered list of splits, each closed by one
// kind of event from the game. Where the game forks and either way will do,
// that is two routes rather than one split with two answers: the route says
// which way this run goes, and two routes are compared as wholes.
//
// A route belongs to exactly one category, which is the set of rules it is a
// way of running; see category.ts. The category is not just a label on the
// route - it is checked against the splits, so a Route value is one its
// category has agreed to have.
//
// A trigger is data so that a route survives a round trip through JSON, which
// is what lets one be recorded, exported, edited by hand and shared. It names
// an event kind and pins whichever of that event's fields matter; a field left
// out is one the trigger does not care about, so { kind: "level-completed",
// world: 0 } closes on any level of world 1.

import { type CategoryId, category, isCategoryId } from "./category.ts";
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
  readonly category: CategoryId;
  readonly name: string;
  readonly splits: readonly RouteSplit[];
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

  const { id, name, category: under, splits } = raw;
  if (typeof id !== "string" || id === "") return null;
  if (typeof name !== "string" || name === "") return null;
  if (!Array.isArray(splits) || splits.length === 0) return null;

  // A category this build does not have is not a category: the route is for a
  // set of rules nothing here can compare a time under, so it does not load.
  if (!isCategoryId(under)) return null;

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

  // The route has to obey the rules it claims to be run under. Checked here,
  // at the one door routes come in through, so that everything downstream can
  // take a Route's category at its word - a route filed under Any% warpless
  // has been read, and its splits do not skip a world.
  if (!category(under).admits(parsed)) return null;

  return { id, name, category: under, splits: parsed };
}

/** A route as it goes to file, which is also how it is kept in storage. */
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
