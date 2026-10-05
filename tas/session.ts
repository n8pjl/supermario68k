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
//
// The game runs in a worker (see machine.ts), so the movie can be edited while
// the game is busy playing frames: whatever it played past an edit made in the
// meantime is not kept, and the game is put back as for any other edit.
//
// A game that hangs is restarted at the latest snapshot before where it hung.
//
// Whenever there is nothing else to do, the game plays on ahead through the
// part of the movie not played since it was last edited, a batch at a time,
// and is put back where it was after each: that is how the levels after an
// edit are found to play as they did (see segments.ts). Anything asked of the
// session goes ahead of the next batch.

import { Greenzone, KEYFRAME_INTERVAL } from "./greenzone.ts";
import { Hung, type Machine, type Player, type Snapshot, type TasEvent } from "./machine.ts";
import type { Input, Movie } from "./movie.ts";
import { beforeEdit, segments, type Segment } from "./segments.ts";

/**
 * At most this many frames are handed to the game at once while seeking. It
 * answers sooner than that if they take a while, so this only bounds what is
 * sent over.
 */
const SEEK_BATCH = 2048;

/** At most this many frames are played per display refresh, however behind. */
const MAX_FRAMES_PER_REFRESH = 4;

/**
 * The most a player's X moves in a frame that is still a move: more is a
 * jump to somewhere else - a pipe, a door, a level started over.
 */
