// The run itself: what is closed, when, and how it stands against the record.
//
// Knows nothing about the page. It takes events in and hands out a view of the
// run, which panel.ts draws and manage.ts reads; that split is what lets the
// whole of the timing be exercised without a DOM.

import {
  type Category,
  type CategoryId,
  category,
  placeIn,
} from "./category.ts";
import { type GameEvent } from "./events.ts";
import { entersMultipleWorlds, groupSplits } from "./groups.ts";
import {
  type Route,
  type RouteSplit,
  timedSplits,
  triggered,
} from "./route.ts";
import {
  type RouteRecord,
  type RunRecord,
  emptyRecord,
  segments,
  sumOfBest,
  timeAt,
  withRun,
} from "./records.ts";
import { duration, shorter } from "./times.ts";

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

/**
 * One world, timed as the long segment that holds its levels.
 *
 * `from` and `to` index into `TimerView.splits`, so the panel can draw a header
 * above the rows it covers. Only worth drawing when `TimerView.nested` is set -
 * a route that never leaves a world is read as its flat list of splits.
 */
export interface GroupView {
  readonly name: string;
  readonly world: number | null;
  readonly from: number;
  readonly to: number;
  readonly state: SplitState;
  /** The running total when the world's last split closed. */
  readonly at: Temporal.Duration | null;
  /** That total against the personal best's, where both reached the world's end. */
  readonly delta: Temporal.Duration | null;
  /** The world start to end, where this run has a segment to attribute to it. */
  readonly segment: Temporal.Duration | null;
  /** That world segment against the best that world has ever been run in. */
  readonly segmentDelta: Temporal.Duration | null;
  /** That world segment is the best it has ever been: a gold on the world. */
  readonly gold: boolean;
}

export interface TimerView {
  /** The rules being run. There is always one; a route there may not be. */
  readonly category: Category;
  /** Null while no route is selected: the clock still runs, nothing splits. */
  readonly route: Route | null;
  readonly state: RunState;
  readonly recording: boolean;
  readonly elapsed: Temporal.Duration;
  readonly splits: readonly SplitView[];
  /** The splits gathered by world; drawn only where `nested` is set. */
  readonly groups: readonly GroupView[];
  /** The route crosses more than one world, so the world grouping is worth showing. */
  readonly nested: boolean;
  readonly pace: Temporal.Duration | null;
  readonly pb: Temporal.Duration | null;
  readonly sumOfBest: Temporal.Duration | null;
}

export class SpeedrunTimer {
  /**
   * The rules being run.
   *
   * A run is under a category first and over a route second: the category is
   * what the time will be comparable against, and it is what a recording is
   * written for. So it is fixed when the timer is built, alongside the route,
   * rather than being asked for once the run is over.
   */
  readonly #category: CategoryId;
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
  /** The best each world segment had been when this run started; see #golds. */
  #groupGolds: ReadonlyMap<string, Temporal.Duration>;

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
  /**
   * The recording ran out of category: it wrote the last split the rules have.
   *
   * Which is the one thing here the game is told about, because it is the one
   * thing the game cannot work out for itself - it would carry on into the next
   * level with the player still playing, timing nothing. Cleared when a run
   * starts, so it only ever describes the run that just stopped.
   */
  #completedRecording = false;

  constructor(
    under: CategoryId,
    route: Route | null,
    record: RouteRecord,
    changed: () => void,
  ) {
    this.#category = under;
    this.#route = route;
    this.#record = record;
    this.#comparison = record.pb;
    this.#golds = record.best;
    this.#groupGolds = record.bestGroups;
    this.#changed = changed;
  }

