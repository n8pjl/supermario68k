// A run in progress, read off its events: where it stands now, for the route
// search to plan the rest of the run from.
//
// The history is written as the game plays (see speedrun/history.ts), so the
// newest attempt, while it has no `ended`, is the run being played - timed or
// practice alike. Its events say which world it is in, what has been beaten
// there, and what the player walked into and came out of each stage as and
// holding. That is a state the search knows: see Start in search/src/input.rs.
//
// A death moves the player too, the way the game does (see Handle_player_map
// in src/map.cpp): back to the last level they cleared in the world, or its
// start. A Bros. beaten since is still beaten, but is not somewhere the game
// sends them back to, and a level flown over on a cloud is in the way again,
// with the cloud gone. A game over's continue loads the world afresh and the
// player with it, and the game says so only by announcing the same world
// again.
//
// What the events do not say is guessed at, and the guesses are the search's
// to make: where on the map the player stands is taken as the last thing they
// beat, and a rock is taken as broken where they could not have got past it
// otherwise, or where a hammer went from the list on the way to somewhere. A
// cloud or an item spent on the map shows up in the list the next stage is
// walked into with. The practice panel's own edits - a level cleared by hand,
// a powerup handed over - report nothing, and show up the same way, or not at
// all; one that takes a hammer away looks like a rock being broken. So does
// walking through a pipe on the map, which the game sends a player who dies
// back to the far end of, and which nothing reports.

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
  /** Where the player is taken to be standing: the last of them, or after a
   *  death the last level cleared. */
  readonly at: string | null;
  readonly power: Power;
  readonly items: readonly Item[];
  /** The stage or fight being played now, and what it was walked into as. */
  readonly inside: { readonly place: string; readonly entry: Entry } | null;
  /** Places walked into with a hammer fewer than before, once per hammer: a
   *  rock was broken on the way to each. */
  readonly rocks: readonly string[];
}

export interface Live {
  readonly attempt: Attempt;
  readonly start: Start;
  /** Over: finished, abandoned, or reset. The route has nothing left to say. */
  readonly over: boolean;
  /** Sent back by a death, and nothing walked into since. */
  readonly died: boolean;
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
  let rocks: string[] = [];
  let over = false;
  let died = false;
  let since = 0;
  /** Where a death sends the player back to. */
  let cleared: string | null = null;
  /** Something has been walked into since the world was last arrived in. */
  let played = false;
  /** The next announcement of a world is the landing of a warp. */
  let warped = false;

  const arrive = (w: number) => {
    world = w;
    done = [];
    at = null;
    inside = null;
    rocks = [];
    cleared = null;
    played = false;
  };
  const hammers = (list: readonly Item[]) => list.filter((i) => i === "hammer").length;

  for (const { at: when, event } of attempt.events) {
    since = when;
    if (event.kind !== "player-hit") died = event.kind === "player-died";
    switch (event.kind) {
      case "run-started":
        arrive(0);
        power = "small";
        items = [];
        over = false;
        warped = false;
        break;
      case "run-ended":
      case "run-abandoned":
        over = true;
        break;
      // The warp zone is a room to pick a pipe in, not a world the route
      // crosses: the pipe out of it arrives somewhere, and says so.
      case "world-entered":
        // The world it is already in, not landed in by a warp, and played
        // in: a continue after a game over, which starts the player over.
        if (event.world === world && played && !warped) {
          power = "small";
          items = [];
        }
        warped = false;
        if (event.world !== WARP_ZONE_WORLD) arrive(event.world);
        break;
      case "warp-taken":
        warped = true;
        if (event.world !== WARP_ZONE_WORLD) arrive(event.world);
        break;
      case "level-entered":
      case "monster-fought":
        // A practice game from a save starts wherever it was saved, with no
        // world-entered of its own.
        if (event.world !== world) arrive(event.world);
        inside = { place: placeKey(event)!, entry: entryOf(event.player) };
        // A hammer is only ever used on the map, on the way here.
        for (let n = hammers(items) - hammers(event.player.items); n > 0; n--) rocks.push(inside.place);
        power = event.player.power;
        items = event.player.items;
        played = true;
        break;
      case "level-completed":
      case "monster-defeated": {
        if (event.world !== world) arrive(event.world);
        const place = placeKey(event)!;
        if (!done.includes(place)) done.push(place);
        at = place;
        if (event.kind === "level-completed") cleared = place;
        inside = null;
        if (event.player !== undefined) {
          power = event.player.power;
          items = event.player.items;
        }
        break;
      }
      case "player-died":
        at = cleared;
        inside = null;
        power = "small";
        items = event.player.items;
        break;
      case "player-hit":
        break;
    }
  }

  return { attempt, start: { world, done, at, power, items, inside, rocks }, over, died, since };
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
