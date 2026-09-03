// Splits gathered into the world each one is played in.
//
// A route is a flat list of splits; a longer run is read a world at a time. A
// group is a run of consecutive splits that share a world number - the levels
// of world 2, then its castle - timed as one long segment that contains the
// inner ones. Nothing new is recorded to make them: the world is already on
// every split's trigger (see route.ts), so the grouping is derived from the
// route each time it is drawn.
//
// The nested view is only worth showing when there is more than one world to
// separate: a World 1 route, or any route that never leaves a world, is read
// as the flat list it already was. `entersMultipleWorlds` is that test, and the
// panel and the names editor both ask it before drawing headers.

import { WARP_ZONE_WORLD } from "./events.ts";
import { type RouteSplit } from "./route.ts";

/**
 * Whether a split is one a world group is built from.
 *
 * Warp markers are not - they carry no time of their own (see route.ts) - and
 * neither is the warp zone: it is a world number only as a quirk of the level
 * set's layout, a room the whistle drops the player into to choose a pipe, not
 * a world the run is timed through. Callers that pass timed splits have already
 * dropped the warps; the check is kept here so the grouping is right whatever
 * it is handed.
 */
function grouped(split: RouteSplit): boolean {
  return split.on.kind !== "warp-taken" && split.on.world !== WARP_ZONE_WORLD;
}

export interface SplitGroup {
  /**
   * Stable across edits to other parts of the route: what a saved world-segment
   * time is matched back up by. Keyed by the world and how many times that world
   * has opened a group before this one, so a route that leaves a world and comes
   * back gives the two visits ids that do not collide.
   */
  readonly id: string;
  readonly name: string;
  /** The world these splits are in, or null for a leading split that names none. */
  readonly world: number | null;
  /** Index of the group's first split in the route, inclusive. */
  readonly from: number;
  /** Index of its last split, inclusive. */
  readonly to: number;
}

/**
 * The route cut into world groups.
 *
 * A split that names no world - a hand-edited trigger with the field left off -
 * joins the group before it rather than breaking it: it does not say it is
 * somewhere else, so it is not treated as somewhere else. One at the very front,
 * with no group to join, opens a group of its own with a null world.
 */
export function groupSplits(splits: readonly RouteSplit[]): SplitGroup[] {
  const groups: SplitGroup[] = [];
  const opens = new Map<number, number>();
  let current: { -readonly [K in keyof SplitGroup]: SplitGroup[K] } | null = null;

  splits.forEach((split, i) => {
    if (!grouped(split)) return;

    const world = split.on.world ?? null;

    if (current !== null && (world === null || world === current.world)) {
      current.to = i;
      return;
    }

    const nth = world === null ? 0 : opens.get(world) ?? 0;
    if (world !== null) opens.set(world, nth + 1);

    current = {
      id: world === null ? `seg${i}` : `w${world}#${nth}`,
      name: world === null ? "Segment" : `World ${world + 1}`,
      world,
      from: i,
      to: i,
    };
    groups.push(current);
  });

  return groups;
}

/** How many distinct worlds the route is played across, null worlds aside. */
export function worldCount(splits: readonly RouteSplit[]): number {
  const worlds = new Set<number>();

  for (const split of splits) {
    if (grouped(split) && split.on.world !== undefined) {
      worlds.add(split.on.world);
    }
  }

  return worlds.size;
}

/** Whether the route is long enough across worlds to be worth nesting. */
export function entersMultipleWorlds(splits: readonly RouteSplit[]): boolean {
  return worldCount(splits) >= 2;
}
