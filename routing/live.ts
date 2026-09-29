// A run in progress, read off its events: where it stands now, for the route
// search to plan the rest of the run from.
//
// The history is written as the game plays (see speedrun/history.ts), so the
// newest attempt, while it has no `ended`, is the run being played - timed or
// practice alike. Its events say which world it is in, what has been beaten
// there, and what the player walked into and came out of each stage as and
// holding. That is a state the search knows: see Start in search/src/input.rs.
//
// What the events do not say is guessed at, and the guesses are the search's
// to make: where on the map the player stands is taken as the last thing they
// beat, and a rock is taken as broken where they could not have got past it
// otherwise. A cloud or an item spent on the map shows up in the list the next
// stage is walked into with. The practice panel's own edits - a level cleared
// by hand, a powerup handed over - report nothing, and show up the same way,
// or not at all.

import {
  type GameEvent,
  type Item,
  type Power,
  WARP_ZONE_WORLD,
} from "../speedrun/events.ts";
import { type Attempt } from "../analysis/attempts.ts";
import { type Entry, entryOf } from "./model.ts";

/** What the search is handed; the same shape as Start in input.rs. */
export interface Start {
  readonly world: number;
  /** Place keys beaten in this world, in the order they were. */
  readonly done: readonly string[];
  /** The last of them, where the player is taken to be standing. */
  readonly at: string | null;
  readonly power: Power;
  readonly items: readonly Item[];
  /** The stage or fight being played now, and what it was walked into as. */
  readonly inside: { readonly place: string; readonly entry: Entry } | null;
}

export interface Live {
  readonly attempt: Attempt;
  readonly start: Start;
  /** Over: finished, abandoned, or reset. The route has nothing left to say. */
  readonly over: boolean;
  /** Milliseconds into the attempt of the event the state was last moved by. */
  readonly since: number;
}

/**
 * How long an attempt can go without an event and still be taken as being
 * played: one whose tab was closed on it never gets an `ended`, and is not a
 * run in progress a day later.
 */
const STALE_MS = 2 * 60 * 60 * 1000;

function placeKey(event: GameEvent): string | null {
  if (!("world" in event)) return null;
  if ("level" in event && event.level !== undefined) return `L${event.world}.${event.level}`;
  if ("monster" in event && event.monster !== undefined) return `M${event.world}.${event.monster}`;
  return null;
}

/** Where an attempt stands after all of its events. */
export function standing(attempt: Attempt): Live {
  let world = 0;
  let done: string[] = [];
  let at: string | null = null;
  let power: Power = "small";
  let items: readonly Item[] = [];
  let inside: Start["inside"] = null;
  let over = false;
  let since = 0;

  const arrive = (w: number) => {
    world = w;
    done = [];
    at = null;
    inside = null;
  };

  for (const { at: when, event } of attempt.events) {
    since = when;
    switch (event.kind) {
      case "run-started":
        arrive(0);
        power = "small";
        items = [];
        over = false;
        break;
      case "run-ended":
      case "run-abandoned":
        over = true;
        break;
      // The warp zone is a room to pick a pipe in, not a world the route
      // crosses: the pipe out of it arrives somewhere, and says so.
      case "world-entered":
      case "warp-taken":
        if (event.world !== WARP_ZONE_WORLD) arrive(event.world);
        break;
      case "level-entered":
      case "monster-fought":
        // A practice game from a save starts wherever it was saved, with no
        // world-entered of its own.
        if (event.world !== world) arrive(event.world);
        inside = { place: placeKey(event)!, entry: entryOf(event.player) };
        power = event.player.power;
        items = event.player.items;
        break;
      case "level-completed":
      case "monster-defeated": {
        if (event.world !== world) arrive(event.world);
        const place = placeKey(event)!;
        if (!done.includes(place)) done.push(place);
        at = place;
        inside = null;
        if (event.player !== undefined) {
          power = event.player.power;
          items = event.player.items;
        }
        break;
      }
      case "player-died":
        inside = null;
        power = "small";
        items = event.player.items;
        break;
      case "player-hit":
        break;
    }
  }

  return { attempt, start: { world, done, at, power, items, inside }, over, since };
}

/**
 * The run being played now, if there is one: the newest attempt, while it is
 * open and has heard from the game lately. Anything else is null, and the
 * route is from a new game.
 */
export function inProgress(attempts: Iterable<Attempt>): Live | null {
  let newest: Attempt | null = null;
  for (const a of attempts) {
    if (newest === null || a.started.epochMilliseconds > newest.started.epochMilliseconds) newest = a;
  }
  if (newest === null || newest.ended !== null) return null;

  const last = newest.started.epochMilliseconds + (newest.events.at(-1)?.at ?? 0);
  if (Date.now() - last > STALE_MS) return null;

  const live = standing(newest);
  return live.over ? null : live;
}
