// The stages and the overworld monsters of every world, and what each is
// called: what a 100% run has to have been through, and the names a player
// knows them by.
//
// Generated from levels/ by tools/mkstages.py, and checked against it on every
// build - see the Makefile. Do not edit: an edit here is a claim about the
// level set that the level set does not make, and the category it feeds would
// then ask for a stage nobody can play or let a run past one it skipped. How
// the names are worked out is in that script.
//
// Worlds are numbered from zero, the way every event the game reports numbers
// them. A level is its index in the world file; a monster is its index in the
// map's object list, which is what tells two of them in one world apart - the
// arena they load is a level of the common file and is shared between them.

export interface Stage {
  /** The level index the events report. */
  readonly level: number;
  /** As the map shows it: "1-3", "2-Pyramid", "8-Bowser". */
  readonly name: string;
}

export interface Monster {
  /** The map object index the events report. */
  readonly monster: number;
  /** As a player would say it: "3-Hammer Bros. 2". */
  readonly name: string;
}

export interface WorldStages {
  readonly world: number;
  /**
   * Every stage of that world, its castle or airship included: the numbered
   * levels in order, then the rest, ending with whatever ends the world.
   */
  readonly levels: readonly Stage[];
  /** Every Hammer Bros. and variant standing on its map. */
  readonly monsters: readonly Monster[];
}

export const STAGES: readonly WorldStages[] = [
  {
    world: 0,
    levels: [
      { level: 0, name: "1-1" },
      { level: 1, name: "1-2" },
      { level: 2, name: "1-3" },
      { level: 3, name: "1-4" },
      { level: 4, name: "1-5" },
      { level: 5, name: "1-6" },
      { level: 6, name: "1-Fortress" },
      { level: 7, name: "1-Castle" },
    ],
    monsters: [
      { monster: 0, name: "1-Hammer Bros." },
    ],
  },
  {
    world: 1,
    levels: [
      { level: 0, name: "2-1" },
      { level: 1, name: "2-2" },
      { level: 2, name: "2-3" },
      { level: 3, name: "2-4" },
      { level: 4, name: "2-5" },
      { level: 6, name: "2-Fortress" },
      { level: 8, name: "2-Pyramid" },
      { level: 9, name: "2-Quicksand" },
      { level: 7, name: "2-Castle" },
    ],
    monsters: [
      { monster: 0, name: "2-Boomerang Bros. 1" },
      { monster: 1, name: "2-Boomerang Bros. 2" },
      { monster: 2, name: "2-Fire Bros." },
    ],
  },
  {
    world: 2,
    levels: [
      { level: 0, name: "3-1" },
      { level: 1, name: "3-2" },
      { level: 2, name: "3-3" },
      { level: 3, name: "3-4" },
      { level: 4, name: "3-5" },
      { level: 5, name: "3-6" },
      { level: 11, name: "3-7" },
      { level: 6, name: "3-Fortress 1" },
      { level: 10, name: "3-Fortress 2" },
      { level: 15, name: "3-Bonus" },
      { level: 7, name: "3-Castle" },
    ],
    monsters: [
      { monster: 0, name: "3-Hammer Bros. 1" },
      { monster: 2, name: "3-Hammer Bros. 2" },
    ],
  },
  {
    world: 3,
    levels: [
      { level: 0, name: "4-1" },
      { level: 1, name: "4-2" },
      { level: 2, name: "4-3" },
      { level: 3, name: "4-4" },
      { level: 7, name: "4-Castle" },
    ],
    monsters: [
      { monster: 0, name: "4-Hammer Bros. 1" },
      { monster: 1, name: "4-Hammer Bros. 2" },
    ],
  },
  {
    world: 4,
    levels: [
      { level: 0, name: "5-1" },
      { level: 1, name: "5-2" },
      { level: 2, name: "5-3" },
      { level: 3, name: "5-4" },
      { level: 4, name: "5-5" },
      { level: 6, name: "5-Fortress" },
      { level: 7, name: "5-Castle" },
    ],
    monsters: [
      { monster: 0, name: "5-Boomerang Bros." },
      { monster: 1, name: "5-Hammer Bros." },
    ],
  },
  {
    world: 5,
    levels: [
      { level: 0, name: "6-1" },
      { level: 1, name: "6-2" },
      { level: 2, name: "6-3" },
      { level: 3, name: "6-4" },
      { level: 4, name: "6-5" },
      { level: 5, name: "6-6" },
      { level: 11, name: "6-7" },
      { level: 6, name: "6-Fortress 1" },
      { level: 10, name: "6-Fortress 2" },
      { level: 15, name: "6-Bonus" },
      { level: 7, name: "6-Castle" },
    ],
    monsters: [
      { monster: 0, name: "6-Hammer Bros. 1" },
      { monster: 2, name: "6-Hammer Bros. 2" },
      { monster: 3, name: "6-Boomerang Bros." },
    ],
  },
  {
    world: 6,
    levels: [
      { level: 0, name: "7-1" },
      { level: 1, name: "7-2" },
      { level: 2, name: "7-3" },
      { level: 3, name: "7-4" },
      { level: 4, name: "7-5" },
      { level: 5, name: "7-6" },
      { level: 11, name: "7-7" },
      { level: 6, name: "7-Fortress" },
      { level: 15, name: "7-Bonus" },
      { level: 7, name: "7-Castle" },
    ],
    monsters: [
      { monster: 0, name: "7-Hammer Bros. 1" },
      { monster: 1, name: "7-Hammer Bros. 2" },
      { monster: 2, name: "7-Boomerang Bros." },
    ],
  },
  {
    world: 7,
    levels: [
      { level: 0, name: "8-1" },
      { level: 1, name: "8-2" },
      { level: 2, name: "8-3" },
      { level: 3, name: "8-4" },
      { level: 4, name: "8-5" },
      { level: 5, name: "8-6" },
      { level: 6, name: "8-Fortress" },
      { level: 19, name: "8-Bowser" },
    ],
    monsters: [],
  },
];
