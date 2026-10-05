// A movie being worked on: the game, the movie, and the greenzone that ties
// one to the other. Everything the panel does goes through here.
//
// The rule everything below keeps is that the game is always the movie played
// up to the current frame. An edit to a frame the game has already played
// breaks that, so the game is marked stale and put back - from the latest
// snapshot the edit left standing, replayed forward to where it was. Playing
// and stepping never start from a stale game.
//
// Recording is the one way the game writes to the movie: while it is on, each
// frame played takes what is held on the keyboard as its input, over whatever
// the movie had there, and the frames after it are left as they were.

import { Greenzone, KEYFRAME_INTERVAL } from "./greenzone.ts";
import type { Machine } from "./machine.ts";
import type { Input, Movie } from "./movie.ts";

/** How long a seek runs before it lets the page paint. */
const SEEK_SLICE_MS = 30;

/** At most this many frames are played per display refresh, however behind. */
const MAX_FRAMES_PER_REFRESH = 4;

/**
 * A run in the movie, timed the way the game page's speedrun timer times one:
 * from "New game" to the last frame Bowser is fought on.
 */
export interface RunTime {
  /** Real time, in milliseconds, from the run's start to the current frame or its end. */
  readonly ms: number;
  /** The run ended at or before the current frame, so `ms` is its final time. */
  readonly finished: boolean;
}

function nextRefresh(): Promise<number> {
  return new Promise((resolve) => requestAnimationFrame(resolve));
}

function yieldToPage(): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve));
}

/**
 * Raises "change" whenever there is something new to show - a frame played, a
 * mode switched - and "movie" whenever the movie itself changed, which is what
 * is worth saving.
 */
export class Session extends EventTarget {
  readonly greenzone = new Greenzone();

  /**
   * Whether each frame played since the last edit before it was a lag frame:
   * one where the game never read its input, so whatever the movie holds
   * there does nothing. Unknown for frames not played since.
   */
  readonly lag: boolean[] = [];

  /**
   * When each frame since the last edit before it was let go, in milliseconds
   * from power-on, as the game page would have let it go: frame n + 1 follows
   * frame n by the period the game asked for while waiting at n. That is the
   * grid src/compat/gray.cpp's wait_for_frame() keeps, whose average holds on
   * any display, so the difference between two of these is the real time a
   * player pressing these inputs would see pass between those frames.
   */
  readonly clock: number[] = [0];

  /**
   * The speedrun events raised while playing each frame since the last edit
   * before it, by the frame they were raised in.
   */
  readonly events = new Map<number, readonly string[]>();

  playing = false;
  recording = false;
  /** Playback speed, as a multiple of the game's own pace. */
  speed = 1;
  /** Why the last thing asked of the session failed, until the next succeeds. */
  error: string | null = null;

  #stale = false;
  #resyncQueued = false;
  #queue: Promise<void> = Promise.resolve();

  constructor(
    readonly machine: Machine,
    readonly movie: Movie,
    readonly live: () => Input,
  ) {
    super();
    this.greenzone.pin(this.#bookmarkFrames());
    this.greenzone.add(machine.capture());
  }

  get frame(): number {
    return this.machine.frame;
  }

  #changed(): void {
    this.dispatchEvent(new Event("change"));
  }

