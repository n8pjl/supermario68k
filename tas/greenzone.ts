// The snapshots kept of the movie as it stands: what a seek starts from. Named
// after the same thing in other TAS tools, where the frames that can be
// reached without replaying from the start are drawn green.
//
// A snapshot at frame n is the result of the movie's first n inputs, so it
// stays good exactly as long as none of those change: an edit at frame f drops
// every snapshot after f, and nothing else ever has to.
//
// Replaying is fast - tens of thousands of frames a second with nothing drawn
// - so snapshots only need to be close together, not on every frame. They are
// taken every KEYFRAME_INTERVAL frames as the movie is played, and once they
// take up more than the budget, the ones that leave the smallest hole behind
// them go first, which keeps whatever is left evenly spread over the movie.

import type { Snapshot } from "./game.ts";

export const KEYFRAME_INTERVAL = 60;

const DEFAULT_BUDGET = 192 * 1024 * 1024;

function size(snapshot: Snapshot): number {
  return snapshot.bytes.byteLength + snapshot.pages.byteLength;
}

export class Greenzone {
  readonly #snapshots = new Map<number, Snapshot>();
  #pinned = new Set<number>();
  #bytes = 0;

  constructor(readonly budget = DEFAULT_BUDGET) {}

  get bytes(): number {
    return this.#bytes;
  }

  has(frame: number): boolean {
    return this.#snapshots.has(frame);
  }

  /** The frames there are snapshots of, ascending. */
  frames(): number[] {
    return [...this.#snapshots.keys()].sort((a, b) => a - b);
  }

  /** The latest snapshot at or before a frame. */
  latest(frame: number): Snapshot | undefined {
    let best: Snapshot | undefined;
    for (const snapshot of this.#snapshots.values()) {
      if (snapshot.frame <= frame && (!best || snapshot.frame > best.frame)) {
        best = snapshot;
      }
    }
    return best;
  }

  add(snapshot: Snapshot): void {
    const old = this.#snapshots.get(snapshot.frame);
    if (old) this.#bytes -= size(old);

    this.#snapshots.set(snapshot.frame, snapshot);
    this.#bytes += size(snapshot);
    this.#trim();
  }

  /** Drops every snapshot after a frame whose input has changed. */
  invalidateAfter(frame: number): void {
    for (const [at, snapshot] of this.#snapshots) {
      if (at > frame) {
        this.#snapshots.delete(at);
        this.#bytes -= size(snapshot);
      }
    }
  }

  /**
   * The frames whose snapshots are never thinned out: the bookmarks, and
   * power-on, which is the one snapshot a seek can always fall back on.
   */
  pin(frames: Iterable<number>): void {
    this.#pinned = new Set(frames);
    this.#pinned.add(0);
  }

  #trim(): void {
    while (this.#bytes > this.budget) {
      const frames = this.frames();
      let victim = -1;
      let smallest = Infinity;

      for (let i = 1; i < frames.length - 1; i++) {
        const frame = frames[i]!;
        if (this.#pinned.has(frame) || frame === 0) continue;

        const hole = frames[i + 1]! - frames[i - 1]!;
        if (hole < smallest) {
          smallest = hole;
          victim = frame;
        }
      }

      // Everything left is pinned, or the newest: over budget it stays.
      if (victim < 0) return;

      this.#bytes -= size(this.#snapshots.get(victim)!);
      this.#snapshots.delete(victim);
    }
  }
}
