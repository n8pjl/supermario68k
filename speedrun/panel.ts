// The panel beside the screen. Draws a TimerView and nothing else: every
// decision about what a run is worth was made before it got here.

import { type SplitView, type TimerView } from "./timer.ts";
import { formatDelta, formatDuration } from "./times.ts";

interface Row {
  readonly root: HTMLElement;
  readonly name: HTMLElement;
  readonly delta: HTMLElement;
  readonly time: HTMLElement;
  readonly segment: HTMLElement;
  readonly segmentDelta: HTMLElement;
}

function element<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  className: string,
  text = "",
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);

  el.className = className;
  el.textContent = text;
  return el;
}

function sign(el: HTMLElement, d: Temporal.Duration | null): void {
  if (d === null) {
    delete el.dataset["sign"];
  } else {
    el.dataset["sign"] = d.sign < 0 ? "ahead" : "behind";
  }
}

export class SpeedrunPanel {
  readonly #root: HTMLElement;
  readonly #title: HTMLElement;
  readonly #list: HTMLElement;
  readonly #empty: HTMLElement;
  readonly #pace: HTMLElement;
  readonly #clock: HTMLElement;
  readonly #best: HTMLElement;
  readonly #sob: HTMLElement;

  /** One row per split of the route last drawn, rebuilt when that changes. */
  #rows: Row[] = [];
  /** The row last scrolled to, so a route being run only scrolls when it moves on. */
  #following = -1;

  constructor(container: HTMLElement) {
    this.#root = container;
    // The category rather than the route: what the run is worth is a time under
    // a set of rules, and which of the routes to those rules is being taken is
    // the splits below rather than a name worth a line of its own. In immersive
    // mode this panel is the only place the rules are on screen at all.
    this.#title = element("h2", "sr-title");
    this.#list = element("ol", "sr-splits");
    this.#empty = element(
      "p",
      "sr-empty",
      "No route in this category yet. Record one below: play a game with " +
        "recording on and every level you beat becomes a split.",
    );
    this.#pace = element("span", "sr-pace");
    this.#clock = element("div", "sr-clock");
    this.#best = element("div", "sr-best");
    this.#sob = element("div", "sr-sob");

    const foot = element("div", "sr-foot");
    foot.append(this.#pace, this.#clock, this.#best, this.#sob);

    container.replaceChildren(this.#title, this.#list, this.#empty, foot);
  }

  // A recorded route grows a split at a time, so the row count is checked on
  // every draw rather than only when the route is swapped.
  #fit(count: number): void {
    if (this.#rows.length === count) return;

    this.#rows = Array.from({ length: count }, () => {
      const root = element("li", "sr-split");
      const name = element("span", "sr-name");
      const delta = element("span", "sr-delta");
      const time = element("span", "sr-time");
      // A second line under the first, holding the same two columns: the
      // panel is narrower than a name and four figures in a row, and the pair
      // above and the pair below are answers to different questions - where
      // the run stands overall, and how the split just played went on its own.
      const segmentDelta = element("span", "sr-segment-delta");
      const segment = element("span", "sr-segment");

      root.append(name, delta, time, segmentDelta, segment);
      return { root, name, delta, time, segment, segmentDelta };
    });

    this.#list.replaceChildren(...this.#rows.map((row) => row.root));
    this.#following = -1;
  }

  /**
   * Keep the split being run inside the scrolled list.
   *
   * The list scrolls rather than the page: a route long enough to scroll is
   * still only part of what is on screen, and a page that jumped every time a
   * split closed would be unplayable. Nothing is moved until the split does, so
   * the list can be scrolled back to read an earlier one mid-run.
   */
  #follow(at: number): void {
    const row = this.#rows[at]?.root;

    if (row === undefined || at === this.#following) return;
    this.#following = at;

    const top = row.offsetTop;
    const bottom = top + row.offsetHeight;
    const list = this.#list;

    if (top < list.scrollTop) {
      list.scrollTop = top;
    } else if (bottom > list.scrollTop + list.clientHeight) {
      list.scrollTop = bottom - list.clientHeight;
    }
  }

  #drawSplit(row: Row, split: SplitView): void {
    row.root.dataset["state"] = split.state;
    // Marked on the row rather than on the time, so the whole line reads as the
    // best that split has been - which is the thing being claimed.
    row.root.dataset["gold"] = String(split.gold);
    row.name.textContent = split.name;

    row.time.textContent =
      split.at !== null
        ? formatDuration(split.at)
        : split.state === "skipped"
          ? "—"
          : "";

    // Blank where the time cannot be attributed to this split alone - it is
    // still ahead, or it was skipped past, or it covers splits that were - and
    // blank again where there is a segment but the best has none to set it
    // against, which is every split of a first run.
    row.segment.textContent =
      split.segment === null ? "" : formatDuration(split.segment);

    // Left the colour of the panel rather than marked ahead or behind: two
    // coloured deltas on one row read as two verdicts on the run, and only the
    // one above is that. This is an aside about the split just played, and it
    // says which way it went with its own sign.
    row.segmentDelta.textContent =
      split.segmentDelta === null ? "" : formatDelta(split.segmentDelta);

    row.delta.textContent = split.delta === null ? "" : formatDelta(split.delta);
    sign(row.delta, split.delta);
  }

  draw(view: TimerView): void {
    this.#root.dataset["state"] = view.state;
    this.#root.dataset["recording"] = String(view.recording);

    this.#title.textContent = view.recording
      ? `Recording ${view.category.name}`
      : view.category.name;

    // Only worth saying before anything has been recorded in this category:
    // once a recording is running its splits are what the panel is showing.
    this.#empty.hidden = view.recording || view.route !== null;

    this.#fit(view.splits.length);
    view.splits.forEach((split, i) => {
      const row = this.#rows[i];
      if (row !== undefined) this.#drawSplit(row, split);
    });

    // A recording has no split to be at - every one of them is closed as it is
    // written - so it follows the end of the list instead.
    const at = view.splits.findIndex((split) => split.state === "current");
    this.#follow(at < 0 ? view.splits.length - 1 : at);

    this.#clock.textContent = formatDuration(view.elapsed);
    this.#pace.textContent = view.pace === null ? "" : formatDelta(view.pace);
    sign(this.#pace, view.pace);

    // Nothing at all while recording: a route being written has no earlier run
    // to have been slower than, and "no finished run yet" would read as one.
    this.#best.textContent = view.recording
      ? ""
      : view.pb === null
        ? "No finished run yet"
        : `Best ${formatDuration(view.pb)}`;
    this.#sob.textContent =
      view.sumOfBest === null
        ? ""
        : `Sum of best ${formatDuration(view.sumOfBest)}`;
  }
}
