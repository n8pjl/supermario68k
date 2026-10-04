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
    this.lag[frame] = !(await this.machine.step());

    // A bookmark's snapshot goes when an edit before it does, and comes
    // back the next time the game passes through it.
    const now = this.frame;
    const wanted =
      now % KEYFRAME_INTERVAL === 0 || this.movie.bookmarks.includes(now);
    if (wanted && !this.greenzone.has(now)) {
      this.greenzone.add(this.machine.capture());
    }
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
