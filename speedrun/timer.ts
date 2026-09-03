// The run itself: what is closed, when, and how it stands against the record.
//
// Knows nothing about the page. It takes events in and hands out a view of the
// run, which panel.ts draws and manage.ts reads; that split is what lets the
// whole of the timing be exercised without a DOM.

import { type GameEvent } from "./events.ts";
import {
  type Route,
  type RouteSplit,
  type Trigger,
  triggered,
} from "./route.ts";
import {
  type RouteRecord,
  type RunRecord,
  segments,
  sumOfBest,
  timeAt,
  withRun,
} from "./records.ts";
import { duration } from "./times.ts";

export type RunState = "idle" | "running" | "finished" | "abandoned";

export type SplitState = "ahead" | "current" | "closed" | "skipped";

export interface SplitView {
  readonly name: string;
  readonly state: SplitState;
  readonly at: Temporal.Duration | null;
  /** Against the personal best, where both this run and it closed the split. */
  readonly delta: Temporal.Duration | null;
  /** This split on its own, where the run has a segment to attribute to it. */
  readonly segment: Temporal.Duration | null;
  /**
   * That segment against the same one in the personal best.
   *
   * Two different questions, so two different figures: the delta above says
   * where the run stands overall, and this says whether the split just played
   * was better or worse than the one in the best - a run can be minutes down
   * and still have just run its finest 1-2.
   */
  readonly segmentDelta: Temporal.Duration | null;
  /** That segment is the best this split has ever been run: a gold. */
  readonly gold: boolean;
}

export interface TimerView {
  /** Null while no route is selected: the clock still runs, nothing splits. */
  readonly route: Route | null;
  readonly state: RunState;
  readonly recording: boolean;
  readonly elapsed: Temporal.Duration;
  readonly splits: readonly SplitView[];
  readonly pace: Temporal.Duration | null;
  readonly pb: Temporal.Duration | null;
  readonly sumOfBest: Temporal.Duration | null;
}

export class SpeedrunTimer {
  readonly #route: Route | null;
  #record: RouteRecord;
  readonly #changed: () => void;

  /** The best as it stood when this run started, which is what it is shown against. */
  #comparison: RunRecord | null;
  /**
   * The best each split had ever been when this run started.
   *
   * Snapshot for the same reason the personal best is: the record is updated
   * when a run stops, and a segment must be measured against what it had to
   * beat rather than against itself once it has.
   */
  #golds: ReadonlyMap<string, Temporal.Duration>;

  #state: RunState = "idle";
  /** performance.now() when the run started. */
  #origin = 0;
  /** Milliseconds on the clock: live while running, final once not. */
  #elapsed = 0;
  /** Cumulative milliseconds at each closed split; null where skipped. */
  #closed: (number | null)[] = [];
  /** The split being run, one past the last once every split is closed. */
  #at = 0;

  #frame: number | null = null;

  /** Armed to record the next run rather than time it against the route. */
  #recording = false;
  /** This run is a recording, which is fixed when it starts and not after. */
  #recordingRun = false;
  #recorded: RouteSplit[] = [];

  constructor(route: Route | null, record: RouteRecord, changed: () => void) {
    this.#route = route;
    this.#record = record;
    this.#comparison = record.pb;
    this.#golds = record.best;
    this.#changed = changed;
  }

  get route(): Route | null {
    return this.#route;
  }

