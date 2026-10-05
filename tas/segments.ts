// The movie cut into levels. A segment ends on the frame a level or a monster
// fight is beaten and is named after it, so it holds the walk over the map
// to that level as well as the level itself, and any deaths on the way. The
// first starts at power-on, menus and all. Whatever follows the last level
// beaten is a segment of its own, unnamed until it ends.
//
// Most edits to a level leave the levels after it as they were: they play the
// same inputs from a state the edit did not touch, just sooner or later. So
// the levels after an edit keep what they were - shifted by the frames the
// edit added or took out - until the game has played them again, and then
// each is told apart by whether it still takes as long as it did. One that
// does not, or a different level where one was, is where the edit reached
// further than its own level.

import { levelName, monsterName } from "../speedrun/names.ts";
import type { TasEvent } from "./game.ts";

export interface Segment {
  /** The level beaten at its end, as the map shows it; "" if it has not ended. */
  readonly name: string;
  /** Its first frame. */
  readonly start: number;
  /** The frame after its last: the one after the level was beaten on. */
  readonly end: number;
  /** Played since the last edit before its end, so it is as the movie has it. */
  readonly checked: boolean;
  /** Its length the first time it was seen whole, this session. */
  readonly base: number | null;
  /** What it was before the last edit, if it lay wholly after it. */
  readonly prior: { readonly name: string; readonly length: number } | null;
}

export function length(segment: Segment): number {
  return segment.end - segment.start;
}

/**
 * A segment after the last edit, played again since and found not to be what
 * it was before it: another level, or the same taking longer or shorter.
 */
export function changed(segment: Segment): boolean {
  const prior = segment.prior;
  return (
    segment.checked &&
    segment.name !== "" &&
    prior !== null &&
    (prior.name !== segment.name || prior.length !== length(segment))
  );
}

/** What ends a segment, and what it is called. */
function beaten(event: TasEvent): string | null {
  if (event.kind === "level-completed") return levelName(event.world, event.index);
  if (event.kind === "monster-defeated") return monsterName(event.world, event.index);
  return null;
}

/**
 * The segments of a movie of `frames` frames, of which the first `known` have
 * been played since they were last edited and raised `events`. What lies past
 * those is taken from `previous`, the segments as they stood before that edit,
 * moved to where it left them.
 */
export function segments(
  events: ReadonlyMap<number, readonly TasEvent[]>,
  known: number,
  frames: number,
  previous: readonly Segment[],
): Segment[] {
  const out: Segment[] = [];
  let start = 0;

  const add = (name: string, end: number, checked: boolean) => {
    const was = previous[out.length];
    const same = was !== undefined && was.name === name;
    out.push({
      name,
      start,
      end,
      checked,
      base: same && was.base !== null ? was.base : name !== "" ? end - start : null,
      prior: was?.prior ?? null,
    });
    start = end;
  };

  const at = [...events.keys()].filter((f) => f < known).sort((a, b) => a - b);
  for (const frame of at) {
    for (const event of events.get(frame)!) {
      const name = beaten(event);
      if (name !== null) add(name, frame + 1, true);
    }
  }

  if (known < frames) {
    // Not played since the edit: as it was, from the segment the end of what
    // is known falls in.
    const rest = previous.filter((s) => s.end > known && s.end > start);
    for (const was of rest) {
      if (was.name === "") break;
      add(was.name, was.end, false);
    }
  }
  // Always at least one, so that every frame - power-on in an empty movie
  // included - is in some segment.
  if (start < frames || out.length === 0) add("", Math.max(frames, start), known >= frames);

  return out;
}

/**
 * The segments as they stand, readied to be `previous` once the movie is
 * edited at `frame` and every frame from there on moves by `shift`: one
 * inserted is +1, one deleted -1, one changed 0. Each after the edited one
 * remembers what it was now, which is what it is held to once it has been
 * played again; the edited one and those before it are held to nothing.
 */
export function beforeEdit(current: readonly Segment[], frame: number, shift: number): Segment[] {
  return current.map((s) => {
    if (s.end <= frame) return { ...s, prior: null };
    if (s.start <= frame) return { ...s, end: Math.max(s.start, s.end + shift), checked: false, prior: null };
    const prior = { name: s.name, length: length(s) };
    return { ...s, start: s.start + shift, end: s.end + shift, checked: false, prior };
  });
}
