// The run history, read the way a route is planned: each stage, by what the
// player walked into it as.
//
// The data page adds visits up by stage; a route needs one step more, because
// what a stage costs depends on what it is entered as - a fortress run small is
// a different stage from the same fortress run with a leaf - and what it leaves
// the player as is what the next one is entered as. So visits are grouped here
// by place and by entry state (power, and a star or P-wing carried in from the
// map), and each clear is given the power it was left with.
//
// The game reports what a stage was beaten as and holding. History from before
// it did has only what the player walked into next, which is read where
// nothing in between can have changed it - no power item spent from the list,
// and the next thing a different place, so it is not practice mode putting the
// player back at the start of the same stage with whatever the panel was asked
// for. Where neither can be had, it is estimated from the hits: the last one
// says what the player was just before it, which catches a leaf picked up
// before being hit, and misses one picked up after (`exitSeen` is false).
//
// Clears are then grouped by how they came out - what the player left as, and
// what they came away with - into variants. A variant the history has seen is
// one the player can go for: 1-1 left as raccoon, 2-Pyramid left with its
// cloud. The search chooses between them, each at the time it took.
//
// Practice counts the same as a timed run here, and every figure carries how
// much of it came from practice, so that it can be told apart where it
// matters.

import { type Item, type Loadout, type Power } from "../speedrun/events.ts";
import { type Attempt } from "../analysis/attempts.ts";
import { type Visit, median } from "../analysis/visits.ts";

export const POWERS: readonly Power[] = ["small", "super", "fire", "racoon"];

/** What a stage is walked into as: the part of a loadout a stage cares about. */
export interface Entry {
  readonly power: Power;
  readonly star: boolean;
  readonly pwing: boolean;
}

export function entryOf(player: Loadout): Entry {
  return { power: player.power, star: player.star, pwing: player.pwing };
}

/** "fire", "racoon+pwing", "small+star": what entries are grouped by. */
export function entryKey(entry: Entry): string {
  return entry.power + (entry.star ? "+star" : "") + (entry.pwing ? "+pwing" : "");
}

export function parseEntryKey(key: string): Entry {
  const [power, ...rest] = key.split("+");
  return {
    power: power as Power,
    star: rest.includes("star"),
    pwing: rest.includes("pwing"),
  };
}

export function entryLabel(entry: Entry): string {
  return [
    entry.power === "racoon" ? "raccoon" : entry.power,
    entry.star && "star",
    entry.pwing && "P-wing",
  ]
    .filter(Boolean)
    .join(" + ");
}

/** A hit takes a suit down to super, and super down to small. */
export function afterHits(power: Power, hits: number): Power {
  if (hits === 0) return power;

  // Both suits are one hit from super: fire and raccoon share a rank.
  const rank = Math.min(POWERS.indexOf(power), 2);
  return POWERS[Math.max(0, rank - hits)]!;
}

/** The items that change what the player is, spent from the list. */
const POWER_ITEMS: readonly Item[] = ["mushroom", "fire-flower", "leaf", "p-wing"];

/** What is in `before` and not in `after`, as a multiset. */
export function spent(before: readonly Item[], after: readonly Item[]): Item[] {
  const left = [...after];
  const out: Item[] = [];

  for (const item of before) {
    const i = left.indexOf(item);
    if (i === -1) out.push(item);
    else left.splice(i, 1);
  }
  return out;
}

export interface Clear {
  readonly ms: number;
  readonly exit: Power;
  /** Reported, or read off what came next, rather than estimated from the hits. */
  readonly exitSeen: boolean;
  /**
   * Items it was left holding that it was not entered with. All of them where
   * the game reported it, and any a house on the way out added; where it was
   * read off the next thing entered, only those the map could not have handed
   * over in between (see MAP_ITEMS).
   */
  readonly gained: readonly Item[];
  readonly hits: number;
  readonly practice: boolean;
  /** Epoch milliseconds the attempt started. */
  readonly when: number;
}

export interface Death {
  readonly practice: boolean;
  readonly when: number;
}

/** Every visit to one place that was walked into as one thing. */
export interface Cell {
  readonly place: string;
  readonly entry: Entry;
  /** Fastest first. */
  readonly clears: Clear[];
  readonly deaths: Death[];
  /** Visits that ended any other way: a reset, the page closing. */
  left: number;
}

/**
 * A cell boiled down to what the search reads, and nothing it cannot copy to
 * a worker.
 */
/** One way a place has been seen to come out, and the clears that did. */
export interface Variant {
  readonly exit: Power;
  /** Sorted, so that two variants alike are keyed alike. */
  readonly gains: readonly Item[];
  readonly clears: number;
  readonly best: number;
  readonly median: number;
  readonly practice: number;
}

export function variantKey(v: { exit: Power; gains: readonly Item[] }): string {
  return [v.exit, ...v.gains].join("+");
}

export interface Summary {
  readonly clears: number;
  readonly deaths: number;
  readonly best: number | null;
  readonly median: number | null;
  readonly mean: number | null;
  /** How the clears ended, by power, counted. */
  readonly exits: Readonly<Partial<Record<Power, number>>>;
  readonly exitsSeen: number;
  /** Clears and deaths that came from practice. */
  readonly practice: number;
  /** The middle half of the clear times, as a share of the median. */
  readonly spread: number | null;
  /** Epoch milliseconds of the newest sample. */
  readonly newest: number | null;
  /** The ways it has come out, fastest first. */
  readonly variants: readonly Variant[];
}

/** Summaries by place key, then by entry key. */
export type Stats = Record<string, Record<string, Summary>>;

function newest(cell: Cell): number | null {
  const all = [...cell.clears, ...cell.deaths].map((s) => s.when);
  return all.length === 0 ? null : Math.max(...all);
}