  #movieChanged(): void {
    this.dispatchEvent(new Event("movie"));
    this.#changed();
  }

  /** Runs one thing at a time against the game, in the order asked. */
  #enqueue(op: () => Promise<void> | void): Promise<void> {
    const run = this.#queue.then(async () => {
      try {
        await op();
        this.error = null;
      } catch (e) {
        this.error = e instanceof Error ? e.message : String(e);
        this.playing = false;
      }
      this.#changed();
    });
    this.#queue = run;
    return run;
  }

  // -------------------------------------------------------------------------
  // Playing

  /** Plays one frame and stops, recording it if recording is on. */
  advance(): Promise<void> {
    this.playing = false;
    return this.#enqueue(() => this.#step(this.recording, true));
  }

  play(): void {
    if (this.playing) return;
    this.playing = true;
    this.#changed();
    this.#run();
  }

  pause(): void {
    this.playing = false;
    this.#changed();
  }

  /**
   * Plays at the pace the game asks for, scaled by the speed, on a grid of
   * deadlines the way src/compat/gray.cpp's wait_for_frame() keeps it: a
   * frame is played on the first display refresh at or past its deadline,
   * and a deadline long gone - a hidden tab - restarts the grid instead of
   * being caught up on.
   */
  async #run(): Promise<void> {
    let due = performance.now();

    while (this.playing) {
      const now = await nextRefresh();
      if (!this.playing) break;

      if (now - due > 4 * (this.machine.period / this.speed || 50)) due = now;

      for (let n = 0; n < MAX_FRAMES_PER_REFRESH && now >= due; n++) {
        if (!this.recording && this.frame >= this.movie.inputs.length) {
          this.playing = false;
          break;
        }
        await this.#enqueue(() => this.#step(this.recording, true));
        if (!this.playing) break;
        due += this.machine.period / this.speed;
      }
    }
    this.#changed();
  }

  async #step(record: boolean, draw: boolean): Promise<void> {
    if (this.#stale) await this.#seek(this.frame, false);

    const frame = this.frame;
    if (record) this.#write(frame, this.live());

    this.machine.drawing = draw;
    const period = this.machine.period;
    const played = await this.machine.step();
    this.lag[frame] = !played.polled;
    this.clock[frame + 1] = (this.clock[frame] ?? NaN) + period;
    if (played.events.length > 0) this.events.set(frame, played.events);
    else this.events.delete(frame);

    // A bookmark's snapshot goes when an edit before it does, and comes
    // back the next time the game passes through it.
    const now = this.frame;
    const wanted =
      now % KEYFRAME_INTERVAL === 0 || this.movie.bookmarks.includes(now);
    if (wanted && !this.greenzone.has(now)) {
      this.greenzone.add(this.machine.capture());
    }
  }

  /**
   * The run the game is in at the current frame, if it is in one: the latest
   * "New game" before it that the game has not since gone back to the main
   * menu from. A run that ended keeps its time even once it has, as on the
   * game page.
   */
  runTime(): RunTime | null {
    const now = this.frame;
    let start: number | null = null;
    let end: number | null = null;

    const frames = [...this.events.keys()].filter((f) => f < now).sort((a, b) => a - b);
    for (const frame of frames) {
      for (const kind of this.events.get(frame)!) {
        if (kind === "run-started") {
          start = frame;
          end = null;
        } else if (kind === "run-abandoned" && end === null) {
          start = null;
        } else if (kind === "run-ended" && start !== null && end === null) {
          end = frame;
        }
      }
    }

    if (start === null) return null;
    const ms = (this.clock[end ?? now] ?? NaN) - this.clock[start]!;
    return Number.isNaN(ms) ? null : { ms, finished: end !== null };
  }

  // -------------------------------------------------------------------------
  // Seeking

  /** Puts the game at a frame, as the movie has it. */
  seek(frame: number): Promise<void> {
    return this.#enqueue(() => this.#seek(frame, true));
  }

  /**
   * Goes back a frame from wherever the game is by the time this runs, so
   * that pressing it five times while a seek is still going goes back five.
   */
  back(): Promise<void> {
    this.playing = false;
    return this.#enqueue(() => this.#seek(this.frame - 1, true));
  }

  /**
   * Starts from wherever is closest without going past the frame - the game
   * as it is, or the latest snapshot before the frame - and plays forward
   * from there with nothing drawn, painting only the frame it lands on.
   */
  async #seek(target: number, show: boolean): Promise<void> {
    const to = Math.max(0, target);
    const from = this.frame;
    const snapshot = this.greenzone.latest(to)!;

    if (this.#stale || from > to || snapshot.frame > from) {
      this.machine.restore(snapshot);
      this.#stale = false;
    }

    let slice = performance.now();
    while (this.frame < to) {
      await this.#step(false, false);
      if (performance.now() - slice > SEEK_SLICE_MS) {
        this.#changed();
        await yieldToPage();
        slice = performance.now();
      }
    }

    if (this.recording && to < from) {
      this.movie.rerecords++;
      this.#movieChanged();
    }
    if (show) this.machine.refresh();
  }

  // -------------------------------------------------------------------------
  // Editing

  /**
   * Everything after a frame is now something else: snapshots and lag past it
   * are dropped, and a game that has played past it is put back.
   */
  #changedFrom(frame: number): void {
    this.greenzone.invalidateAfter(frame);
    this.lag.length = Math.min(this.lag.length, frame + 1);
    this.clock.length = Math.min(this.clock.length, frame + 1);
    for (const at of this.events.keys()) if (at >= frame) this.events.delete(at);
    if (frame < this.frame) this.#stale = true;
    this.#movieChanged();
  }

  #write(frame: number, input: Input): boolean {
    const inputs = this.movie.inputs;
    if (frame < inputs.length && inputs[frame] === input) return false;

    while (inputs.length < frame) inputs.push(0);
    inputs[frame] = input;
    this.#changedFrom(frame);
    return true;
  }

  /**
   * Many edits arrive at once from a drag down the editor, and each would
   * otherwise replay the same frames again; one replay after them is enough.
   */
  #resync(): void {
    if (!this.#stale || this.#resyncQueued) return;

    this.#resyncQueued = true;
    this.#enqueue(async () => {
      this.#resyncQueued = false;
      if (this.#stale) await this.#seek(this.frame, true);
    });
  }

  setInput(frame: number, input: Input): void {
    if (this.#write(frame, input)) this.#resync();
  }

  /** A blank frame at `frame`, pushing that frame and everything after down. */
  insert(frame: number): void {
    if (frame > this.movie.inputs.length) return;
    this.movie.inputs.splice(frame, 0, 0);
    this.#changedFrom(frame);
    this.#resync();
  }

  remove(frame: number): void {
    if (frame >= this.movie.inputs.length) return;
    this.movie.inputs.splice(frame, 1);
    this.#changedFrom(frame);
    this.#resync();
  }

  /** Cuts the movie off at the current frame. */
  truncate(): void {
    if (this.frame >= this.movie.inputs.length) return;
    this.movie.inputs.length = this.frame;
    this.#changedFrom(this.frame);
  }

  // -------------------------------------------------------------------------
  // Bookmarks

  #bookmarkFrames(): number[] {
    return this.movie.bookmarks.filter((frame) => frame !== null);
  }

  setBookmark(slot: number): Promise<void> {
    return this.#enqueue(async () => {
      if (this.#stale) await this.#seek(this.frame, true);

      this.movie.bookmarks[slot] = this.frame;
      this.greenzone.pin(this.#bookmarkFrames());
      if (!this.greenzone.has(this.frame)) {
        this.greenzone.add(this.machine.capture());
      }
      this.#movieChanged();
    });
  }

  gotoBookmark(slot: number): Promise<void> {
    const frame = this.movie.bookmarks[slot];
    if (frame === null || frame === undefined) return Promise.resolve();
    this.playing = false;
    return this.seek(frame);
  }
}
