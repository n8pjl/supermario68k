// Every attempt, kept: the run history.
//
// records.ts keeps what a route's times are worth - the best run, the best each
// split has ever been - and lets the rest go, which is right for a panel and
// wrong for working out a route. This keeps the rest: every run the timer
// times, finished or reset, and every game played in practice mode, each with
// every event the game reported during it and when.
//
// The events are the point. A split time is an answer to one route, and routes
// change; the events are what happened, and read back just as well against
// whatever the route is next. The ones that enter a level carry what the player
// walked in with (see Loadout in events.ts), which is most of what an item
// route is made of.
//
// In IndexedDB rather than in localStorage beside the routes: this only grows,
// and localStorage is a few megabytes, read and written whole. Nothing reads it
// back during play, so it can be as asynchronous as it likes. Storage being
// unavailable costs the history and nothing else, the same as it does there.
//
// Written as it goes rather than when the attempt is over, so that closing the
// tab on a run in progress keeps it up to the last event. A row like that never
// gets an `ended`, which is the truth about it.

import { type CategoryId } from "./category.ts";
import { type GameEvent } from "./events.ts";
import { download } from "./files.ts";
import { type RunRecord, runToJSON } from "./records.ts";
import { duration, stamp } from "./times.ts";

const DB_NAME = "sm68k.history";
const DB_VERSION = 1;
const STORE = "attempts";

/**
 * Where every write is announced, once it has landed.
 *
 * IndexedDB says nothing to anyone else when it changes, and the data page is
 * usually open in another tab while the game is played in this one. So each
 * write is posted here as the rows it wrote, in their stored form, and a page
 * that wants to follow along merges them rather than reading everything again.
 * Posted after the transaction completes, so a reader that does go back to the
 * database finds what it was told about.
 */
export const HISTORY_CHANNEL = "sm68k.history";

/** Made the first time there is something to say. Null where there is none. */
let channel: BroadcastChannel | null | undefined;

function announce(rows: readonly unknown[]): void {
  if (channel === undefined) {
    try {
      channel = new BroadcastChannel(HISTORY_CHANNEL);
    } catch {
      channel = null;
    }
  }

  channel?.postMessage(rows);
}

/**
 * What an attempt was.
 *
 * A run is what the timer timed, from a new game to a route finished or
 * abandoned. A practice session is a game played with practice mode on, which
 * has no route and no clock of its own: it lasts from the first thing the game
 * reports to the return to the main menu.
 */
export type AttemptMode = "run" | "practice";

/** What a run is being timed against, fixed when it starts. */
export interface RunContext {
  readonly category: CategoryId;
  /** Null with no route selected, and for a recording until it is saved. */
  readonly route: string | null;
  readonly recording: boolean;
}

interface LoggedEvent {
  /** Since the attempt started, on the same monotonic clock a run is timed on. */
  readonly at: Temporal.Duration;
  readonly event: GameEvent;
}

interface Attempt {
  /** Sorts in the order the attempts were started; see begin(). */
  readonly id: string;
  readonly mode: AttemptMode;
  readonly started: Temporal.ZonedDateTime;
  /** performance.now() at the start, which `at` is measured from. */
  readonly origin: number;
  /** Null while it is going, and for good if the page went before it ended. */
  ended: Temporal.ZonedDateTime | null;
  readonly category: CategoryId | null;
  route: string | null;
  readonly recording: boolean;
  /**
   * The timer's own account of the run - the splits, as records.ts writes them
   * - so a split time here is the one the panel showed. Null for practice.
   */
  run: RunRecord | null;
  readonly events: LoggedEvent[];
}

/**
 * One attempt as it is stored and exported.
 *
 * Durations and stamps as their ISO 8601 strings, the same as the speedrun
 * file (see times.ts), and each event flattened beside its time rather than
 * nested under it, so a line of the export reads as a row.
 */
function attemptToJSON(attempt: Attempt): unknown {
  return {
    id: attempt.id,
    mode: attempt.mode,
    started: attempt.started.toString(),
    ended: attempt.ended?.toString() ?? null,
    category: attempt.category,
    route: attempt.route,
    recording: attempt.recording,
    run: attempt.run === null ? null : runToJSON(attempt.run),
    events: attempt.events.map(({ at, event }) => ({
      at: at.toString(),
      ...event,
    })),
  };
}

