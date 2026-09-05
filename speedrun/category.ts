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
// nothing downstream has to ask again. Three of them have something to check: a
// warpless route is one the game never reported a warp during, a World 1 route
// has to stay in world 1 and stop at its castle, and a 100% route has to have
// been through every stage and every overworld monster there is.
//
// That last one is the one rule that is not about the run alone: what there is
// to have been through is a fact about the level set, so it is read from
// stages.ts, which is generated from levels/ and checked against it on every
// build. A stage added to a map is then a stage this category asks for, rather
// than one it quietly stops asking for.

import { BOWSER_LEVEL, CASTLE_LEVEL } from "./events.ts";
import { type RouteSplit, type Trigger } from "./route.ts";
import { STAGES } from "./stages.ts";

/**
 * Every category there is.
 *
 * Written out rather than derived from the table below so that the table has
 * something to be checked against: a category added here without a definition,
 * or defined here under the wrong id, is a type error rather than a category
 * that quietly does not exist.
 */
export type CategoryId = "any" | "any-warpless" | "100" | "world-1";

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

// ---------------------------------------------------------------------------
// 100%: every stage, and every overworld monster
// ---------------------------------------------------------------------------

/** One thing the rules ask for, in a form two lists of them can be compared by. */
function stage(world: number, level: number): string {
  return `w${world}l${level}`;
}

function fight(world: number, monster: number): string {
  return `w${world}m${monster}`;
}

/**
 * Bowser's castle, which is asked for like any other stage and closed unlike
 * any other.
 *
 * Nothing ever reports it completed: the game does not come back to the map
 * from it, so it arrives as `run-ended` (see events.ts). The last world is the
 * one it is in, and the worlds are in order, so that is the last of them.
 */
const BOWSER_CASTLE = stage(STAGES.length - 1, BOWSER_LEVEL);

/** Everything a 100% run has to have completed, stages and monsters together. */
const REQUIRED: readonly string[] = STAGES.flatMap((world) => [
  ...world.levels.map((level) => stage(world.world, level)),
  ...world.monsters.map((monster) => fight(world.world, monster)),
]);

/**
 * What closing this split proves was completed, or null where it proves
 * nothing.
 *
 * "Proves" is why the world and the level both have to be pinned: a trigger
 * that leaves a field off closes on any value of it (see route.ts), so
 * { kind: "level-completed", world: 0 } is a split that closes on one of world
 * 1's stages without saying which. That is a fine split to run on and no
 * evidence at all that a particular stage was played, and this category is
 * evidence.
 */
function completes(on: Trigger): string | null {
  if (on.kind === "run-ended") return BOWSER_CASTLE;

  if (on.world === undefined) return null;

  if (on.kind === "level-completed" && on.level !== undefined) {
    return stage(on.world, on.level);
  }

  if (on.kind === "monster-defeated" && on.monster !== undefined) {
    return fight(on.world, on.monster);
  }

  return null;
}

/** Whether these splits close every stage and every fight the rules name. */
function coversEverything(splits: readonly RouteSplit[]): boolean {
  const done = new Set<string>();

  for (const split of splits) {
    const one = completes(split.on);

    if (one !== null) done.add(one);
  }

  return REQUIRED.every((one) => done.has(one));
}

/**
 * Modelled on Super Mario Bros. 3's 100%, which is the rule set this one is
 * meant to be read against: every action stage, and every overworld Hammer
 * Bros. including the Boomerang, Fire and Sledge variations.
 *
 * The two halves of that land on the two things this game has. An action stage
 * is a level tile on a world map - numbered stages, fortresses, the castle or
 * airship the map walks onto at the end of a world - and the plants and hands
 * that stand in the way in SMB3's later worlds are, here, map objects fought
 * where they stand, which is what the game's own code calls a monster and what
 * the Hammer Bros. are too. So the list is stages and monsters, and stages.ts
 * has both straight out of the maps.
 *
 * What SMB3 leaves out is left out here as well: mushroom houses, the card and
 * roulette games and the overworld pipes are allowed and not required, and
 * nothing asks about them because none of them reports (see src/speedrun.h).
 */
const HUNDRED = {
  id: "100",
  name: "100%",
  rules:
    "Beat the game, entering and completing every stage and every overworld " +
    "Hammer Bros. Mushroom houses, card games and pipes are allowed but not " +
    "required.",
  // Which is the whole of the rule, and it is a rule warps cannot be run
  // around: a warp skips worlds, and every one of their stages is on this
  // list. So there is nothing to say about warping here that the stages do
  // not already say.
  admits: coversEverything,
  // Over when the game is, like Any%. The last thing the rules ask for is
  // Bowser's castle, and beating him is what ends a run from the game's side -
  // so there is no end here that the splits could reach first.
  complete: () => false,
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
  "100": HUNDRED,
  "world-1": WORLD_1,
};

/** Every category, in the order they are offered. */
export const CATEGORIES: readonly Category[] = [
  ANY,
  ANY_WARPLESS,
  HUNDRED,
  WORLD_1,
];

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