export function summarise(cell: Cell): Summary {
  const times = cell.clears.map((c) => c.ms);
  const exits: Partial<Record<Power, number>> = {};

  for (const c of cell.clears) exits[c.exit] = (exits[c.exit] ?? 0) + 1;

  const quartile = (q: number) => times[Math.min(times.length - 1, Math.floor(q * times.length))]!;
  const mid = median(times);

  return {
    clears: times.length,
    deaths: cell.deaths.length,
    best: times[0] ?? null,
    median: mid,
    mean: times.length === 0 ? null : times.reduce((a, b) => a + b, 0) / times.length,
    exits,
    exitsSeen: cell.clears.filter((c) => c.exitSeen).length,
    practice:
      cell.clears.filter((c) => c.practice).length +
      cell.deaths.filter((d) => d.practice).length,
    spread:
      times.length < 4 || mid === null || mid === 0
        ? null
        : (quartile(0.75) - quartile(0.25)) / mid,
    newest: newest(cell),
    variants: variantsOf(cell.clears),
  };
}

function variantsOf(clears: readonly Clear[]): Variant[] {
  const groups = new Map<string, Clear[]>();
  for (const c of clears) {
    const key = variantKey({ exit: c.exit, gains: [...c.gained].sort() });
    groups.set(key, [...(groups.get(key) ?? []), c]);
  }

  return [...groups.values()]
    .map((list) => {
      const times = list.map((c) => c.ms).sort((a, b) => a - b);
      return {
        exit: list[0]!.exit,
        gains: [...list[0]!.gained].sort(),
        clears: times.length,
        best: times[0]!,
        median: median(times)!,
        practice: list.filter((c) => c.practice).length,
      };
    })
    .sort((a, b) => a.best - b.best);
}

/** An item spent from the list on the map, and what it was spent before. */
export interface ItemUse {
  readonly item: Item;
  readonly place: string;
  readonly practice: boolean;
}

/**
 * Items that can come from the map between one stage and the next: a mushroom
 * house, a card game. Read off the next thing entered, these could be the
 * map's rather than the stage's, so only the rest are credited to a stage
 * there. Reported by the game at the end of the stage, everything is.
 */
const MAP_ITEMS: readonly Item[] = ["mushroom", "fire-flower", "leaf", "star"];

export interface Model {
  readonly cells: ReadonlyMap<string, ReadonlyMap<string, Cell>>;
  readonly stats: Stats;
  readonly uses: readonly ItemUse[];
}

/**
 * The model of every visit in these attempts. `visitsOf` is the data page's
 * cutting of an attempt into visits, which is cached there and here alike.
 */
export function buildModel(
  attempts: readonly Attempt[],
  visitsOf: (attempt: Attempt) => readonly Visit[],
): Model {
  const cells = new Map<string, Map<string, Cell>>();
  const uses: ItemUse[] = [];

  const cell = (place: string, entry: Entry): Cell => {
    let byEntry = cells.get(place);
    if (byEntry === undefined) cells.set(place, (byEntry = new Map()));

    const key = entryKey(entry);
    let found = byEntry.get(key);
    if (found === undefined) {
      found = { place, entry, clears: [], deaths: [], left: 0 };
      byEntry.set(key, found);
    }
    return found;
  };

  for (const attempt of attempts) {
    const visits = visitsOf(attempt);
    const practice = attempt.mode === "practice";
    const when = attempt.started.epochMilliseconds;

    visits.forEach((visit, i) => {
      const here = cell(visit.place.key, entryOf(visit.player));
      const next = visits[i + 1];
      const used = next === undefined ? [] : spent(visit.player.items, next.player.items);
      // Whether the next visit follows on from this one in play, rather than
      // from the practice panel starting something over.
      const follows =
        next !== undefined &&
        next.place.key !== visit.place.key &&
        next.place.world - visit.place.world <= 1 &&
        next.place.world >= visit.place.world;

      if (follows) {
        for (const item of used) uses.push({ item, place: next.place.key, practice });
      }

      switch (visit.outcome) {
        case "cleared": {
          const seen = follows && !used.some((item) => POWER_ITEMS.includes(item));
          let exit: Power;
          let gained: Item[];

          if (visit.exit !== null) {
            exit = visit.exit.power;
            gained = spent(visit.exit.items, visit.player.items);
            // A house some levels open on the way out - 1-3's white-block
            // whistle, a hidden house for the right coins - is entered after
            // the level has been reported beaten, so what it gives turns up
            // only in what is walked into next.
            if (follows) {
              gained.push(
                ...spent(next!.player.items, visit.exit.items).filter((i) => !MAP_ITEMS.includes(i)),
              );
            }
          } else if (seen) {
            exit = next!.player.power;
            gained = spent(next!.player.items, visit.player.items).filter((i) => !MAP_ITEMS.includes(i));
          } else {
            const last = visit.hitAs.at(-1);
            exit = last === undefined ? visit.player.power : afterHits(last, 1);
            gained = [];
          }

          here.clears.push({
            ms: visit.end - visit.start,
            exit,
            exitSeen: visit.exit !== null || seen,
            gained: [...new Set(gained)],
            hits: visit.hits,
            practice,
            when,
          });
          break;
        }
        case "died":
          here.deaths.push({ practice, when });
          break;
        default:
          here.left++;
      }
    });
  }

  const stats: Stats = {};
  for (const [place, byEntry] of cells) {
    const out: Record<string, Summary> = {};

    for (const [key, one] of byEntry) {
      one.clears.sort((a, b) => a.ms - b.ms);
      out[key] = summarise(one);
    }
    stats[place] = out;
  }

  return { cells, stats, uses };
}
