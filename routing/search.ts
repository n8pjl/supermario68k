// The route search: the fastest way through Any% warpless, as the history
// says the player plays it.
//
// A run is a walk across eight maps, choosing as it goes: which stage to play
// next, which to leave, whether to walk to a mushroom house or a Bros. for an
// item, and what to spend from the item list before going in. What each
// choice costs comes from the history (see model.ts), by stage and by what
// the stage is walked into as; what the map allows comes from maps.ts.
//
// Some outcomes are chance: a chest in a mushroom house, and what a stage
// leaves the player as - a hit on the way costs the suit, and a death sends
// them back in small. So this is an expectimax: at each state the choice with
// the least expected time left, where the time left after a chance outcome is
// the average over the outcomes, weighted as the history weights them. With
// the objective set to a median or a best time, deaths are left out and each
// stage leaves the player as it most often did, which makes it a plain
// shortest path through the same states.
//
// A state is: the world, what has been done in it (stages beaten, houses
// visited, Bros. fought, rocks broken), where the player stands, their power,
// and the item list. Everything a state can do next either does something new
// or spends an item, so nothing leads back to where it started, and each state
// is worked out once and remembered.
//
// Where the history has nothing for a stage walked into as something, the
// figure is borrowed - from the same stage entered as something close, or,
// failing that, assumed - and every figure that was not simply read off the
// history is marked, so the page can say which parts of a route rest on it.

import { type Item, type Power } from "../speedrun/events.ts";
import { type MapNode, MAPS, type TreasureItem, type WorldMap } from "./maps.ts";
import {
  type Entry,
  POWERS,
  type Stats,
  type Summary,
  entryKey,
} from "./model.ts";

export type Objective = "expected" | "median" | "best";

export interface Settings {
  readonly objective: Objective;
  /** Spend items from the list: power-ups, stars, P-wings, clouds, hammers. */
  readonly items: boolean;
  /** Walk out of the way for items: mushroom houses and Bros. */
  readonly detours: boolean;
  /** A stage with no history at all: assume `unknownMs` for it, or avoid it. */
  readonly unknown: "assume" | "avoid";
  readonly unknownMs: number;
  /** One square of map, walked or sailed. */
  readonly msPerTile: number;
  /** Going into anything and coming back out to the map. */
  readonly overheadMs: number;
  /** A death: the animation, the map, and walking back in. */
  readonly respawnMs: number;
  /** A mushroom house, chest and all. */
  readonly houseMs: number;
  /** A pipe's passage from one side of a map to the other. */
  readonly pipeMs: number;
  /** An airship that flew off on a death, walked after. */
  readonly chaseMs: number;
}

/**
 * The map moves at 4 pixels a frame at 30 frames a second (map_speed and
 * MAP_FPS), which is 16 pixels, one square, in 4 frames. The rest are guesses
 * the page lets the player correct.
 */
export const DEFAULTS: Settings = {
  objective: "expected",
  items: true,
  detours: true,
  unknown: "assume",
  unknownMs: 60_000,
  msPerTile: (4 / 30) * 1000,
  overheadMs: 1500,
  respawnMs: 4000,
  houseMs: 6000,
  pipeMs: 8000,
  chaseMs: 8000,
};

/** The items a route spends, in the order the item list is kept in a state. */
export const SLOTS = [
  "mushroom",
  "fire-flower",
  "leaf",
  "star",
  "p-wing",
  "cloud",
  "hammer",
  "anchor",
] as const satisfies readonly Item[];

export type Slot = (typeof SLOTS)[number];

/**
 * How many of one item a state keeps count of. More are dropped: a third leaf
 * in hand changes nothing a second does not, and the count is what the number
 * of states grows with.
 */
const CAP = 2;
const BASE = CAP + 1;
const WEIGHT = SLOTS.map((_, i) => BASE ** i);

function count(inv: number, slot: number): number {
  return Math.floor(inv / WEIGHT[slot]!) % BASE;
}

function give(inv: number, slot: number): number {
  return count(inv, slot) >= CAP ? inv : inv + WEIGHT[slot]!;
}