  /**
   * The splits this run is being measured against.
   *
   * A recording is measured against the route it is writing, which is as long
   * as the run so far; anything else against the selected route, of which there
   * may not be one.
   */
  get #splits(): readonly RouteSplit[] {
    return this.#recordingRun ? this.#recorded : (this.#route?.splits ?? []);
  }

  get record(): RouteRecord {
    return this.#record;
  }

  get running(): boolean {
    return this.#state === "running";
  }

  /** The run is over, so there is something worth writing down. */
  get settled(): boolean {
    return this.#state === "finished" || this.#state === "abandoned";
  }

  get recording(): boolean {
    return this.#recordingRun || this.#recording;
  }

  /** The run that just stopped was a recording, so it produced a route. */
  get recordedRun(): boolean {
    return this.#recordingRun;
  }

  /**
   * Record the next run rather than time it against the route.
   *
   * The splits are appended as they are played, so the panel shows the route
   * being written as it is written. Disarming part way through a recording ends
   * it where it stands rather than being ignored: the route is whatever was
   * played up to that point, which is the only thing "stop recording" can
   * reasonably mean while a game is going on.
   */
  arm(recording: boolean): void {
    this.#recording = recording;

    if (!recording && this.#recordingRun && this.#state === "running") {
      this.#stop("finished");
    }
  }

  /** The route just recorded, or null if the last run was not one. */
  takeRecording(name: string, category: string): Route | null {
    if (this.#recorded.length === 0) return null;

    const id = `rec-${Date.now().toString(36)}`;
    const route: Route = {
      id,
      category,
      name,
      splits: this.#recorded,
    };

    this.#recorded = [];
    return route;
  }

  get recorded(): readonly RouteSplit[] {
    return this.#recorded;
  }

  handle(event: GameEvent): void {
    switch (event.kind) {
      case "run-started":
        this.#start();
        break;

      case "run-abandoned":
        // Raised on every return to the main menu, the ending included, so a
        // run that already ended keeps the time it earned.
        if (this.#state === "running") this.#stop("abandoned");
        break;

      // Everything else is only ever a split. Beating Bowser is the last split
      // of a route that runs to the end of the game and nothing at all to one
      // that stops earlier, so it is offered to the route like any other event
      // rather than stopping the clock in its own right.
      default:
        this.#advance(event);
    }
  }

  #start(): void {
    this.#comparison = this.#record.pb;
    this.#golds = this.#record.best;
    this.#state = "running";
    this.#origin = performance.now();
    this.#elapsed = 0;
    this.#closed = [];
    this.#at = 0;
    this.#recordingRun = this.#recording;

    if (this.#recordingRun) this.#recorded = [];

    this.#tick();
  }

  #stop(state: "finished" | "abandoned"): void {
    this.#state = state;

    if (this.#frame !== null) cancelAnimationFrame(this.#frame);
    this.#frame = null;

    // A recorded run is a route, not a time over one: there was nothing to be
    // faster than, and its splits only became splits as it went.
    if (!this.#recordingRun) this.#record = withRun(this.#record, this.#run());

    this.#changed();
  }

  #tick(): void {
    this.#frame = requestAnimationFrame(() => {
      this.#elapsed = performance.now() - this.#origin;
      this.#changed();
      this.#tick();
    });
  }

  /** This run as it would be written down. */
  #run(): RunRecord {
    return {
      finished: this.#state === "finished",
      total: duration(this.#elapsed),
      splits: this.#splits.map((split, i) => {
        const ms = this.#closed[i];

        return {
          id: split.id,
          at: ms === undefined || ms === null ? null : duration(ms),
        };
      }),
    };
  }

  /**
   * Close whichever split this event answers to, if any.
   *
   * Matched against every split still ahead rather than only the next one: a
   * run can leave its route - a warp pipe skips whole worlds - and the splits
   * jumped over are then closed as skipped. They took no time that can be told
   * apart from the split which swallowed them.
   */
  #advance(event: GameEvent): void {
    if (this.#state !== "running") return;

    if (this.#recordingRun) {
      this.#appendRecorded(event);
      return;
    }

    const at = this.#splits.findIndex(
      (split, i) => i >= this.#at && triggered(split.on, event),
    );
    if (at < 0) return;

    this.#close(at);
  }

  #close(at: number): void {
    this.#elapsed = performance.now() - this.#origin;

    while (this.#at < at) this.#closed[this.#at++] = null;
    this.#closed[this.#at++] = this.#elapsed;

    // The route is out of splits, so the run is over. What finishes a run is
    // its last split closing, whichever event did that: beating Bowser for a
    // route that goes the distance, beating world 1's castle for one that
    // stops there.
    //
    // Never while recording: that route is as long as the run so far, so it is
    // out of splits after every single one of them. What ends a recording is
    // the game saying so.
    if (!this.#recordingRun && this.#at === this.#splits.length) {
      this.#stop("finished");
      return;
    }

    this.#changed();
  }

  /** One more split on the route being written, for a level just beaten. */
  #appendRecorded(event: GameEvent): void {
    // Nothing left to record: the game is over and the route is whatever was
    // played. An abandoned run is stopped by run-abandoned in handle().
    if (event.kind === "run-ended") {
      this.#stop("finished");
      return;
    }

    if (event.kind !== "level-completed") return;

    const on: Trigger = {
      kind: "level-completed",
      world: event.world,
      level: event.level,
    };

    // Numbered rather than named: nothing here knows what the level is called,
    // and a guess at it would be a name the player has to correct rather than
    // one they can accept. The routes section is where these get their names;
    // the id is what a saved time is tied to, so renaming costs nothing.
    this.#recorded.push({
      id: `w${event.world}-l${event.level}`,
      name: `Split ${this.#recorded.length + 1}`,
      on,
    });

    this.#close(this.#recorded.length - 1);
  }

  view(): TimerView {
    const running = this.#state === "running";
    // A recording is not a run over anything: the route is being written by it,
    // so there is no earlier time these splits were reached at, no segment that
    // was ever beaten, and nothing to be ahead of. Held back from the view
    // rather than from the panel so that nothing downstream has to remember it.
    const recording = this.recording;
    const pb = recording ? null : this.#comparison;
    // The best's splits taken apart the same way this run's are, so the two
    // can be compared segment against segment rather than only total against
    // total.
    const pbSegments = pb === null ? null : segments(pb);

    // A split that follows skipped ones covers them too. That time is real but
    // cannot be divided up, so it is not a segment for any one of them and
    // stands as a gold for none - the same rule segments() saves runs by.
    let previous = duration(0);
    let coversSkipped = false;

    const splits = this.#splits.map((split, i): SplitView => {
      const ms = this.#closed[i] ?? null;
      const skipped = i < this.#at && ms === null;
      const at = ms === null ? null : duration(ms);
      const pbAt = pb === null ? null : timeAt(pb, split.id);
      const pbSegment = pbSegments?.get(split.id) ?? null;

      let segment: Temporal.Duration | null = null;

      if (at !== null) {
        if (!coversSkipped) segment = at.subtract(previous);

        previous = at;
        coversSkipped = false;
      } else if (skipped) {
        coversSkipped = true;
      }

      const best = this.#golds.get(split.id);

      return {
        name: split.name,
        state: skipped
          ? "skipped"
          : at !== null
            ? "closed"
            : running && i === this.#at
              ? "current"
              : "ahead",
        at,
        delta: at === null || pbAt === null ? null : at.subtract(pbAt),
        segment,
        segmentDelta:
          segment === null || pbSegment === null
            ? null
            : segment.subtract(pbSegment),
        // Never run before is as good as it has ever been, which is what makes
        // every segment of a first run a gold.
        gold:
          !recording &&
          segment !== null &&
          (best === undefined ||
            Temporal.Duration.compare(segment, best) < 0),
      };
    });

    return {
      route: this.#route,
      state: this.#state,
      recording,
      elapsed: duration(this.#elapsed),
      splits,
      pace: this.#pace(),
      pb: recording ? null : (this.#record.pb?.total ?? null),
      sumOfBest:
        recording || this.#route === null
          ? null
          : sumOfBest(this.#route, this.#record),
    };
  }

  /**
   * Where the run stands against the best.
   *
   * At a closed split that is the difference there. Between splits it appears
   * only once the clock has passed the time the best had reached by now, which
   * is the moment this split stopped being on pace - there is nothing to say
   * before that except that the split is not over.
   */
  #pace(): Temporal.Duration | null {
    const pb = this.#comparison;
    if (pb === null || this.#state === "idle" || this.#recordingRun) return null;

    const now = duration(this.#elapsed);

    if (this.#state === "running") {
      const here = this.#splits[this.#at];
      const target = here === undefined ? null : timeAt(pb, here.id);

      if (target !== null && Temporal.Duration.compare(now, target) > 0) {
        return now.subtract(target);
      }
    }

    const last = this.#splits[this.#at - 1];
    if (last === undefined) return null;

    const ms = this.#closed[this.#at - 1] ?? null;
    const was = timeAt(pb, last.id);

    return ms === null || was === null ? null : duration(ms).subtract(was);
  }
}
