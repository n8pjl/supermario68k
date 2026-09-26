// The route search: the fastest way through a category, as the history says
// the player plays it.
//
// A run is a walk across eight maps, choosing as it goes: which stage to play
// next, which to leave, whether to walk to a mushroom house or a Bros. for an
// item, and what to spend from the item list before going in. What each
// choice costs comes from the history (see model.ts), by stage and by what
// the stage is walked into as; what the map allows comes from maps.ts.
//
// How a stage comes out is the player's to choose, as far as the history has
// seen it done: 1-1 left as raccoon, 2-Pyramid left with its cloud. Each way
// out the history holds is an option, at the time the clears that came out
// that way took (see Variant in model.ts).
//
// What is left to chance is chance: a chest's pick of three, in a mushroom
// house, a Bros.' random drop or a bonus room. The settings say whether those
// are planned on at their odds - an expectimax, where the time left after a
// chance is the average over its outcomes - or not planned on at all, so that
// no route waits on one. With the objective set to expected time, a death is
// chance too: it sends the player back in small, at the history's death rate.
//
// The category is what says where the run ends and what it may do on the
// way: World 1 is over at world 1's castle; Any% may spend a whistle to warp
// ahead from anywhere on a map; 100% has to have beaten every stage and every
// Bros. in a world before its castle, which makes the Bros. part of the route
// rather than detours from it. See rulesFor().
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

import { type CategoryId } from "../speedrun/category.ts";
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
  readonly category: CategoryId;
  readonly objective: Objective;
  /**
   * Items that come by chance: planned on at their odds ("odds"), or not at
   * all ("sure"), so that a route never waits on a chest to be kind. Only
   * what is certain counts then - a castle's reward, a Bros.' set drop, a
   * find the history has never once come away from a stage without.
   */
  readonly luck: "sure" | "odds";
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
  luck: "sure",
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
  warpMs: 6000,
};

/** What a category asks of a route, as the search needs it said. */
export interface Rules {
  /** The world whose end is the end of the run, counted from zero. */
  readonly lastWorld: number;
  /** Whether the whistle may be spent. */
  readonly warps: boolean;
  /** Whether every stage and Bros. of a world has to be beaten before its end. */
  readonly everything: boolean;
}

/**
 * The rules of each category, read the way speedrun/category.ts writes them.
 * That file checks a route's splits after the fact; this is the same rules
 * told forwards, as what a route may do next.
 */
