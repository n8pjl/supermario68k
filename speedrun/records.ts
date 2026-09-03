// What a run was worth, and the file it travels in.
//
// Two things are kept per route: the best whole run, and the best each split
// has ever been on its own. The second is what a sum of best is added up from,
// and the two are independent - a sum of best is a time nobody has run, made of
// segments from runs that were otherwise nothing special.
//
// A time is tied to a split by its id rather than by where it sits in the
// route, so inserting a split into a route leaves the times either side of it
// where they were.

import { type SplitGroup } from "./groups.ts";
import { type Route, timedSplits } from "./route.ts";
import { duration, parseDuration, shorter } from "./times.ts";

export interface SplitTime {
  readonly id: string;
  /** Time on the clock when the split closed; null if the run skipped it. */
  readonly at: Temporal.Duration | null;
}

export interface RunRecord {
  readonly finished: boolean;
  readonly total: Temporal.Duration;
  readonly splits: readonly SplitTime[];
}

export interface RouteRecord {
  readonly route: string;
  readonly pb: RunRecord | null;
  /** Best segment - not cumulative - for each split id that has ever closed. */
  readonly best: ReadonlyMap<string, Temporal.Duration>;
  /**
   * Best segment for each world group that has ever closed as a whole.
   *
   * Kept apart from `best` rather than added up from it: a world's best segment
   * is the fastest that world has been run start to end in one go, which is not
   * the sum of the best its levels have ever been on their own. Keyed by
   * SplitGroup.id; see groups.ts.
   */
  readonly bestGroups: ReadonlyMap<string, Temporal.Duration>;
}

export function emptyRecord(route: string): RouteRecord {
  return { route, pb: null, best: new Map(), bestGroups: new Map() };
}

/** The cumulative time a run reached at one split, if it closed it. */
export function timeAt(run: RunRecord, id: string): Temporal.Duration | null {
  return run.splits.find((split) => split.id === id)?.at ?? null;
}

/**
 * A run's segment times: how long each split took on its own.
 *
 * A split that follows skipped ones covers them too - the time is real, it just
 * cannot be divided up - so it is not a segment for any one of them, and only
 * the split that closed gets it.
 */
export function segments(run: RunRecord): Map<string, Temporal.Duration> {
  const out = new Map<string, Temporal.Duration>();
  let previous = duration(0);
  let skipped = false;

  for (const split of run.splits) {
    if (split.at === null) {
      skipped = true;
      continue;
    }

    if (!skipped) out.set(split.id, split.at.subtract(previous));

    previous = split.at;
    skipped = false;
  }

  return out;
}

/**
 * A run's world-segment times: how long each group took start to end.
 *
 * A group's start is the clock at the last split that closed before it, or zero
 * where it opens the run; its end is its own last split. Skipped splits inside a
 * group do not stop it having a segment - a warp through the middle of a world
 * is part of that world's time - but a group whose last split was skipped has no
 * clean end and gets none.
 */
export function groupSegments(
  run: RunRecord,
  groups: readonly SplitGroup[],
): Map<string, Temporal.Duration> {
  const out = new Map<string, Temporal.Duration>();

  for (const group of groups) {
    const end = run.splits[group.to]?.at ?? null;
    if (end === null) continue;

    let start = duration(0);
    for (let i = group.from - 1; i >= 0; i--) {
      const at = run.splits[i]?.at;
      if (at != null) {
        start = at;
        break;
      }
    }

    out.set(group.id, end.subtract(start));
  }

  return out;
}

/** The record after a run: a new best if it beat one, and any better segments. */
export function withRun(
  record: RouteRecord,
  run: RunRecord,
  groups: readonly SplitGroup[] = [],
): RouteRecord {
  const best = new Map(record.best);

  for (const [id, segment] of segments(run)) {
    const was = best.get(id);

    best.set(id, was === undefined ? segment : shorter(was, segment));
  }

  const bestGroups = new Map(record.bestGroups);

  for (const [id, segment] of groupSegments(run, groups)) {
    const was = bestGroups.get(id);

    bestGroups.set(id, was === undefined ? segment : shorter(was, segment));
  }

  // Only a finished run can be a personal best; an abandoned one is not a time
  // over the route at all, however far it got. Its segments still count.
  const pb =
    run.finished &&
    (record.pb === null ||
      Temporal.Duration.compare(run.total, record.pb.total) < 0)
      ? run
      : record.pb;

  return { route: record.route, pb, best, bestGroups };
}