/** The database, or null where the browser will not give us one. */
function openDatabase(): Promise<IDBDatabase | null> {
  return new Promise((resolve) => {
    let request: IDBOpenDBRequest;

    try {
      request = indexedDB.open(DB_NAME, DB_VERSION);
    } catch {
      resolve(null);
      return;
    }

    request.onupgradeneeded = () => {
      request.result.createObjectStore(STORE, { keyPath: "id" });
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => resolve(null);
  });
}

/** Every attempt as it was stored, or null where they cannot be read. */
function readRows(db: IDBDatabase | null): Promise<unknown[] | null> {
  return new Promise((resolve) => {
    if (db === null) {
      resolve(null);
      return;
    }

    try {
      const request = db.transaction(STORE).objectStore(STORE).getAll();

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => resolve(null);
    } catch {
      resolve(null);
    }
  });
}

/**
 * Every attempt this browser has kept, in the form they are stored and
 * exported, for reading rather than for adding to: the data page's way in.
 * Null where the browser is keeping none.
 */
export async function readHistory(): Promise<unknown[] | null> {
  const db = await openDatabase();
  const rows = await readRows(db);

  db?.close();
  return rows;
}

export class RunHistory {
  /** Opened the first time something is written or read, and kept. */
  #db: Promise<IDBDatabase | null> | null = null;
  #open: Attempt | null = null;
  /** Attempts changed since they were last written; see #save(). */
  readonly #dirty = new Set<Attempt>();

  #database(): Promise<IDBDatabase | null> {
    this.#db ??= openDatabase();
    return this.#db;
  }

  /**
   * Start an attempt, ending whichever one is still open.
   *
   * One can be: a practice game played from a save never returns to the menu
   * the way a new game does, so the next new game is the first thing to say it
   * is over.
   */
  begin(mode: AttemptMode, context: RunContext | null = null): void {
    this.end();

    const started = stamp();

    this.#open = {
      id: `att-${started.epochMilliseconds.toString(36)}`,
      mode,
      started,
      origin: performance.now(),
      ended: null,
      category: context?.category ?? null,
      route: context?.route ?? null,
      recording: context?.recording ?? false,
      run: null,
      events: [],
    };
    this.#save(this.#open);
  }

  /** One event from the game, if there is an attempt to put it in. */
  note(event: GameEvent): void {
    const open = this.#open;
    if (open === null) return;

    open.events.push({
      at: duration(performance.now() - open.origin),
      event,
    });
    this.#save(open);
  }

  /**
   * The run as the timer has it so far.
   *
   * Kept up with during the run rather than only taken at the end, for the same
   * reason the events are: a run the page closed on still has its splits.
   */
  progress(run: RunRecord): void {
    const open = this.#open;
    if (open === null) return;

    open.run = run;
    this.#save(open);
  }

  /**
   * The attempt is over: finished, abandoned, or the game has gone.
   *
   * `run` is the timer's last word on it, and `route` the id a recording was
   * saved under, which it has only now that it has been.
   */
  end(run?: RunRecord, route?: string): void {
    const open = this.#open;
    if (open === null) return;

    open.ended = stamp();
    if (run !== undefined) open.run = run;
    if (route !== undefined) open.route = route;

    this.#open = null;
    this.#save(open);
  }

  /**
   * One event from a game played in practice mode, which is the whole of how
   * a practice session is told apart from a run: the shell hands its events
   * here rather than to a timer.
   */
  practice(event: GameEvent): void {
    if (event.kind === "run-started" || this.#open === null) {
      this.begin("practice");
    }

    this.note(event);

    if (event.kind === "run-abandoned") this.end();
  }

  /**
   * Hand the player every attempt as a file, one per line, oldest first.
   *
   * JSON Lines rather than one document: it is meant for reading with other
   * tools, most of which take a line as a row, and it is the one file here that
   * is never imported back into the game - the data page reads it, but only to
   * show it. Answers with what to tell the player about it,
   * which both of the buttons that ask for it say the same way.
   */
  async export(): Promise<string> {
    const unreadable =
      "The run history could not be read: this browser is not keeping it.";

    const rows = await readRows(await this.#database());
    if (rows === null) return unreadable;

    download(
      `sm68k-history-${stamp().toPlainDate().toString()}.jsonl`,
      rows.map((row) => JSON.stringify(row) + "\n").join(""),
      "application/jsonl",
    );
    return `Exported ${rows.length} attempt(s).`;
  }

  /**
   * Write an attempt out once whatever is changing it now is done.
   *
   * Once per task rather than once per change: an event that closes a split
   * both notes it and moves the run on, and one write covers both. Taken as a
   * snapshot when the write is made, because the attempt goes on changing
   * after. The writes are queued on the one database promise, so they land in
   * the order they were made, and a later one never loses to an earlier.
   */
  #save(attempt: Attempt): void {
    const scheduled = this.#dirty.size > 0;

    this.#dirty.add(attempt);
    if (scheduled) return;

    queueMicrotask(() => {
      const rows = [...this.#dirty].map(attemptToJSON);

      this.#dirty.clear();
      this.#database().then((db) => {
        if (db === null) return;

        try {
          const transaction = db.transaction(STORE, "readwrite");
          const store = transaction.objectStore(STORE);

          for (const row of rows) store.put(row);
          transaction.oncomplete = () => announce(rows);
        } catch {
          /* Nothing to be done, and nothing that depends on it having worked. */
        }
      });
    });
  }
}