export function rulesFor(category: CategoryId): Rules {
  const last = MAPS.length - 1;

  switch (category) {
    case "any":
      return { lastWorld: last, warps: true, everything: false };
    case "any-warpless":
      return { lastWorld: last, warps: false, everything: false };
    // A warp would skip a world whose every stage the rules ask for, so there
    // is nothing for one to do here.
    case "100":
      return { lastWorld: last, warps: false, everything: true };
    case "world-1":
      return { lastWorld: 0, warps: false, everything: false };
  }
}

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
  "whistle",
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
  // A chest's pick of three is no item to plan on where only the certain
  // counts: the houses go, and a Bros. that drops one drops nothing.
  const sure = settings.luck === "sure";
  const houses = sure
    ? []
    : map.nodes.filter((n) => n.kind === "house").map((n) => thing("house", n.id, null, RANDOM_CHEST));
  const bros = map.bros.map((b) =>
    thing("bros", b.node, `M${map.world}.${b.monster}`, sure && b.treasure === "random" ? [] : treasure(b.treasure)),
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

/**
 * One way the search can take a stage walked into as something: going for one
 * of the ways the history has seen it come out (see Variant in model.ts).
 */
export interface Costing {
  readonly ms: number;
  /**
   * What it leaves the player as and with: power, how likely, the items it
   * hands over (as SLOTS indices). One outcome, the one aimed for, except in
   * the expected objective, where a death sends the player back in small.
   */
  readonly exits: readonly (readonly [number, number, readonly number[]])[];
  /** What is aimed for: leaving as this, holding these. */
  readonly exit: Power;
  readonly gains: readonly Item[];
  readonly source: Source;
  /** Whose figures were used, where they were borrowed. */
  readonly from: string | null;
  /** Clears of the variant aimed for. */
  readonly clears: number;
  /** The chance of a death on any one go, in the expected objective. */
  readonly death: number;
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

function slotsOf(items: readonly Item[]): number[] {
  return items.map((i) => SLOTS.indexOf(i as Slot)).filter((i) => i !== -1);
}

class Coster {
  readonly #cache = new Map<string, Costing[]>();
  readonly stats: Stats;
  readonly settings: Settings;

  constructor(stats: Stats, settings: Settings) {
    this.stats = stats;
    this.settings = settings;
  }

  /** Every way the place can be taken walked in as `entry`, fastest first. */
  cost(place: string, entry: Entry, airship: boolean): Costing[] {
    const id = `${place}|${entryKey(entry)}|${airship ? 1 : 0}`;
    let found = this.#cache.get(id);

    if (found === undefined) {
      found = this.#work(place, entry, airship).sort((a, b) => a.ms - b.ms);
      this.#cache.set(id, found);
    }
    return found;
  }

  readonly #byThing: (Costing[] | undefined)[] = [];

  /**
   * The same, looked up by a thing's index rather than by name: the search
   * asks this for every option at every state.
   */
  costOf(thing: Thing, entry: Entry, airship: boolean): Costing[] {
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
  get costed(): IterableIterator<[string, Costing[]]> {
    return this.#cache.entries();
  }

  #work(place: string, entry: Entry, airship: boolean): Costing[] {
    const { settings } = this;
    const found = lookup(this.stats, place, entry);
    const expected = settings.objective === "expected";
    const isSmall = entryKey(entry) === entryKey(SMALL);

    const source: Source = found === null ? "assumed" : found.exact ? "data" : "borrowed";
    const from = found === null || found.exact ? null : found.key;

    // The ways to take it, each with its clear time. Borrowed from another
    // power, the way it came out there says nothing about this one, so it is
    // taken to leave the player as they came.
    let aims: { exit: Power; gains: readonly Item[]; ms: number; clears: number }[];
    let deathMs: number;
    let death: number;

    const pick = (v: { best: number; median: number; mean: number }) =>
      settings.objective === "best" ? v.best : settings.objective === "median" ? v.median : v.mean;

    if (found === null) {
      if (settings.unknown === "avoid") {
        return [{ ms: Infinity, exits: [], exit: entry.power, gains: [], source, from, clears: 0, death: 0 }];
      }
      aims = [{ exit: entry.power, gains: [], ms: settings.unknownMs, clears: 0 }];
      deathMs = settings.unknownMs / 2;
      death = 0.1;
    } else {
      const s = found.summary;
      aims = found.samePower
        ? s.variants.map((v) => ({ exit: v.exit, gains: v.gains, ms: pick(v), clears: v.clears }))
        : [{ exit: entry.power, gains: [], ms: pick({ best: s.best!, median: s.median!, mean: s.mean! }), clears: s.clears }];
      deathMs = s.deathMs ?? aims[0]!.ms / 2;
      // A little prior weight toward "rarely dies", so one death in one go is
      // not read as a stage that kills every time, nor none in two as never.
      death = (s.deaths + 0.25) / (s.clears + s.deaths + 2);
    }

    return aims.map((aim) => {
      const exit = POWERS.indexOf(aim.exit);
      const gains = slotsOf(aim.gains);
      const base = { exit: aim.exit, gains: aim.gains, source, from, clears: aim.clears };

      if (!expected) return { ...base, ms: aim.ms, exits: [[exit, 1, gains]], death: 0 };

      const lost = deathMs + settings.respawnMs + (airship ? settings.chaseMs : 0);

      if (isSmall) {
        // Every retry is as small again, so the tries are alike: the expected
        // number of deaths before a clear is death / (1 - death).
        return { ...base, ms: aim.ms + (death / (1 - death)) * lost, exits: [[exit, 1, gains]], death };
      }

      // A death sends the player back in small, for the same again if small
      // has been seen to get it, and for small's fastest if not.
      const retries = this.cost(place, SMALL, airship);
      const retry =
        retries.find((r) => r.exit === aim.exit && r.gains.join() === aim.gains.join()) ?? retries[0]!;
      return {
        ...base,
        ms: (1 - death) * aim.ms + death * (lost + retry.ms),
        exits: [
          [exit, 1 - death, gains] as const,
          ...retry.exits.map(([p, q, g]) => [p, q * death, g] as const),
        ],
        death,
      };
    });
  }
}

// ---------------------------------------------------------------------------
// The search

/** Something to do next, and what it leads to. */
export interface Action {
  readonly kind: "stage" | "cloud" | "house" | "bros" | "rock" | "warp";
  /** A warp: the world it goes to. */
  readonly warp: number | null;
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

export function search(
  stats: Stats,
  settings: Settings,
  budget = 4_000_000,
): Search {
  things = 0;
  const rules = rulesFor(settings.category);
  const worlds = MAPS.map((m) => prepare(m, settings));

  /** Where the run is over. */
  const END: State = { world: MAPS.length, done: 0, pos: 0, power: 0, inv: 0 };

  /** Everything the rules want beaten in a world before its end, as a mask. */
  const required = worlds.map((w) =>
    rules.everything
      ? w.stage.reduce(
          (mask, st, node) =>
            st && w.map.nodes[node]!.kind !== "castle" && w.map.nodes[node]!.kind !== "bowser"
              ? mask | st.bit
              : mask,
          0,
        ) | w.bros.reduce((mask, b) => mask | b.bit, 0)
      : 0,
  );

  const POWER_UPS = [S.mushroom, S["fire-flower"], S.leaf];

  /**
   * The item lists a clear leaves, with how likely each is, given what it was
   * seen to hand over: the items themselves, less what the maps already give
   * on their own - a castle's reward, which comes on the way out of the world
   * (see nextWorld()), and a Bros.' treasure, which its fight gives. A power-up
   * out of a stage with a random chest in it is that chest, which is luck: at
   * its odds, or nothing where only the certain counts.
   */
  function handed(world: World, thing: Thing, gains: readonly number[], inv: number): [number, number][] {
    if (thing.kind === "bros") return [[1, inv]];

    const node = world.map.nodes[thing.node]!;
    const reward = node.kind === "castle" ? treasure(world.map.reward).map(([slot]) => slot) : [];
    const chest = node.chests?.includes("random") ?? false;
    let out: [number, number][] = [[1, inv]];

    for (const slot of gains) {
      if (reward.includes(slot)) continue;
      if (chest && POWER_UPS.includes(slot)) {
        if (settings.luck === "sure") continue;
        out = out.flatMap(([p, i]) => RANDOM_CHEST.map(([got, q]) => [p * q, give(i, got)] as [number, number]));
        continue;
      }
      out = out.map(([p, i]) => [p, give(i, slot)]);
    }
    return out;
  }

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

      // A stage not yet beaten is walked onto and no further - the player's
      // own square included, which is one only when a cloud has just set
      // them down on it, and from there it is that stage or nothing.
      const st = world.stage[u];
      if (st && !(s.done & st.bit)) continue;

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

  /**
   * Whether walking into a place as `to` rather than `from` is worth anything:
   * a better time, or leaving as something else. Spending an item that does
   * neither only loses the item, so it is never offered - which with a
   * history that has never seen a stage entered with a star is most of the
   * choices there would otherwise be, and every one of them another item list
   * to work the rest of the run out for.
   */
  const helpsMemo = new Map<string, boolean>();

  function helps(thing: Thing, from: Entry, to: Entry): boolean {
    const id = `${thing.index}|${entryKey(from)}|${entryKey(to)}`;
    let found = helpsMemo.get(id);
    if (found === undefined) {
      // Better if it opens a way out the other does not have, or takes one
      // they share faster.
      const aim = (c: Costing) => `${c.exit}|${c.gains.join()}`;
      const before = new Map(coster.costOf(thing, from, false).map((c) => [aim(c), c.ms]));
      found = coster.costOf(thing, to, false).some((c) => c.ms < (before.get(aim(c)) ?? Infinity));
      helpsMemo.set(id, found);
    }
    return found;
  }

  /** What each power item makes of the player; see Handle_player_map(). */
  const SPENDS: readonly { slot: Slot; to: (p: Power) => Power | null; pwing: boolean }[] = [
    { slot: "mushroom", to: (p) => (p === "small" ? "super" : null), pwing: false },
    { slot: "fire-flower", to: (p) => (p === "fire" ? null : "fire"), pwing: false },
    { slot: "leaf", to: (p) => (p === "racoon" ? null : "racoon"), pwing: false },
    { slot: "p-wing", to: () => "racoon", pwing: true },
  ];

  /**
   * Whether an item could be worth spending anywhere from world `w` on. One
   * that could not is dropped from every state there (see tidy()).
   */
  const useful: boolean[][] = worlds.map((_, w) =>
    SLOTS.map((slot) => {
      const later = worlds.slice(w);
      const places = later.flatMap((world) => [...world.stage.filter((t): t is Thing => t !== null), ...world.bros]);
      const plain = (power: Power): Entry => ({ power, star: false, pwing: false });

      switch (slot) {
        case "star":
          return places.some((t) => POWERS.some((p) => helps(t, plain(p), { power: p, star: true, pwing: false })));
        case "cloud":
          // Not planned on where every stage has to be beaten. One crossed
          // early can still save a walk around it, to be come back for - a
          // few seconds, measured - but every cloud held is another item
          // list for every stage order in every world after, and the search
          // took twenty times as long to find them.
          return !rules.everything;
        case "hammer":
          return later.some((world) => world.rocks.length > 0);
        case "anchor":
          return settings.objective === "expected" && later.some((world) => world.map.airship);
        case "whistle":
          return rules.warps && w < rules.lastWorld;
        default: {
          const spend = SPENDS.find((x) => x.slot === slot)!;
          return places.some((t) =>
            POWERS.some((p) => {
              const to = spend.to(p);
              return to !== null && helps(t, plain(p), { power: to, star: false, pwing: spend.pwing });
            }),
          );
        }
      }
    }),
  );

  /** What can be walked into a place as, and what it spends. */
  function loadouts(
    s: State,
    thing: Thing,
    airship: boolean,
  ): { entry: Entry; use: Slot[]; inv: number; anchored: boolean }[] {
    const power = POWERS[s.power]!;
    const plain: Entry = { power, star: false, pwing: false };
    const base = [{ power, pwing: false, use: [] as Slot[], inv: s.inv }];

    if (settings.items) {
      for (const spend of SPENDS) {
        const to = spend.to(power);
        if (to === null || count(s.inv, S[spend.slot]) === 0) continue;
        if (!helps(thing, plain, { power: to, star: false, pwing: spend.pwing })) continue;
        base.push({ power: to, pwing: spend.pwing, use: [spend.slot], inv: take(s.inv, S[spend.slot]) });
      }
    }

    const out: { entry: Entry; use: Slot[]; inv: number; anchored: boolean }[] = [];
    for (const b of base) {
      const without: Entry = { power: b.power, star: false, pwing: b.pwing };
      const withStar =
        settings.items && count(b.inv, S.star) > 0 && helps(thing, without, { ...without, star: true })
          ? [false, true]
          : [false];
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

  // An item nothing further on can be spent to any purpose is dropped from
  // the state: a hammer after the last rock, a star where no stage left has
  // ever been played faster with one. Holding it changes nothing, and two
  // states that differ only by it are one.
  function tidy(world: number, inv: number): number {
    let out = inv;
    const keep = useful[world];
    if (keep === undefined) return out;
    for (let slot = 0; slot < SLOTS.length; slot++) {
      if (!keep[slot]) out -= count(out, slot) * WEIGHT[slot]!;
    }
    return out;
  }

  function after(s: State, done: number, pos: number, power: number, inv: number): State {
    return { world: s.world, done, pos, power, inv: tidy(s.world, inv) };
  }

  /**
   * The start of the next world, with the castle's reward in hand - or the
   * end, where this world was the last the rules ask for.
   */
  function nextWorld(world: World, power: number, inv: number): State {
    if (world.map.world >= rules.lastWorld) return END;
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
      const needed = required[s.world]!;
      const allowed = !final || (s.done & needed) === needed;

      for (const l of allowed ? loadouts(s, st, airship) : []) {
        for (const costing of coster.costOf(st, l.entry, airship && !l.anchored)) {
          const done = s.done | st.bit;
          const outcomes = costing.exits.flatMap(([power, p, gains]) =>
            handed(world, st, gains, l.inv).map(([q, inv]) => [
              p * q,
              final ? nextWorld(world, power, inv) : after(s, done, node.id, power, inv),
            ] as const),
          );

          out.push({
            kind: "stage",
            warp: null,
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
      }

      // A cloud carries the player over a stage without playing it, onto
      // whatever is beyond; the stage is still there to block the way back.
      // Beyond can be another stage not yet beaten - 8-6 is crossed onto
      // Bowser's castle - and the player is then standing at it, to play it
      // and nothing else, as if they had walked up to it.
      if (settings.items && !final && node.id !== s.pos && count(s.inv, S.cloud) > 0) {
        for (const link of world.links[node.id]!) {
          if (dist[link.to] !== Infinity) continue;
          if (link.door !== 0 && !(s.done & world.opens[link.door])) continue;
          if (link.rock !== 0 && !(s.done & link.rock)) continue;

          out.push({
            kind: "cloud",
            warp: null,
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
          warp: null,
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
    }

    // A Bros. is a detour, unless the rules want it beaten, when it is part
    // of the route like a stage.
    if (rules.everything || (settings.items && settings.detours)) {
      for (const bros of world.bros) {
        if (s.done & bros.bit || dist[bros.node] === Infinity || !open(world, s, bros.node)) continue;
        if (!rules.everything) {
          // Fought for its drop alone, so one that drops nothing that could
          // be of use from here on is only time lost. A random drop is
          // weighed one detour at a time, like a house (see choices()).
          if (!bros.treasure.some(([slot]) => useful[s.world]![slot])) continue;
          if (!detours && bros.treasure.length > 1) continue;
        }

        for (const l of loadouts(s, bros, false)) {
          for (const costing of coster.costOf(bros, l.entry, false)) {
            out.push({
              kind: "bros",
              warp: null,
              node: bros.node,
              to: bros.node,
              place: bros.place,
              use: l.use,
              entry: l.entry,
              walkMs: dist[bros.node]!,
              doMs: costing.ms + settings.overheadMs,
              costing,
              // Nothing, where the drop is left to chance and only the
              // certain counts: the fight is still won.
              outcomes: costing.exits.flatMap(([power, p]) =>
                (bros.treasure.length > 0 ? bros.treasure : [[-1, 1] as const]).map(([slot, q]) => [
                  p * q,
                  after(s, s.done | bros.bit, bros.node, power, slot === -1 ? l.inv : give(l.inv, slot)),
                ] as const),
              ),
            });
          }
        }
      }
    }

    // A rock is the one thing an item is spent on that a category can need:
    // 100% has to reach 3-Bonus, which is behind one. So the hammer is used
    // there even with spending turned off.
    if ((settings.items || rules.everything) && count(s.inv, S.hammer) > 0) {
      for (const rock of world.rocks) {
        if (s.done & rock.bit || dist[rock.node] === Infinity || !open(world, s, rock.node)) continue;
        out.push({
          kind: "rock",
          warp: null,
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

    // The whistle, from wherever the player stands: into the warp zone, and
    // down whichever of its pipes goes furthest usefully - which is for the
    // search to say, so every one ahead is an option.
    if (rules.warps && settings.items && count(s.inv, S.whistle) > 0) {
      for (const warp of world.map.warps) {
        if (warp.world <= s.world || warp.world > rules.lastWorld) continue;
        const next = s0(warp.world);
        out.push({
          kind: "warp",
          warp: warp.world,
          node: s.pos,
          to: next.pos,
          place: null,
          use: ["whistle"],
          entry: null,
          walkMs: warp.tiles * settings.msPerTile,
          doMs: settings.warpMs,
          costing: null,
          outcomes: [[1, { ...next, power: s.power, inv: tidy(warp.world, take(s.inv, S.whistle)) }]],
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
   * own, and then the best that can be done after it.
   *
   * The detours that hand out a chance - a mushroom house, a Bros. with a
   * random drop - come in here and only here. Searched exactly, they make the
   * search too big to finish, since every chest's outcome is another item
   * list to work every later world out for; so each is weighed one at a time
   * against the best route that takes none after it. That is never worse than
   * taking none, and finds every one that pays for itself on its own; what it
   * can miss is two that only pay together. A Bros. with a set drop is part of
   * the search proper, chains and all - a hammer from one to break the rock
   * in front of another. Where only the certain counts, chance detours hand
   * out nothing and there are none to weigh, so the search is exact.
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