function take(inv: number, slot: number): number {
  return inv - WEIGHT[slot]!;
}

export function inventory(inv: number): Slot[] {
  return SLOTS.flatMap((slot, i) => Array<Slot>(count(inv, i)).fill(slot));
}

const S = Object.fromEntries(SLOTS.map((slot, i) => [slot, i])) as Record<Slot, number>;

/** A chest in a mushroom house: Handle_treasure_all() on Fg_plane.step. */
const RANDOM_CHEST: readonly [number, number][] = [
  [S.mushroom, 0.25],
  [S["fire-flower"], 0.25],
  [S.leaf, 0.5],
];

function treasure(item: TreasureItem | string | null): readonly [number, number][] {
  if (item === "random") return RANDOM_CHEST;
  const slot = SLOTS.indexOf(item as Slot);
  return slot === -1 ? [] : [[slot, 1]];
}

// ---------------------------------------------------------------------------
// The maps, made ready to search

interface Link {
  readonly to: number;
  readonly ms: number;
  readonly door: 1 | 2 | 0;
  /** The rock's bit, or 0. */
  readonly rock: number;
}

type ThingKind = "stage" | "house" | "bros" | "rock";

interface Thing {
  readonly kind: ThingKind;
  /** Unique across every world: where its costings are kept; see Coster. */
  readonly index: number;
  readonly bit: number;
  readonly node: number;
  /** The place key visits are grouped by: stages and Bros. */
  readonly place: string | null;
  readonly treasure: readonly [number, number][];
}

interface World {
  readonly map: WorldMap;
  readonly links: readonly (readonly Link[])[];
  /** Each node's stage, if it is one - the castle and Bowser's included. */
  readonly stage: readonly (Thing | null)[];
  readonly houses: readonly Thing[];
  readonly bros: readonly Thing[];
  readonly rocks: readonly Thing[];
  /** The fortresses that open each kind of door, as a mask. */
  readonly opens: readonly [number, number, number];
  readonly start: number;
}

let things = 0;

function prepare(map: WorldMap, settings: Settings): World {
  let bits = 0;
  const thing = (
    kind: ThingKind,
    node: number,
    place: string | null,
    loot: readonly [number, number][] = [],
  ): Thing => ({ kind, index: things++, bit: 1 << bits++, node, place, treasure: loot });

  const stage = map.nodes.map((n: MapNode) =>
    n.level !== undefined ? thing("stage", n.id, `L${map.world}.${n.level}`) : null,
  );
  const houses = map.nodes
    .filter((n) => n.kind === "house")
    .map((n) => thing("house", n.id, null, RANDOM_CHEST));
  const bros = map.bros.map((b) =>
    thing("bros", b.node, `M${map.world}.${b.monster}`, treasure(b.treasure)),
  );
  const rocks = map.rocks.map((r) => thing("rock", r.from, null));

  const opens: [number, number, number] = [0, 0, 0];
  for (const n of map.nodes) {
    if (n.opens !== undefined) opens[n.opens] |= stage[n.id]!.bit;
  }

  const links: Link[][] = map.nodes.map(() => []);
  for (const e of map.edges) {
    links[e.a]!.push({
      to: e.b,
      ms: e.by === "pipe" ? settings.pipeMs : e.tiles * settings.msPerTile,
      door: e.door ?? 0,
      rock: e.rock === undefined ? 0 : rocks[e.rock]!.bit,
    });
  }

  return { map, links, stage, houses, bros, rocks, opens, start: map.start };
}

// ---------------------------------------------------------------------------
// What a stage costs

export type Source = "data" | "borrowed" | "assumed";

/** What the search took a stage walked into as something to cost. */
export interface Costing {
  readonly ms: number;
  /** The power it leaves the player as, and how likely: sums to 1. */
  readonly exits: readonly (readonly [number, number])[];
  readonly source: Source;
  /** Whose figures were used, where they were borrowed. */
  readonly from: string | null;
  readonly clears: number;
  /** The chance of a death on any one go, in the expected objective. */
  readonly death: number;
}

