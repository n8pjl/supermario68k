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

export interface LevelEntered {
  readonly kind: "level-entered";
  readonly world: number;
  /** The level's index within its world file, counted from zero. */
  readonly level: number;
}

export interface LevelCompleted {
  readonly kind: "level-completed";
  readonly world: number;
  readonly level: number;
}

export type GameEvent =
  | RunStarted
  | RunAbandoned
  | RunEnded
  | WorldEntered
  | WarpTaken
  | LevelEntered
  | LevelCompleted;

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