/**
 * The route run with every split at its best, or null if one has never closed.
 *
 * Null rather than a partial total: a sum missing a segment is not a time the
 * route could be run in, and showing it as one would flatter every comparison
 * made against it.
 *
 * Warps are left out - they are not a segment the run keeps on its own (see
 * route.ts), and their time is already inside the split that follows them - so
 * this stays a total the route could really be run in.
 *
 * Takes the bests as a map rather than a whole record so that a run in progress
 * can hand in the record's bests with its own golds already folded through
 * them: a segment that has just beaten its best is part of the sum from the
 * moment it closes, not from whenever the run is over.
 */
export function sumOfBest(
  route: Route,
  best: ReadonlyMap<string, Temporal.Duration>,
): Temporal.Duration | null {
  let total = duration(0);

  for (const split of timedSplits(route.splits)) {
    const segment = best.get(split.id);
    if (segment === undefined) return null;

    total = total.add(segment);
  }

  return total.round({ largestUnit: "hour", smallestUnit: "millisecond" });
}

// ---------------------------------------------------------------------------
// The file
//
// One document holds routes and times together, so a route and the run it was
// set on travel as one file. Import merges: a route or a record already held
// under the same id is replaced, everything else is left alone, which is what
// makes it safe to take a file from another machine without losing what is
// here.
// ---------------------------------------------------------------------------

export const FORMAT = "sm68k.speedrun";
export const FORMAT_VERSION = 1;

function asRecordObject(value: unknown): Record<string, unknown> | null {
  return typeof value === "object" && value !== null && !Array.isArray(value)
    ? (value as Record<string, unknown>)
    : null;
}

function parseRun(value: unknown): RunRecord | null {
  const raw = asRecordObject(value);
  if (raw === null) return null;

  const total = parseDuration(raw["total"]);
  if (total === null || !Array.isArray(raw["splits"])) return null;

  const splits: SplitTime[] = [];

  for (const entry of raw["splits"]) {
    const split = asRecordObject(entry);
    if (split === null || typeof split["id"] !== "string") return null;

    splits.push({
      id: split["id"],
      at: split["at"] === null ? null : parseDuration(split["at"]),
    });
  }

  return { finished: raw["finished"] !== false, total, splits };
}

export function parseRouteRecord(value: unknown): RouteRecord | null {
  const raw = asRecordObject(value);
  if (raw === null || typeof raw["route"] !== "string") return null;

  const best = new Map<string, Temporal.Duration>();
  const rawBest = asRecordObject(raw["best"]) ?? {};

  for (const [id, time] of Object.entries(rawBest)) {
    const parsed = parseDuration(time);
    if (parsed !== null) best.set(id, parsed);
  }

  // Absent in a file written before world groups: an older record still loads,
  // it just has no world bests until a run sets some.
  const bestGroups = new Map<string, Temporal.Duration>();
  const rawGroups = asRecordObject(raw["groups"]) ?? {};

  for (const [id, time] of Object.entries(rawGroups)) {
    const parsed = parseDuration(time);
    if (parsed !== null) bestGroups.set(id, parsed);
  }

  return {
    route: raw["route"],
    pb: raw["pb"] === undefined || raw["pb"] === null ? null : parseRun(raw["pb"]),
    best,
    bestGroups,
  };
}

function runToJSON(run: RunRecord): unknown {
  return {
    finished: run.finished,
    total: run.total.toString(),
    splits: run.splits.map((split) => ({
      id: split.id,
      at: split.at?.toString() ?? null,
    })),
  };
}

export function recordToJSON(record: RouteRecord): unknown {
  return {
    route: record.route,
    pb: record.pb === null ? null : runToJSON(record.pb),
    best: Object.fromEntries(
      [...record.best].map(([id, time]) => [id, time.toString()]),
    ),
    groups: Object.fromEntries(
      [...record.bestGroups].map(([id, time]) => [id, time.toString()]),
    ),
  };
}