const MAX_MOVE = 16;

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
  readonly events = new Map<number, readonly TasEvent[]>();

  /**
   * The player's X once each frame since the last edit before it was played,
   * or null for one played outside a level.
   */
  readonly x: (number | null)[] = [];

  playing = false;
  recording = false;
  /** Playback speed, as a multiple of the game's own pace. */
  speed = 1;
  /** Why the last thing asked of the session failed, until the next succeeds. */
  error: string | null = null;

  #stale = false;
  /** The earliest frame edited since the game was last asked to do something. */
  #editedFrom = Infinity;
  #resyncQueued = false;
  #queue: Promise<void> = Promise.resolve();

  /** The segments as they stood before the last edit, moved to where it left them. */
  #previous: Segment[] = [];

  /**
   * Where the game is to the page while it plays on ahead: the frame it is
   * put back at after, and the player there. Null the rest of the time.
   */
  #home: { frame: number; player: Player | null } | null = null;
  #aheadQueued = false;
  /** Why playing on ahead stopped, until the next edit. */
  aheadStopped: string | null = null;

  constructor(
    readonly machine: Machine,
    readonly movie: Movie,
    readonly live: () => Input,
  ) {
    super();
    this.greenzone.pin(this.#bookmarkFrames());
    this.greenzone.add(machine.powerOn);
    this.#scheduleAhead();
  }

  /** The frame the game is waiting to play, as far as the page is concerned. */
  get frame(): number {
    return this.#home?.frame ?? this.machine.frame;
  }

  /** The player at the current frame, or null outside a level. */
  get player(): Player | null {
    return this.#home ? this.#home.player : this.machine.player;
  }

  /** How many frames from the start have been played since they were last edited. */
  get known(): number {
    return this.clock.length - 1;
  }

  /**
   * How far the player's X moved on a frame, in pixels: null on one not
   * played since it was edited, played outside a level, or that took the
   * player somewhere else altogether.
   */
  moved(frame: number): number | null {
    const now = this.x[frame];
    const before = this.x[frame - 1];
    if (now === null || now === undefined || before === null || before === undefined) return null;
    const moved = now - before;
    return Math.abs(moved) > MAX_MOVE ? null : moved;
  }

  /** The movie cut into levels: see segments.ts. */
  segments(): Segment[] {
    return segments(this.events, this.known, this.movie.inputs.length, this.#previous);
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
        if (e instanceof Hung) await this.#recover(e);
      }
      this.#changed();
      if (this.#queue === run) this.#scheduleAhead();
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
    this.#scheduleAhead();
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
        if (!this.recording && this.machine.frame >= this.movie.inputs.length) {
          this.playing = false;
          break;
        }
        await this.#enqueue(() => this.#step(this.recording, true));
        if (!this.playing) break;
        due += this.machine.period / this.speed;
      }
    }
    this.#changed();
    this.#scheduleAhead();
  }

  async #step(record: boolean, draw: boolean): Promise<void> {
    if (this.#stale) await this.#reach(this.machine.frame);

    const frame = this.machine.frame;
    if (record) this.#write(frame, this.live());
    await this.#play(1, draw);
  }

  /**
   * Plays up to `count` frames of the movie from the current one - fewer if
   * the game answers before it gets through them all - and keeps what they
   * did, up to any edit made while they played.
   */
  async #play(count: number, draw: boolean): Promise<void> {
    const from = this.machine.frame;
    const inputs = Array.from({ length: count }, (_, i) => this.movie.inputs[from + i] ?? 0);

    // A bookmark's snapshot goes when an edit before it does, and comes
    // back the next time the game passes through it.
    const capture: number[] = [];
    for (let at = from + 1; at <= from + count; at++) {
      const wanted = at % KEYFRAME_INTERVAL === 0 || this.movie.bookmarks.includes(at);
      if (wanted && !this.greenzone.has(at)) capture.push(at);
    }

    this.#editedFrom = Infinity;
    const { frames, snapshots } = await this.machine.play(inputs, draw, capture);
    const edited = this.#editedFrom;

    frames.forEach((played, i) => {
      const frame = from + i;
      if (frame >= edited) return;
      this.lag[frame] = !played.polled;
      this.clock[frame + 1] = (this.clock[frame] ?? NaN) + played.period;
      this.x[frame] = played.x;
      if (played.events.length > 0) this.events.set(frame, played.events);
      else this.events.delete(frame);
    });
    for (const snapshot of snapshots) {
      if (snapshot.frame <= edited) this.greenzone.add(snapshot);
    }
    if (edited < this.machine.frame) this.#stale = true;

    if (this.machine.ended) throw new Error(this.machine.ended);
  }

  /**
   * Starts a game that hung again, at the latest snapshot before where it
   * hung. Played on from there as the movie stands, it is likely to hang the
   * same way: the movie wants changing somewhere before then.
   */
  async #recover(hung: Hung): Promise<void> {
    const snapshot = this.greenzone.latest(hung.frame)!;
    try {
      await this.#restore(snapshot);
      await this.machine.refresh();
      this.error += ` It was restarted at frame ${snapshot.frame}.`;
    } catch (e) {
      this.error += ` It could not be restarted: ${e instanceof Error ? e.message : String(e)}`;
    }
  }

  /**
   * Puts the game back at a snapshot. If an edit while that was going on
   * left the snapshot behind, the game is stale again.
   */
  async #restore(snapshot: Snapshot): Promise<void> {
    this.#stale = false;
    this.#editedFrom = Infinity;
    await this.machine.restore(snapshot);
    if (this.#editedFrom < snapshot.frame) this.#stale = true;
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
      for (const { kind } of this.events.get(frame)!) {
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
    return this.#enqueue(() => this.#seek(this.machine.frame - 1, true));
  }

  /**
   * Starts from wherever is closest without going past the frame - the game
   * as it is, or the latest snapshot before the frame - and plays forward
   * from there with nothing drawn, painting only the frame it lands on.
   */
  async #seek(target: number, show: boolean): Promise<void> {
    const to = Math.max(0, target);
    const from = this.machine.frame;

    await this.#reach(to);

    if (this.recording && to < from) {
      this.movie.rerecords++;
      this.#movieChanged();
    }
    if (show) await this.machine.refresh();
  }

  /** Puts the game at a frame, drawing nothing. */
  async #reach(to: number): Promise<void> {
    // An edit while the game is playing forward can leave it stale again,
    // and sends it back.
    for (;;) {
      const snapshot = this.greenzone.latest(to)!;
      const at = this.machine.frame;
      if (this.#stale || at > to || snapshot.frame > at) {
        await this.#restore(snapshot);
        continue;
      }
      if (at >= to) break;

      await this.#play(Math.min(to - at, SEEK_BATCH), false);
      this.#changed();
    }
  }

  // -------------------------------------------------------------------------
  // Playing on ahead

  /**
   * Plays on ahead once nothing else is waiting, if there is any of the
   * movie the game has not played since it was edited.
   */
  #scheduleAhead(): void {
    if (this.#aheadQueued || this.playing || this.aheadStopped !== null) return;
    if (this.known >= this.movie.inputs.length) return;

    this.#aheadQueued = true;
    setTimeout(() => {
      this.#aheadQueued = false;
      if (this.playing) return;
      this.#enqueue(() => this.#ahead());
    }, 0);
  }

  /**
   * One batch of playing on ahead: from the end of what is known, as far as
   * the game gets in one request, and then back to where the page has it,
   * which looks no different from never having left.
   *
   * Anything going wrong out there stops it until the next edit. A game
   * that hung or ended there is started again, which the page's frame -
   * behind where that happened - restores into.
   */
  async #ahead(): Promise<void> {
    if (this.#stale) await this.#seek(this.machine.frame, true);
    const from = this.known;
    if (this.playing || from >= this.movie.inputs.length) return;

    this.#home = { frame: this.machine.frame, player: this.machine.player };
    let restarted = false;
    try {
      await this.#reach(from);
      await this.#play(Math.min(this.movie.inputs.length - from, SEEK_BATCH), false);
    } catch (e) {
      const why = e instanceof Error ? e.message : String(e);
      this.aheadStopped = `Playing on ahead stopped at frame ${this.machine.frame}: ${why}`;
      restarted = e instanceof Hung || this.machine.ended !== null;
      if (this.machine.ended) this.machine.restart();
    }

    const home = this.#home.frame;
    try {
      await this.#reach(home);
      if (restarted) await this.machine.refresh();
    } finally {
      this.#home = null;
    }
  }

  // -------------------------------------------------------------------------
  // Editing

  /**
   * Everything after a frame is now something else: snapshots and lag past it
   * are dropped, and a game that has played past it is put back. The frames
   * from there on have moved by `shift`, which is where the segments after
   * it are expected to be found again.
   */
  #changedFrom(frame: number, shift: number): void {
    this.#previous = beforeEdit(this.segments(), frame, shift);
    this.aheadStopped = null;
    this.greenzone.invalidateAfter(frame);
    this.lag.length = Math.min(this.lag.length, frame + 1);
    this.x.length = Math.min(this.x.length, frame);
    this.clock.length = Math.min(this.clock.length, frame + 1);
    for (const at of this.events.keys()) if (at >= frame) this.events.delete(at);
    this.#editedFrom = Math.min(this.#editedFrom, frame);
    if (frame < this.machine.frame) this.#stale = true;
    this.#movieChanged();
  }

  #write(frame: number, input: Input): boolean {
    const inputs = this.movie.inputs;
    if (frame < inputs.length && inputs[frame] === input) return false;

    const shift = Math.max(0, frame + 1 - inputs.length);
    while (inputs.length < frame) inputs.push(0);
    inputs[frame] = input;
    this.#changedFrom(frame, shift);
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
      if (this.#stale) await this.#seek(this.machine.frame, true);
    });
  }

  setInput(frame: number, input: Input): void {
    if (this.#write(frame, input)) this.#resync();
  }

  /** A blank frame at `frame`, pushing that frame and everything after down. */
  insert(frame: number): void {
    if (frame > this.movie.inputs.length) return;
    this.movie.inputs.splice(frame, 0, 0);
    this.#changedFrom(frame, 1);
    this.#resync();
  }

  remove(frame: number): void {
    if (frame >= this.movie.inputs.length) return;
    this.movie.inputs.splice(frame, 1);
    this.#changedFrom(frame, -1);
    this.#resync();
  }

  /** Cuts the movie off at the current frame. */
  truncate(): void {
    const frame = this.frame;
    if (frame >= this.movie.inputs.length) return;
    this.movie.inputs.length = frame;
    this.#changedFrom(frame, 0);
  }

  // -------------------------------------------------------------------------
  // Bookmarks

  #bookmarkFrames(): number[] {
    return this.movie.bookmarks.filter((frame) => frame !== null);
  }

  setBookmark(slot: number): Promise<void> {
    return this.#enqueue(async () => {
      if (this.#stale) await this.#seek(this.machine.frame, true);

      this.movie.bookmarks[slot] = this.machine.frame;
      this.greenzone.pin(this.#bookmarkFrames());
      if (!this.greenzone.has(this.machine.frame)) {
        this.#editedFrom = Infinity;
        const snapshot = await this.machine.capture();
        if (this.#editedFrom >= snapshot.frame) this.greenzone.add(snapshot);
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