function exitsOf(summary: Summary, keep: "all" | "mode"): [number, number][] {
  const counts = POWERS.map((p) => summary.exits[p] ?? 0);
  const total = counts.reduce((a, b) => a + b, 0);

  if (keep === "mode") {
    const most = counts.indexOf(Math.max(...counts));
    return [[most, 1]];
  }
  return counts.flatMap((c, i) => (c === 0 ? [] : [[i, c / total] as [number, number]]));
}

function mix(
  a: readonly (readonly [number, number])[],
  wa: number,
  b: readonly (readonly [number, number])[],
  wb: number,
): [number, number][] {
  const out = new Map<number, number>();
  for (const [p, w] of a) out.set(p, (out.get(p) ?? 0) + w * wa);
  for (const [p, w] of b) out.set(p, (out.get(p) ?? 0) + w * wb);
  return [...out];
}

/**
 * The figures a stage walked in as `entry` is costed from: its own, else the
 * same stage walked in as the nearest thing to it - without the star or the
 * P-wing first, then the other powers, nearest first and the weaker side of a
 * tie first, since a borrowed time is better too slow than too fast.
 */
function lookup(
  stats: Stats,
  place: string,
  entry: Entry,
): { summary: Summary; key: string; exact: boolean; samePower: boolean } | null {
  const here = stats[place];
  if (here === undefined) return null;

  const usable = (key: string) => (here[key]?.clears ?? 0) > 0;
  const key = entryKey(entry);
  if (usable(key)) return { summary: here[key]!, key, exact: true, samePower: true };

  const plain = entryKey({ power: entry.power, star: false, pwing: false });
  if (usable(plain)) return { summary: here[plain]!, key: plain, exact: false, samePower: true };

  const rank = POWERS.indexOf(entry.power);
  const order = POWERS.map((p, i) => ({ p, d: Math.abs(i - rank), weaker: i < rank }))
    .filter(({ d }) => d > 0)
    .sort((a, b) => a.d - b.d || Number(b.weaker) - Number(a.weaker));

  for (const { p } of order) {
    const other = entryKey({ power: p, star: false, pwing: false });
    if (usable(other)) return { summary: here[other]!, key: other, exact: false, samePower: false };
  }
  return null;
}

const SMALL: Entry = { power: "small", star: false, pwing: false };

class Coster {
  readonly #cache = new Map<string, Costing>();
  readonly stats: Stats;
  readonly settings: Settings;

  constructor(stats: Stats, settings: Settings) {
    this.stats = stats;
    this.settings = settings;
  }

  cost(place: string, entry: Entry, airship: boolean): Costing {
    const id = `${place}|${entryKey(entry)}|${airship ? 1 : 0}`;
    let found = this.#cache.get(id);

    if (found === undefined) {
      found = this.#work(place, entry, airship);
      this.#cache.set(id, found);
    }
    return found;
  }

  readonly #byThing: (Costing | undefined)[] = [];

  /**
   * The same, looked up by a thing's index rather than by name: the search
   * asks this for every option at every state.
   */
  costOf(thing: Thing, entry: Entry, airship: boolean): Costing {
    const slot =
      thing.index * 32 +
      POWERS.indexOf(entry.power) * 8 +
      (entry.star ? 4 : 0) +
      (entry.pwing ? 2 : 0) +
      (airship ? 1 : 0);
    let found = this.#byThing[slot];
    if (found === undefined) {
      found = this.cost(thing.place!, entry, airship);
      this.#byThing[slot] = found;
    }
    return found;
  }

  /** Every stage and entry costed so far: what the search found reachable. */
  get costed(): IterableIterator<[string, Costing]> {
    return this.#cache.entries();
  }

  #work(place: string, entry: Entry, airship: boolean): Costing {
    const { settings } = this;
    const found = lookup(this.stats, place, entry);
    const power = POWERS.indexOf(entry.power);
    const expected = settings.objective === "expected";
    const isSmall = entryKey(entry) === entryKey(SMALL);

    let clearMs: number;
    let deathMs: number;
    let death: number;
    let exits: [number, number][];

