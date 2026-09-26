// What happened in each stage: the events of an attempt cut into visits.
//
// A visit is one go at a level or an overworld monster, from walking in to
// whatever ended it. The game reports the walking in with what the player
// carried (level-entered, monster-fought) and reports a clear and a death, but
// nothing for the other ways out - a reset, a warp, the page closing - so a
// visit that is still open when something else starts is closed then, as left.
//
// Everything the page says about a stage is added up from these.

import {
  BOWSER_LEVEL,
  type GameEvent,
  type Loadout,
  type Power,
} from "../speedrun/events.ts";
import { levelName, monsterName, placeOrder } from "../speedrun/names.ts";
import { type Attempt } from "./attempts.ts";

export type Outcome = "cleared" | "died" | "warped" | "left";

export interface Place {
  /** Stable, and what places are grouped by: `L<world>.<level>` or `M…`. */
  readonly key: string;
  readonly world: number;
  readonly level: number | null;
  readonly monster: number | null;
}

export interface Visit {
  readonly attempt: Attempt;
  readonly place: Place;
  readonly player: Loadout;
  /** Milliseconds into the attempt. */
  readonly start: number;
  readonly end: number;
  readonly outcome: Outcome;
  readonly hits: number;
  /** What each hit took: the power just before it, in order. */
  readonly hitAs: readonly Power[];
  /**
   * What a cleared visit was beaten as and holding, where the game said so;
   * null for anything else, and for history from before it did.
   */
  readonly exit: Loadout | null;
}

export function levelPlace(world: number, level: number): Place {
  return { key: `L${world}.${level}`, world, level, monster: null };
}

export function monsterPlace(world: number, monster: number): Place {
  return { key: `M${world}.${monster}`, world, level: null, monster };
}

/** Where an event says it happened, if it names a level or a monster. */
function placeOf(event: GameEvent): Place | null {
  if (!("world" in event)) return null;

  if ("level" in event && event.level !== undefined) {
    return levelPlace(event.world, event.level);
  }
  if ("monster" in event && event.monster !== undefined) {
    return monsterPlace(event.world, event.monster);
  }
  return null;
}

/** A place as the map shows it; see speedrun/names.ts. */
export function placeName(place: Place): string {
  return place.monster !== null
    ? monsterName(place.world, place.monster)
    : levelName(place.world, place.level!);
}

function order(place: Place): number {
  return (
    placeOrder(
      place.world,
      place.monster !== null ? { monster: place.monster } : { level: place.level! },
    ) ?? Number.MAX_SAFE_INTEGER
  );
}

/**
 * Sorted the way a world reads - its numbered levels, the rest of its stages,
 * then its monsters - and anything the manifest does not know after the rest
 * of its world, by index.
 */
export function comparePlaces(a: Place, b: Place): number {
  return (
    a.world - b.world ||
    order(a) - order(b) ||
    Number(a.monster !== null) - Number(b.monster !== null) ||
    (a.level ?? a.monster ?? 0) - (b.level ?? b.monster ?? 0)
  );
}

export function visitsOf(attempt: Attempt): Visit[] {
  const out: Visit[] = [];
  let open: {
    place: Place;
    player: Loadout;
    start: number;
    hits: number;
    hitAs: Power[];
  } | null = null;

  const close = (end: number, outcome: Outcome, exit: Loadout | null = null) => {
    if (open === null) return;
    out.push({ attempt, ...open, end, outcome, exit });
    open = null;
  };

  for (const { at, event } of attempt.events) {
    switch (event.kind) {
      case "level-entered":
      case "monster-fought":
        close(at, "left");
        open = { place: placeOf(event)!, player: event.player, start: at, hits: 0, hitAs: [] };
        break;

      case "level-completed":
      case "monster-defeated":
        if (open !== null && open.place.key === placeOf(event)?.key) {
          close(at, "cleared", event.player ?? null);
        }
        break;

      case "player-hit":
        if (open !== null) {
          open.hits++;
          open.hitAs.push(event.player.power);
        }
        break;

      case "player-died":
        close(at, "died");
        break;

      case "warp-taken":
        close(at, "warped");
        break;

      // Bowser's castle is never completed: the ending takes over from it.
      case "run-ended":
        close(
          at,
          open?.place.key === levelPlace(7, BOWSER_LEVEL).key ? "cleared" : "left",
        );
        break;

      case "run-started":
      case "run-abandoned":
      case "world-entered":
        close(at, "left");
        break;
    }
  }

  close(attempt.events.at(-1)?.at ?? 0, "left");
  return out;
}

export interface PlaceStats {
  readonly place: Place;
  readonly visits: readonly Visit[];
  readonly entered: number;
  readonly cleared: number;
  readonly died: number;
  readonly hits: number;
  /** Clear times, fastest first. */
  readonly clears: readonly number[];
  /** Time spent in visits that ended in a death. */
  readonly lost: number;
}

export function statsByPlace(visits: readonly Visit[]): PlaceStats[] {
  const groups = new Map<string, Visit[]>();

  for (const visit of visits) {
    const list = groups.get(visit.place.key);
    if (list === undefined) groups.set(visit.place.key, [visit]);
    else list.push(visit);
  }

  return [...groups.values()]
    .map((list) => {
      const clears = list
        .filter((v) => v.outcome === "cleared")
        .map((v) => v.end - v.start)
        .sort((a, b) => a - b);

      return {
        place: list[0]!.place,
        visits: list,
        entered: list.length,
        cleared: clears.length,
        died: list.filter((v) => v.outcome === "died").length,
        hits: list.reduce((sum, v) => sum + v.hits, 0),
        clears,
        lost: list
          .filter((v) => v.outcome === "died")
          .reduce((sum, v) => sum + v.end - v.start, 0),
      };
    })
    .sort((a, b) => comparePlaces(a.place, b.place));
}

export function median(sorted: readonly number[]): number | null {
  if (sorted.length === 0) return null;

  const mid = sorted.length >> 1;
  return sorted.length % 2 === 1
    ? sorted[mid]!
    : (sorted[mid - 1]! + sorted[mid]!) / 2;
}

/** What a loadout amounts to, for grouping visits by it. */
export function loadoutLabel(player: Loadout): string {
  const extras = [player.star && "star", player.pwing && "P-wing"].filter(
    Boolean,
  );

  return [player.power, ...extras].join(" + ");
}

export const POWER_ORDER: readonly Power[] = ["small", "super", "fire", "racoon"];

/**
 * Where an unfinished run stopped: the last place it was in, if the run
 * stopped inside one rather than on the map between them.
 */
export function stoppedAt(attempt: Attempt, visits: readonly Visit[]): Visit | null {
  if (attempt.finished) return null;

  const last = visits.at(-1);
  return last === undefined || last.outcome === "cleared" ? null : last;
}
