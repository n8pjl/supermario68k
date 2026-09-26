// What a place in the game is called.
//
// The events say where something happened by index - a level's place in its
// world file, a monster's in the map's object list - which is what a route is
// pinned to and nothing a player would recognise. The names are the ones the
// map shows, worked out from levels/ along with the rest of the stage manifest
// (see tools/mkstages.py), so they are the same everywhere they are shown.

import { WARP_ZONE_WORLD } from "./events.ts";
import { STAGES } from "./stages.ts";

const LEVELS = new Map<string, string>();
const MONSTERS = new Map<string, string>();

/** Where each named place sits in the manifest: world, then its own order. */
const ORDER = new Map<string, number>();

for (const { world, levels, monsters } of STAGES) {
  for (const { level, name } of levels) {
    LEVELS.set(`${world}.${level}`, name);
    ORDER.set(`L${world}.${level}`, ORDER.size);
  }
  for (const { monster, name } of monsters) {
    MONSTERS.set(`${world}.${monster}`, name);
    ORDER.set(`M${world}.${monster}`, ORDER.size);
  }
}

/**
 * A level as the map shows it: "1-3", "2-Pyramid", "8-Bowser".
 *
 * Every level a map tile enters has a name. One that does not - reached some
 * other way, or reported by a build whose level set this one does not have -
 * falls back to its index, so it is still told apart from the others.
 */
export function levelName(world: number, level: number): string {
  if (world === WARP_ZONE_WORLD) return "Warp zone";
  return LEVELS.get(`${world}.${level}`) ?? `${world + 1}-#${level}`;
}

/** An overworld monster: "3-Hammer Bros. 2". The same fallback as above. */
export function monsterName(world: number, monster: number): string {
  return MONSTERS.get(`${world}.${monster}`) ?? `${world + 1}-Bros. #${monster}`;
}

/**
 * Where a place comes in the manifest - a world's numbered levels, then the
 * rest of its stages, then its monsters - for listing places the way a world
 * reads. Null for one the manifest does not have.
 */
export function placeOrder(
  world: number,
  where: { level: number } | { monster: number },
): number | null {
  const key =
    "level" in where ? `L${world}.${where.level}` : `M${world}.${where.monster}`;

  return ORDER.get(key) ?? null;
}