    if (found === null) {
      if (settings.unknown === "avoid") {
        return { ms: Infinity, exits: [[power, 1]], source: "assumed", from: null, clears: 0, death: 0 };
      }
      clearMs = settings.unknownMs;
      deathMs = clearMs / 2;
      death = 0.1;
      exits = [[power, 1]];
    } else {
      const s = found.summary;
      clearMs =
        settings.objective === "best" ? s.best! : settings.objective === "median" ? s.median! : s.mean!;
      deathMs = s.deathMs ?? clearMs / 2;
      // A little prior weight toward "rarely dies", so one death in one go is
      // not read as a stage that kills every time, nor none in two as never.
      death = (s.deaths + 0.25) / (s.clears + s.deaths + 2);
      exits = found.samePower ? exitsOf(s, expected ? "all" : "mode") : [[power, 1]];
    }

    const source: Source = found === null ? "assumed" : found.exact ? "data" : "borrowed";
    const from = found === null || found.exact ? null : found.key;
    const clears = found?.summary.clears ?? 0;

    if (!expected) return { ms: clearMs, exits, source, from, clears, death: 0 };

    const lost = deathMs + settings.respawnMs + (airship ? settings.chaseMs : 0);

    if (isSmall) {
      // Every retry is as small again, so the tries are alike: the expected
      // number of deaths before a clear is death / (1 - death).
      return { ms: clearMs + (death / (1 - death)) * lost, exits, source, from, clears, death };
    }

    const retry = this.cost(place, SMALL, airship);
    return {
      ms: (1 - death) * clearMs + death * (lost + retry.ms),
      exits: mix(exits, 1 - death, retry.exits, death),
      source,
      from,
      clears,
      death,
    };
  }
}

// ---------------------------------------------------------------------------
// The search

/** Something to do next, and what it leads to. */
export interface Action {
  readonly kind: "stage" | "cloud" | "house" | "bros" | "rock";
  readonly node: number;
  /** Where the player ends up: the thing itself, or past a clouded stage. */
  readonly to: number;
  readonly place: string | null;
  readonly use: readonly Slot[];
  readonly entry: Entry | null;
  readonly walkMs: number;
  /** Everything else it takes: the stage, the house, the fight. */
  readonly doMs: number;
  readonly costing: Costing | null;
  /** Chance outcomes: probability, then the state it leaves. */
  readonly outcomes: readonly (readonly [number, State])[];
}

export interface State {
  readonly world: number;
  readonly done: number;
  readonly pos: number;
  readonly power: number;
  readonly inv: number;
}

const MASK_SPAN = 2 ** 24;

/**
 * Numbers by state, in two levels so that every key is a small integer: one
 * number for world, power and items, another for what is done and where the
 * player stands. A state packed into one number would not fit in a small
 * integer, and a map keyed by any other kind of number is several times
 * slower to look things up in, which is most of what the search does.
 */
class Memo {
  readonly #outer = new Map<number, Map<number, number>>();
  size = 0;

  get(s: State): number | undefined {
    return this.#outer.get(outerKey(s))?.get(innerKey(s));
  }

  set(s: State, value: number): void {
    const key = outerKey(s);
    let inner = this.#outer.get(key);
    if (inner === undefined) this.#outer.set(key, (inner = new Map()));
    const before = inner.size;
    inner.set(innerKey(s), value);
    this.size += inner.size - before;
  }
}

/** How many item lists there are: every count of every item. */
const LISTS = BASE ** SLOTS.length;

function outerKey(s: State): number {
  return (s.world * 4 + s.power) * LISTS + s.inv;
}

function innerKey(s: State): number {
  return s.done * 64 + s.pos;
}

export interface Search {
  /** The expected time left from a state, following the route found. */
  readonly value: (s: State) => number;
  /**
   * The expected time for the whole run, following the route through every
   * chance outcome, or null where that would take more than `budget` states
   * more than have been worked out already.
   */
  readonly total: (budget: number) => number | null;
  /** What to do at a state, best first, with the time each leaves. */
  readonly choices: (s: State) => { action: Action; total: number }[];
  readonly start: State;
  readonly worlds: readonly World[];
  readonly coster: Coster;
  readonly states: () => number;
}