  get category(): Category {
    return category(this.#category);
  }

  get route(): Route | null {
    return this.#route;
  }

  /**
   * The splits this run is being measured against, warps aside.
   *
   * A recording is measured against the route it is writing, which is as long
   * as the run so far; anything else against the selected route, of which there
   * may not be one. Either way the warp markers are dropped: they are not a
   * split the run is timed on (see route.ts), so nothing from here on - the
   * clock, the panel, the record - ever has to think about them. They stay on
   * `#recorded` for the route that takeRecording() writes.
   */
  get #splits(): readonly RouteSplit[] {
    return timedSplits(
      this.#recordingRun ? this.#recorded : (this.#route?.splits ?? []),
    );
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
   * That recording stopped because its category was over, not because the game
   * was: there is nothing left to record and the game should stop too.
   */
  get completedRecording(): boolean {
    return this.#completedRecording;
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

  /**
   * The route just recorded and the times the run that wrote it set, or null if
   * the last run was not a recording.
   *
   * Filed under the category it was recorded for, unless the run did not obey
   * those rules - a warpless attempt that took the whistle - in which case it
   * goes to the first category that will have it. Saving it somewhere is worth
   * more than holding out for the category that was asked for: the run has
   * already been played, and the route it wrote is a real route however it was
   * played. Which category it landed in is the caller's to report.
   *
   * The times come with it because the recording was also a run of the route it
   * was writing - the same play, timed the same way - so there is no reason for
   * the route to arrive with nothing on it. It arrives with a best run and a
   * gold on every split, and the next run of it is measured against the run
   * that made it rather than against nothing at all.
   */
  takeRecording(name: string): { route: Route; record: RouteRecord } | null {
    if (this.#recorded.length === 0) return null;

    const id = `rec-${Date.now().toString(36)}`;
    const route: Route = {
      id,
      category: placeIn(this.#category, this.#recorded).id,
      name,
      splits: this.#recorded,
    };
    // Read before the splits are let go of: #run() is written against whatever
    // the timer is running, which for a recording is the list below.
    const record = withRun(emptyRecord(id), this.#run(), groupSplits(this.#splits));

    this.#recorded = [];
    return { route, record };
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
    this.#groupGolds = this.#record.bestGroups;
    this.#state = "running";
    this.#origin = performance.now();
    this.#elapsed = 0;
    this.#closed = [];
    this.#at = 0;
    this.#recordingRun = this.#recording;
    this.#completedRecording = false;

    if (this.#recordingRun) this.#recorded = [];

    this.#tick();
  }

  #stop(state: "finished" | "abandoned"): void {
    this.#state = state;

    if (this.#frame !== null) cancelAnimationFrame(this.#frame);
    this.#frame = null;

    // A recorded run sets no time on the route that was selected - it was not
    // run over that route - so nothing is written here. Its own times go to the
    // route it just wrote, which does not have an id until takeRecording() has
    // been asked for one.
    if (!this.#recordingRun) {
      this.#record = withRun(
        this.#record,
        this.#run(),
        groupSplits(this.#splits),
      );
    }

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

  /** One more split on the route being written, for what the game just did. */
  #appendRecorded(event: GameEvent): void {
    // Beating Bowser is the run's last split. The game does not report it as a
    // completed level - there is no walk back to the map, the ending takes over
    // from the frame Bowser falls - so it arrives as run-ended, and the split
    // is closed on that. Then the recording stops, because a route that ran to
    // the end of the game has nothing after it. An abandoned run never gets
    // here: run-abandoned stops it from handle().
    if (event.kind === "run-ended") {
      const last = timedSplits(this.#recorded).length + 1;

      this.#recorded.push({
        id: "run-ended",
        name: `Split ${last}`,
        on: { kind: "run-ended" },
      });
      this.#close(last - 1);
      this.#stop("finished");
      return;
    }

    // A warp is put on the route but is not a split of the run. It is the
    // evidence a warpless category is judged against - a warp into the next
    // world is invisible in the world numbers - so it has to be written down,
    // but it takes no time that can be told apart from the level it happens
    // during and it is not something the player runs or reads. So it goes onto
    // #recorded, where takeRecording() will find it, and nothing else: no row,
    // no name worth keeping, and no close, so its time falls into the next
    // split's segment. A recording never ends on one either.
    if (event.kind === "warp-taken") {
      this.#recorded.push({
        id: `warp${this.#recorded.length}-w${event.world}`,
        name: "Warp",
        on: { kind: "warp-taken", world: event.world },
      });
      return;
    }

    if (event.kind !== "level-completed") return;

    // Numbered rather than named: nothing here knows what the level is called,
    // and a guess at it would be a name the player has to correct rather than
    // one they can accept. Numbered by the timed splits so far, warps aside, so
    // the count matches the rows the panel shows. The routes section is where
    // these get their names; the id is what a saved time is tied to, so
    // renaming costs nothing.
    const number = timedSplits(this.#recorded).length + 1;
    const split: RouteSplit = {
      id: `w${event.world}-l${event.level}`,
      name: `Split ${number}`,
      on: { kind: "level-completed", world: event.world, level: event.level },
    };

    this.#recorded.push(split);
    this.#close(number - 1);

    // Some categories know their own end. World 1 is over when world 1's castle
    // is, and a recording that ran on past it would be writing a route for a
    // category that does not exist - so the recording stops itself there rather
    // than waiting for the player to stop it or for the game to end.
    if (this.category.complete(this.#recorded)) {
      this.#completedRecording = true;
      this.#stop("finished");
    }
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

    // The bests the sum of best is added up from, with this run folded in as it
    // goes: a segment that has just beaten its best belongs to the sum from the
    // moment it closes rather than from whenever the record is written, which
    // is what makes the figure in the panel move during the run that earns it.
    const liveBest = new Map(this.#golds);

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

      if (segment !== null) {
        const was = liveBest.get(split.id);

        liveBest.set(
          split.id,
          was === undefined ? segment : shorter(was, segment),
        );
      }

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

    // The same splits, cut into the world each is played in. Recomputed here
    // rather than kept: it is derived from the route, and the route being
    // recorded grows a split at a time.
    const routeSplits = this.#splits;
    const nested = entersMultipleWorlds(routeSplits);
    const groups = groupSplits(routeSplits).map((group): GroupView => {
      const endMs = this.#closed[group.to] ?? null;
      const at = endMs === null ? null : duration(endMs);

      // The world starts on the clock the last split before it left it at, or at
      // zero if it opens the run. A split skipped past leaves no time, so this
      // walks back to the last one that actually closed.
      let startMs = 0;
      for (let i = group.from - 1; i >= 0; i--) {
        const ms = this.#closed[i];
        if (ms != null) {
          startMs = ms;
          break;
        }
      }

      const wholeSkipped = group.to < this.#at && at === null;
      const endSplit = routeSplits[group.to];
      const pbAt =
        pb === null || endSplit === undefined ? null : timeAt(pb, endSplit.id);
      const segment = at === null ? null : at.subtract(duration(startMs));
      // Nothing to compare a world against while its route is being written by
      // this very run, the same reason the split segments hold back their delta.
      const groupBest = recording ? undefined : this.#groupGolds.get(group.id);

      return {
        name: group.name,
        world: group.world,
        from: group.from,
        to: group.to,
        state: wholeSkipped
          ? "skipped"
          : at !== null
            ? "closed"
            : running && this.#at >= group.from && this.#at <= group.to
              ? "current"
              : "ahead",
        at,
        delta: at === null || pbAt === null ? null : at.subtract(pbAt),
        segment,
        segmentDelta:
          segment === null || groupBest === undefined
            ? null
            : segment.subtract(groupBest),
        gold:
          !recording &&
          segment !== null &&
          (groupBest === undefined ||
            Temporal.Duration.compare(segment, groupBest) < 0),
      };
    });

    return {
      category: this.category,
      route: this.#route,
      state: this.#state,
      recording,
      elapsed: duration(this.#elapsed),
      splits,
      groups,
      nested,
      pace: this.#pace(),
      pb: recording ? null : (this.#record.pb?.total ?? null),
      sumOfBest:
        recording || this.#route === null
          ? null
          : sumOfBest(this.#route, liveBest),
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
