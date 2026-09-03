// The categories a run can be run under: the rules, rather than one way of
// obeying them.
//
// A category is what makes two times comparable. A route says which levels this
// run goes through and in what order; the category says which routes are the
// same race. So routes are held under categories - several routes to one
// category, each a way of running those rules - and a personal best is only
// ever a best within one of them.
//
// Categories are hardcoded, unlike routes. A route is a claim about how the
// game is played and the player is the only honest source for it, but a
// category is a claim about what counts, and one copy of the game inventing its
// own would produce times that look comparable to everyone else's and are not.
// So this table is the whole set, and anything naming a category outside it -
// a hand-edited file, or one exported by a build that has categories this one
// does not - does not load.
//
// Hardcoded is also what makes them checkable. Each carries `admits`, which
// reads a route's splits and says whether that route obeys the rules; a Route
// value only exists once its category has admitted it (see route.ts), so
// nothing downstream has to ask again. Two of them have something to check: a
// warpless route is one the game never reported a warp during, and a World 1
// route has to stay in world 1 and stop at its castle.

import { CASTLE_LEVEL } from "./events.ts";
import { type RouteSplit } from "./route.ts";

/**
 * Every category there is.
 *
 * Written out rather than derived from the table below so that the table has
 * something to be checked against: a category added here without a definition,
 * or defined here under the wrong id, is a type error rather than a category
 * that quietly does not exist.
 */
export type CategoryId = "any" | "any-warpless" | "world-1";

export interface Category {
  readonly id: CategoryId;
  readonly name: string;
  /** What the player is agreeing to, in a line the routes section shows. */
  readonly rules: string;
  /**
   * Whether a route obeys those rules, as far as its splits can show.
   *
   * "As far as its splits can show" is the whole of the contract: this is not a
   * referee. It rejects routes that provably break the rules, and admits the
   * rest.
   */
  readonly admits: (splits: readonly RouteSplit[]) => boolean;
  /**
   * Whether these splits are a whole run of the category rather than the start
   * of one, which is what stops a recording.
   *
   * False where the end is not something splits can show: Any% is over when the
   * game is, and how many worlds that takes belongs to the level set rather
   * than to anything here, so the game is what says when. A category that does
   * know its own end says so, and a recording of it stops the moment it is
   * reached instead of running on into levels the category does not cover.
   */
  readonly complete: (splits: readonly RouteSplit[]) => boolean;
}

/** Whether any of these splits is one the game reported a warp for. */
function warps(splits: readonly RouteSplit[]): boolean {
  return splits.some((split) => split.on.kind === "warp-taken");
}

/**
 * The worlds a route passes through, in the order its splits name them.
 *
 * Splits that name no world are left out rather than counted as a gap: a
 * trigger with no world on it is one that does not care which world it is in,
 * and a route made of those says nothing either way about warping.
 */
function worlds(splits: readonly RouteSplit[]): number[] {
  const seen: number[] = [];

  for (const split of splits) {
    const world = split.on.world;

    if (world !== undefined && world !== seen[seen.length - 1]) {
      seen.push(world);
    }
  }

  return seen;
}

// `satisfies` rather than a type annotation: each of these has to be a
// Category, and has to keep its own id as the literal it was written as, which
// is what lets the table below check the two against each other.
const ANY = {
  id: "any",
  name: "Any%",
  rules: "Beat the game however you like. Warps allowed.",
  // Nothing to check. Any% is a rule about where the run ends, not about how it
  // got there, and every route is a way of running it - which is what makes it
  // the category anything else falls back to.
  admits: () => true,
  // Only the game knows when the game is beaten.
  complete: () => false,
} satisfies Category;

const ANY_WARPLESS = {
  id: "any-warpless",
  name: "Any% warpless",
  rules: "Beat the game without taking a warp pipe or a whistle.",
  admits: (splits) => {
    // The game says when a warp was taken, which is the whole of this rule and
    // the only way to enforce it honestly: a warp into the very next world
    // moves the world number exactly as far as beating a castle does, so the
    // numbers alone cannot see it.
    if (warps(splits)) return false;

    const visited = worlds(splits);

    // A route that names no world at all has not been shown to be warpless, and
    // this is the category where that has to be shown.
    if (visited[0] !== 0) return false;

    // The numbers are still worth checking, for the warps that left no event
    // behind: a route recorded before the game reported them, or one edited by
    // hand. A world skipped over is a world nobody played.
    return visited.every(
      (world, i) => i === 0 || world === (visited[i - 1] ?? 0) + 1,
    );
  },
  complete: () => false,
} satisfies Category;

/** The run World 1 is: world 1's castle beaten, and that is the end of it. */
function endsAtWorld1Castle(splits: readonly RouteSplit[]): boolean {
  const last = splits[splits.length - 1]?.on;

  return (
    last?.kind === "level-completed" &&
    last.world === 0 &&
    last.level === CASTLE_LEVEL
  );
}

const WORLD_1 = {
  id: "world-1",
  name: "World 1",
  rules: "Beat world 1's castle. The run ends there, whatever is played after.",
  // The one category here with an end its splits can be held to: it is over
  // when world 1's castle is, so a route that stops anywhere else is not a way
  // of running it. Nothing may leave world 1 either - a route that carries on
  // into world 2 is timing more than these rules cover - and the warp out of
  // world 1 is a warp like any other.
  admits: (splits) =>
    !warps(splits) &&
    worlds(splits).every((world) => world === 0) &&
    endsAtWorld1Castle(splits),
  // Which is also what ends a recording of it: the castle falls and the route
  // is written, however much of the game is played afterwards.
  complete: endsAtWorld1Castle,
} satisfies Category;

/**
 * Lookup, keyed so that the key and the category's own id cannot disagree.
 *
 * The mapped type is the check: each entry has to be the category whose id is
 * the key it sits under, and every id has to have one.
 */
const BY_ID: Readonly<{ [K in CategoryId]: Category & { readonly id: K } }> = {
  any: ANY,
  "any-warpless": ANY_WARPLESS,
  "world-1": WORLD_1,
};

/** Every category, in the order they are offered. */
export const CATEGORIES: readonly Category[] = [ANY, ANY_WARPLESS, WORLD_1];

/**
 * The category anything unplaceable ends up in.
 *
 * Any% admits every route, so there is always somewhere for a recorded run to
 * go, however it was played.
 */
export const FALLBACK_CATEGORY: Category = ANY;

export function category(id: CategoryId): Category {
  return BY_ID[id];
}

export function isCategoryId(value: unknown): value is CategoryId {
  return typeof value === "string" && Object.hasOwn(BY_ID, value);
}

/**
 * The categories that will have a route as it stands.
 *
 * Always at least Any%, so a run recorded under rules it turned out to break -
 * a warpless attempt that took the whistle - has somewhere to be saved rather
 * than being thrown away at the end of the run that set it.
 */
export function categoriesFor(splits: readonly RouteSplit[]): Category[] {
  return CATEGORIES.filter((one) => one.admits(splits));
}

/**
 * Where a route belongs: the category asked for if it will have it, else the
 * first that will.
 */
export function placeIn(
  preferred: CategoryId,
  splits: readonly RouteSplit[],
): Category {
  const wanted = category(preferred);

  return wanted.admits(splits)
    ? wanted
    : (categoriesFor(splits)[0] ?? FALLBACK_CATEGORY);
}