/** Thrown when a search has looked at more states than it is allowed. */
export class TooBig extends Error {}

export function search(stats: Stats, settings: Settings, budget = 4_000_000): Search {
  things = 0;
  const worlds = MAPS.map((m) => prepare(m, settings));
  const coster = new Coster(stats, settings);
  const memo = new Memo();

  /**
   * Shortest walks from the player's square, over what is open now. Kept, as
   * they depend on neither power nor items, which most states differ by.
   */
  const walked = new Map<number, Float64Array>();

  function walks(world: World, s: State): Float64Array {
    const key = (s.world * MASK_SPAN + s.done) * 64 + s.pos;
    let found = walked.get(key);
    if (found === undefined) {
      found = walk(world, s);
      walked.set(key, found);
    }
    return found;
  }

  function walk(world: World, s: State): Float64Array {
    const n = world.map.nodes.length;
    const dist = new Float64Array(n).fill(Infinity);
    const settled = new Uint8Array(n);
    dist[s.pos] = 0;

    for (;;) {
      let u = -1;
      for (let i = 0; i < n; i++) {
        if (!settled[i] && dist[i]! < Infinity && (u === -1 || dist[i]! < dist[u]!)) u = i;
      }
      if (u === -1) break;
      settled[u] = 1;

      // A stage not yet beaten is walked onto and no further.
      const st = world.stage[u];
      if (st && !(s.done & st.bit) && u !== s.pos) continue;

      for (const link of world.links[u]!) {
        if (link.door !== 0 && !(s.done & world.opens[link.door])) continue;
        if (link.rock !== 0 && !(s.done & link.rock)) continue;
        const d = dist[u]! + link.ms;
        if (d < dist[link.to]!) dist[link.to] = d;
      }
    }
    return dist;
  }

  /** Walkable to and past, as opposed to walkable onto. */
  function open(world: World, s: State, node: number): boolean {
    const st = world.stage[node];
    return !st || (s.done & st.bit) !== 0;
  }

  /** What can be walked into a stage as, and what it spends. */
  function loadouts(s: State, airship: boolean): { entry: Entry; use: Slot[]; inv: number; anchored: boolean }[] {
    const power = POWERS[s.power]!;
    const base = [{ power, pwing: false, use: [] as Slot[], inv: s.inv }];

    if (settings.items) {
      const offer = (slot: Slot, to: Power, pwing = false) => {
        if (count(s.inv, S[slot]) > 0) {
          base.push({ power: to, pwing, use: [slot], inv: take(s.inv, S[slot]) });
        }
      };
      if (power === "small") offer("mushroom", "super");
      if (power !== "fire") offer("fire-flower", "fire");
      if (power !== "racoon") offer("leaf", "racoon");
      offer("p-wing", "racoon", true);
    }

    const out: { entry: Entry; use: Slot[]; inv: number; anchored: boolean }[] = [];
    for (const b of base) {
      const withStar = settings.items && count(b.inv, S.star) > 0 ? [false, true] : [false];
      for (const star of withStar) {
        const inv = star ? take(b.inv, S.star) : b.inv;
        const use: Slot[] = star ? [...b.use, "star"] : b.use;
        const anchors =
          airship && settings.items && count(inv, S.anchor) > 0 && settings.objective === "expected"
            ? [false, true]
            : [false];

        for (const anchored of anchors) {
          out.push({
            entry: { power: b.power, star, pwing: b.pwing },
            use: anchored ? [...use, "anchor"] : use,
            inv: anchored ? take(inv, S.anchor) : inv,
            anchored,
          });
        }
      }
    }
    return out;
  }

  // An item nothing further on can be spent on is dropped from the state:
  // a hammer after the last rock, an anchor after the last airship. Holding
  // one changes nothing, and two states that differ only by it are one.
  const lastRock = Math.max(-1, ...worlds.filter((w) => w.rocks.length > 0).map((w) => w.map.world));
  const lastAirship =
    settings.objective === "expected"
      ? Math.max(-1, ...worlds.filter((w) => w.map.airship).map((w) => w.map.world))
      : -1;

  function tidy(world: number, inv: number): number {
    let out = inv;
    if (world > lastRock) out -= count(out, S.hammer) * WEIGHT[S.hammer]!;
    if (world > lastAirship) out -= count(out, S.anchor) * WEIGHT[S.anchor]!;
    return out;
  }

  function after(s: State, done: number, pos: number, power: number, inv: number): State {
    return { world: s.world, done, pos, power, inv: tidy(s.world, inv) };
  }

  /** The start of the next world, with the castle's reward in hand. */
  function nextWorld(world: World, power: number, inv: number): State {
    const next = s0(world.map.world + 1);
    let withReward = inv;
    for (const [slot] of treasure(world.map.reward)) withReward = give(withReward, slot);
    return { ...next, power, inv: tidy(next.world, withReward) };
  }

  function s0(w: number): State {
    return { world: w, done: 0, pos: worlds[w]?.start ?? 0, power: 0, inv: 0 };
  }

  function actions(s: State, detours: boolean): Action[] {
    const world = worlds[s.world]!;
    const dist = walks(world, s);
    const out: Action[] = [];
    const nodes = world.map.nodes;

    for (const node of nodes) {
      const st = world.stage[node.id];
      if (!st || s.done & st.bit || dist[node.id] === Infinity) continue;

      const final = node.kind === "castle" || node.kind === "bowser";
      const airship = node.kind === "castle" && world.map.airship;
      const walkMs = dist[node.id]!;

      for (const l of loadouts(s, airship)) {
        const costing = coster.costOf(st, l.entry, airship && !l.anchored);
        const done = s.done | st.bit;
        const outcomes = costing.exits.map(([power, p]) => [
          p,
          final
            ? node.kind === "bowser"
              ? { world: MAPS.length, done: 0, pos: 0, power: 0, inv: 0 }
              : nextWorld(world, power, l.inv)
            : after(s, done, node.id, power, l.inv),
        ] as const);

        out.push({
          kind: "stage",
          node: node.id,
          to: node.id,
          place: st.place,
          use: l.use,
          entry: l.entry,
          walkMs,
          doMs: costing.ms + settings.overheadMs,
          costing,
          outcomes,
        });
      }

      // A cloud carries the player over a stage without playing it, onto
      // whatever is beyond; the stage is still there to block the way back.
      if (settings.items && !final && count(s.inv, S.cloud) > 0) {
        for (const link of world.links[node.id]!) {
          if (dist[link.to] !== Infinity || !open(world, s, link.to)) continue;
          if (link.door !== 0 && !(s.done & world.opens[link.door])) continue;
          if (link.rock !== 0 && !(s.done & link.rock)) continue;

          out.push({
            kind: "cloud",
            node: node.id,
            to: link.to,
            place: st.place,
            use: ["cloud"],
            entry: null,
            walkMs: walkMs + link.ms,
            doMs: 0,
            costing: null,
            outcomes: [[1, after(s, s.done, link.to, s.power, take(s.inv, S.cloud))]],
          });
        }
      }
    }

    if (settings.items && detours) {
      for (const house of world.houses) {
        if (s.done & house.bit || dist[house.node] === Infinity) continue;
        out.push({
          kind: "house",
          node: house.node,
          to: house.node,
          place: null,
          use: [],
          entry: null,
          walkMs: dist[house.node]!,
          doMs: settings.houseMs,
          costing: null,
          outcomes: house.treasure.map(([slot, p]) => [
            p,
            after(s, s.done | house.bit, house.node, s.power, give(s.inv, slot)),
          ]),
        });
      }

      for (const bros of world.bros) {
        if (s.done & bros.bit || dist[bros.node] === Infinity || !open(world, s, bros.node)) continue;

        for (const l of loadouts(s, false)) {
          const costing = coster.costOf(bros, l.entry, false);
          out.push({
            kind: "bros",
            node: bros.node,
            to: bros.node,
            place: bros.place,
            use: l.use,
            entry: l.entry,
            walkMs: dist[bros.node]!,
            doMs: costing.ms + settings.overheadMs,
            costing,
            outcomes: costing.exits.flatMap(([power, p]) =>
              bros.treasure.map(([slot, q]) => [
                p * q,
                after(s, s.done | bros.bit, bros.node, power, give(l.inv, slot)),
              ] as const),
            ),
          });
        }
      }
    }

    if (settings.items && count(s.inv, S.hammer) > 0) {
      for (const rock of world.rocks) {
        if (s.done & rock.bit || dist[rock.node] === Infinity || !open(world, s, rock.node)) continue;
        out.push({
          kind: "rock",
          node: rock.node,
          to: rock.node,
          place: null,
          use: ["hammer"],
          entry: null,
          walkMs: dist[rock.node]!,
          doMs: 0,
          costing: null,
          outcomes: [[1, after(s, s.done | rock.bit, rock.node, s.power, take(s.inv, S.hammer))]],
        });
      }
    }

    return out;
  }

  /** The best one could do from here, keeping out of every house and Bros. */
  function value(s: State): number {
    if (s.world >= MAPS.length) return 0;

    const known = memo.get(s);
    if (known !== undefined) return known;

    if (memo.size >= budget) throw new TooBig();
    // Marked before it is worked out, as a guard: nothing should lead back
    // here, and if something did it would read as a dead end, not loop.
    memo.set(s, Infinity);

    let best = Infinity;
    for (const a of actions(s, false)) {
      const v = lookahead(a);
      if (v < best) best = v;
    }


    memo.set(s, best);
    return best;
  }

  /** An action's time, and the best that can be done after it. */
  function lookahead(a: Action): number {
    let total = a.walkMs + a.doMs;
    if (total === Infinity) return Infinity;
    for (const [p, next] of a.outcomes) total += p * value(next);
    return total;
  }

  /**
   * What to do here, every option ranked by the time it leaves to go: its
   * own, and then the best that can be done without a detour.
   *
   * With detours allowed this is where they come in. Searched exactly, they
   * make the search too big to finish - every house outcome and every Bros.
   * treasure is another item list to work every later world out for - so a
   * detour is weighed here, one at a time, against the best route that takes
   * none after it: the choice at each state is the best single step with
   * everything after it done the no-detour way. That is never worse than
   * taking no detours, and it finds every detour that pays for itself on its
   * own; what it can miss is two that only pay together.
   */
  function choices(s: State): { action: Action; total: number }[] {
    // Between two that come out the same, the one that spends less: an item
    // kept costs nothing, and may yet be wanted.
    return actions(s, settings.detours)
      .map((action) => ({ action, total: lookahead(action) }))
      .sort((x, y) => x.total - y.total || x.action.use.length - y.action.use.length);
  }

  const policyMemo = new Memo();

  /** The expected time left following choices() from here on. */
  let limit = budget;

  function total(extra: number): number | null {
    limit = Math.min(budget, memo.size + policyMemo.size + extra);
    try {
      return follow(s0(0));
    } catch (e) {
      if (!(e instanceof TooBig)) throw e;
      return null;
    } finally {
      limit = budget;
    }
  }

  function follow(s: State): number {
    if (s.world >= MAPS.length) return 0;
    if (!settings.detours) return value(s);

    const known = policyMemo.get(s);
    if (known !== undefined) return known;

    if (memo.size + policyMemo.size >= limit) throw new TooBig();

    policyMemo.set(s, Infinity);
    const best = choices(s)[0];
    let total = Infinity;
    if (best !== undefined && best.total < Infinity) {
      total = best.action.walkMs + best.action.doMs;
      for (const [p, next] of best.action.outcomes) total += p * follow(next);
    }
    policyMemo.set(s, total);
    return total;
  }

  return {
    value: follow,
    total,
    choices,
    start: s0(0),
    worlds,
    coster,
    states: () => memo.size + policyMemo.size,
  };
}
