// The events the game reports, and the whole of what it reports.
//
// The other half of src/speedrun.h: each struct there arrives as an object
// tagged with its `kind`, so the two sides only have to agree on these names.
// A route's triggers are written against them (see route.ts), which is why the
// field names here are part of the saved file format and not just of this code.

export interface RunStarted {
  readonly kind: "run-started";
}

export interface RunAbandoned {
  readonly kind: "run-abandoned";
}

export interface RunEnded {
  readonly kind: "run-ended";
}

export interface WorldEntered {
  readonly kind: "world-entered";
  /** Levelsetdata.CurrentWorld, counted from zero. */
  readonly world: number;
}

/**
 * A warp was taken: a pipe that jumps to another world, or the whistle that
 * goes to the warp zone. `world` is the one it lands in.
 *
 * Reported because a warp cannot be told from the world numbers either side of
 * it - a warp into the very next world looks exactly like having finished the
 * one before - so a category that forbids warping has to be told rather than
 * left to work it out.
 */
export interface WarpTaken {
  readonly kind: "warp-taken";
  readonly world: number;
}

/** What the player can be, as src/speedrun.cpp names it. */
export type Power = "small" | "super" | "fire" | "racoon";

/**
 * An item list entry, as src/speedrun.cpp names it. The last is a number the
 * game has no item for, kept rather than dropped so the list stays whole.
 */
export type Item =
  | "mushroom"
  | "fire-flower"
  | "leaf"
  | "star"
  | "whistle"
  | "hammer"
  | "p-wing"
  | "cloud"
  | "anchor"
  | `item-${number}`;

/**
 * What the player walked into a level or a fight with.
 *
 * Nothing in the timer reads it - a trigger pins world, level and monster and
 * nothing else, so a route splits the same whatever was carried in - but it is
 * kept in the run history, which is where a route is worked out from.
 */
export interface Loadout {
  readonly power: Power;
  /** A star used from the map, which lasts into the next level. */
  readonly star: boolean;
  /** A P-wing used from the map, the same. */
  readonly pwing: boolean;
  /** The item list in slot order, empty slots left out. */
  readonly items: readonly Item[];
}

export interface LevelEntered {
  readonly kind: "level-entered";
  readonly world: number;
  /** The level's index within its world file, counted from zero. */
  readonly level: number;
  readonly player: Loadout;
}

export interface LevelCompleted {
  readonly kind: "level-completed";
  readonly world: number;
  readonly level: number;
  /**
   * What the player beat it as and holding, which is what they walk out with.
   * Missing from history written before the game reported it.
   */
  readonly player?: Loadout | undefined;
}

/**
 * An overworld monster was walked into: a Hammer Bros. or one of the Boomerang,
 * Fire and Sledge Bros. that share its handler.
 *
 * `monster` is the object's index in the world's map file, not a level. The
 * arena it drops into is a level of the common file, shared by every monster of
 * that kind in the game, so the level says which sort of Bros. this is and
 * nothing about which of them - and 100% is a rule about which of them.
 */
export interface MonsterFought {
  readonly kind: "monster-fought";
  readonly world: number;
  readonly monster: number;
  readonly player: Loadout;
}

/** That monster was beaten, rather than run from or died to. */
export interface MonsterDefeated {
  readonly kind: "monster-defeated";
  readonly world: number;
  readonly monster: number;
  /** As on LevelCompleted: its treasure is already in the list. */
  readonly player?: Loadout | undefined;
}

/**
 * A life was lost in a level or a monster fight: exactly one of `level` and
 * `monster` says which, the other crossing as undefined. Raised the moment it
 * happens, before the death animation.
 *
 * Only ever inside something being played, so a death in the title screen's
 * demo or in a pipe passage off the map reports nothing - 7-Pipe aside, which
 * is a level. `player` is what the player died as - see src/speedrun.h.
 */
export interface PlayerDied {
  readonly kind: "player-died";
  readonly world: number;
  readonly level?: number | undefined;
  readonly monster?: number | undefined;
  readonly player: Loadout;
}

/**
 * The player was hurt and lived: a suit knocked off, or super down to small.
 * Said where the same way as a death, and silent in the same places.
 *
 * `player` is what they were just before the hit, which is what the hit took;
 * what is left follows from it. A hit that kills reports only as player-died.
 */
export interface PlayerHit {
  readonly kind: "player-hit";
  readonly world: number;
  readonly level?: number | undefined;
  readonly monster?: number | undefined;
  readonly player: Loadout;
}

export type GameEvent =
  | RunStarted
  | RunAbandoned
  | RunEnded
  | WorldEntered
  | WarpTaken
  | LevelEntered
  | LevelCompleted
  | MonsterFought
  | MonsterDefeated
  | PlayerDied
  | PlayerHit;

export type EventKind = GameEvent["kind"];

/** Every kind, for checking what an imported route says it triggers on. */
export const EVENT_KINDS: readonly EventKind[] = [
  "run-started",
  "run-abandoned",
  "run-ended",
  "world-entered",
  "warp-taken",
  "level-entered",
  "level-completed",
  "monster-fought",
  "monster-defeated",
  "player-died",
  "player-hit",
];

export function isEventKind(value: unknown): value is EventKind {
  return EVENT_KINDS.includes(value as EventKind);
}

/**
 * The level index every world file keeps its world-ending castle under.
 *
 * Not a level tile on the map - it is the big_castle walked onto, loaded by
 * Enter_enemy_ship() in map.c - so it is the one level number that means the
 * same thing in every world, and the one a recorded route can name.
 */
export const CASTLE_LEVEL = 7;

/**
 * The level index world 8 keeps Bowser's castle under.
 *
 * The one stage no split can name as completed: the game never returns to the
 * map from it - the ending takes over the frame Bowser falls - so it is
 * reported as `run-ended` and as nothing else. A category that asks for every
 * stage has to know that (see category.ts).
 */
export const BOWSER_LEVEL = 19;

/**
 * The world number the warp zone reports as.
 *
 * It is Levelsetdata.Commonfile - the shared level file, index 8, one past the
 * eight world files (see src/levelset.h) - because the warp zone is a map like
 * any other and that is the slot it lives in. The whistle raises
 * `world-entered` and `warp-taken` for it on the way through. It is not a world
 * the run is timed across, though: it is a room to pick a pipe in, so the
 * grouping leaves it out the same way it leaves out the warps themselves.
 */
export const WARP_ZONE_WORLD = 8;
