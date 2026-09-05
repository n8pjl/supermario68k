// The stages and the overworld monsters of every world: what a 100% run has to
// have been through, and the whole of what the rules ask for.
//
// Generated from levels/ by tools/mkstages.py, and checked against it on every
// build - see the Makefile. Do not edit: an edit here is a claim about the
// level set that the level set does not make, and the category it feeds would
// then ask for a stage nobody can play or let a run past one it skipped.
//
// Worlds are numbered from zero, the way every event the game reports numbers
// them. A level is its index in the world file; a monster is its index in the
// map's object list, which is what tells two of them in one world apart - the
// arena they load is a level of the common file and is shared between them.

export interface WorldStages {
  readonly world: number;
  /** Every stage of that world, its castle or airship included. */
  readonly levels: readonly number[];
  /** Every Hammer Bros. and variant standing on its map. */
  readonly monsters: readonly number[];
}

export const STAGES: readonly WorldStages[] = [
  { world: 0, levels: [0, 1, 2, 3, 4, 5, 6, 7], monsters: [0] },
  { world: 1, levels: [0, 1, 2, 3, 4, 6, 7, 8, 9], monsters: [0, 1, 2] },
  { world: 2, levels: [0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 15], monsters: [0, 2] },
  { world: 3, levels: [0, 1, 2, 3, 7], monsters: [0, 1] },
  { world: 4, levels: [0, 1, 2, 3, 4, 6, 7], monsters: [0, 1] },
  { world: 5, levels: [0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 15], monsters: [0, 2, 3] },
  { world: 6, levels: [0, 1, 2, 3, 4, 5, 6, 7, 11, 15], monsters: [0, 1, 2] },
  { world: 7, levels: [0, 1, 2, 3, 4, 5, 6, 19], monsters: [] },
];
