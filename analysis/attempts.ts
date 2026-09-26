// The run history read back: rows as history.ts stores and exports them, made
// into something that can be counted.
//
// Rows come from two places - this browser's own history, and exported files
// opened on the page, which may have come from another machine or an older
// build - so nothing about one is taken on trust. A row that is not an attempt
// at all is dropped; an event in it that is not one this build knows is
// dropped on its own, and the rest of the attempt is kept.

import { type CategoryId, isCategoryId } from "../speedrun/category.ts";
import {
  type GameEvent,
  type Item,
  type Loadout,
  type Power,
  isEventKind,
} from "../speedrun/events.ts";
import { parseDuration, parseStamp } from "../speedrun/times.ts";

export type AttemptMode = "run" | "practice";

export interface TimedEvent {
  /** Milliseconds since the attempt started. */
  readonly at: number;
  readonly event: GameEvent;
}

export interface Attempt {
  readonly id: string;
  readonly mode: AttemptMode;
  readonly started: Temporal.ZonedDateTime;
  /** Null for an attempt the page closed on before it was over. */
  readonly ended: Temporal.ZonedDateTime | null;
  readonly category: CategoryId | null;
  readonly route: string | null;
  readonly recording: boolean;
  /** Whether the timer called the run finished; false for practice. */
  readonly finished: boolean;
  /** The run's time on the clock, as the timer had it; null for practice. */
  readonly total: number | null;
  readonly events: readonly TimedEvent[];
}

const POWERS: readonly Power[] = ["small", "super", "fire", "racoon"];

function ms(d: Temporal.Duration): number {
  return d.total({ unit: "millisecond" });
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}

function isIndex(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value) && value >= 0;
}

function parseLoadout(value: unknown): Loadout | null {
  if (!isRecord(value)) return null;

  const { power, star, pwing, items } = value;
  if (!POWERS.includes(power as Power)) return null;

  return {
    power: power as Power,
    star: star === true,
    pwing: pwing === true,
    items: Array.isArray(items)
      ? items.filter((item): item is Item => typeof item === "string")
      : [],
  };
}

/**
 * One event from a row, or null if this build cannot make sense of it.
 *
 * Checked only as far as the analysis leans on it: the kind, the numbers that
 * say where, and the loadout on the kinds that carry one.
 */
function parseEvent(value: unknown): TimedEvent | null {
  if (!isRecord(value)) return null;

  const at = parseDuration(value["at"]);
  const { kind } = value;
  if (at === null || !isEventKind(kind)) return null;

  for (const field of ["world", "level", "monster"]) {
    if (value[field] !== undefined && !isIndex(value[field])) return null;
  }

  if (
    kind === "level-entered" ||
    kind === "monster-fought" ||
    kind === "player-died" ||
    kind === "player-hit"
  ) {
    const player = parseLoadout(value["player"]);
    if (player === null) return null;

    return { at: ms(at), event: { ...value, kind, player } as GameEvent };
  }

  // Beaten, and since the game started saying so, beaten as what: kept where
  // it reads, and left off where it is missing or does not.
  if (kind === "level-completed" || kind === "monster-defeated") {
    const player = parseLoadout(value["player"]);
    const { player: _, ...rest } = value;
    return {
      at: ms(at),
      event: (player === null ? { ...rest, kind } : { ...rest, kind, player }) as unknown as GameEvent,
    };
  }

  return { at: ms(at), event: value as unknown as GameEvent };
}

export function parseAttempt(value: unknown): Attempt | null {
  if (!isRecord(value)) return null;

  const { id, mode, category, route, run } = value;
  const started = parseStamp(value["started"]);

  if (typeof id !== "string" || started === null) return null;
  if (mode !== "run" && mode !== "practice") return null;

  const events: TimedEvent[] = [];
  for (const entry of Array.isArray(value["events"]) ? value["events"] : []) {
    const event = parseEvent(entry);
    if (event !== null) events.push(event);
  }

  const total = isRecord(run) ? parseDuration(run["total"]) : null;

  return {
    id,
    mode,
    started,
    ended: parseStamp(value["ended"]),
    category: isCategoryId(category) ? category : null,
    route: typeof route === "string" ? route : null,
    recording: value["recording"] === true,
    finished: isRecord(run) && run["finished"] === true,
    total: total === null ? null : ms(total),
    events,
  };
}

/** The rows of an exported file: JSON Lines, one attempt to a line. */
export function parseLines(text: string): Attempt[] {
  const out: Attempt[] = [];

  for (const line of text.split("\n")) {
    if (line.trim() === "") continue;

    try {
      const attempt = parseAttempt(JSON.parse(line));
      if (attempt !== null) out.push(attempt);
    } catch {
      /* One bad line costs that line. */
    }
  }

  return out;
}

/** How long an attempt lasted, as far as its events say. */
export function span(attempt: Attempt): number {
  return attempt.total ?? attempt.events.at(-1)?.at ?? 0;
}
