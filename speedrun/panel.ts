// The panel beside the screen. Draws a TimerView and nothing else: every
// decision about what a run is worth was made before it got here.

import { type GroupView, type SplitView, type TimerView } from "./timer.ts";
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

/**
 * Write text only where it is not already what is being written.
 *
 * Assigning textContent replaces the text node whether or not the string is the
 * same one, and a replaced text node dirties layout for the whole list. The
 * panel is drawn on every animation frame, so every field of every row was
 * being dirtied sixty times a second to show a clock that is the only thing
 * moving. The comparison is what keeps the rest of the panel off the layout
 * path between splits.
 */
function text(el: HTMLElement, value: string): void {
  if (el.textContent !== value) el.textContent = value;
}

/** The same, for a data attribute: setting one restyles even when it matches. */
function mark(el: HTMLElement, name: string, value: string): void {
  if (el.dataset[name] !== value) el.dataset[name] = value;
}

function sign(el: HTMLElement, d: Temporal.Duration | null): void {
  if (d === null) {
    if (el.dataset["sign"] !== undefined) delete el.dataset["sign"];
  } else {
    mark(el, "sign", d.sign < 0 ? "ahead" : "behind");
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
  /** One row per world group, when the route is nested; empty when it is flat. */
  #groupRows: Row[] = [];
  /** The shape last laid out - which rows, and where the groups fall. */
  #signature = "";
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

  #makeRow(className: string): Row {
    const root = element("li", className);
    const name = element("span", "sr-name");
    const delta = element("span", "sr-delta");
    const time = element("span", "sr-time");
    // A second line under the first, holding the same two columns: the panel is
    // narrower than a name and four figures in a row, and the pair above and the
    // pair below are answers to different questions - where the run stands
    // overall, and how the split (or world) just played went on its own.
    const segmentDelta = element("span", "sr-segment-delta");
    const segment = element("span", "sr-segment");

    root.append(name, delta, time, segmentDelta, segment);
    return { root, name, delta, time, segment, segmentDelta };
  }

  // A recorded route grows a split at a time, and switching route or crossing
  // into a second world changes the shape, so the layout is checked on every
  // draw rather than only when the route is swapped. Rebuilt only when the
  // shape actually moved, so a route being run is not torn down each frame.
  #layout(view: TimerView): void {
    // Whether the run is being read a world at a time, which the timer decides:
    // the pace and the sum of best in the footer are read off the same rows, so
    // there is one answer to which rows those are and it is not the panel's.
    const worlds = view.worlds;
    const signature =
      (worlds ? "w:" : "s:") +
      (view.nested
        ? `n:${view.groups.map((g) => `${g.from}-${g.to}`).join(",")}`
        : `f:${view.splits.length}`);

    if (signature === this.#signature) return;
    this.#signature = signature;
    this.#following = -1;

    // No split rows at all rather than hidden ones: they are what the option
    // was turned on to be rid of, and a row that is not in the list is one
    // nothing has to draw or measure.
    this.#rows = worlds ? [] : view.splits.map(() => this.#makeRow("sr-split"));

    if (worlds) {
      this.#groupRows = view.groups.map(() => this.#makeRow("sr-group"));
      this.#list.replaceChildren(...this.#groupRows.map((row) => row.root));
      return;
    }

    if (!view.nested) {
      this.#groupRows = [];
      this.#list.replaceChildren(...this.#rows.map((row) => row.root));
      return;
    }

    // A header per world, then that world's split rows under it, indented by the
    // data attribute so the CSS can step them in without a second row class.
    this.#groupRows = view.groups.map(() => this.#makeRow("sr-group"));

    const children: HTMLElement[] = [];
    view.groups.forEach((group, g) => {
      const header = this.#groupRows[g];
      if (header !== undefined) children.push(header.root);

      for (let i = group.from; i <= group.to; i++) {
        const row = this.#rows[i];
        if (row === undefined) continue;

        row.root.dataset["nested"] = "true";
        children.push(row.root);
      }
    });

    this.#list.replaceChildren(...children);
  }

  /**
   * Keep the split being run inside the scrolled list.
   *
   * The list scrolls rather than the page: a route long enough to scroll is
   * still only part of what is on screen, and a page that jumped every time a
   * split closed would be unplayable. Nothing is moved until the split does, so
   * the list can be scrolled back to read an earlier one mid-run.
   */
  #follow(rows: readonly Row[], at: number): void {
    const row = rows[at]?.root;

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
    mark(row.root, "state", split.state);
    // Marked on the row rather than on the time, so the whole line reads as the
    // best that split has been - which is the thing being claimed.
    mark(row.root, "gold", String(split.gold));
    text(row.name, split.name);

    text(
      row.time,
      split.at !== null
        ? formatDuration(split.at)
        : split.state === "skipped"
          ? "—"
          : "",
    );

    // Blank where the time cannot be attributed to this split alone - it is
    // still ahead, or it was skipped past, or it covers splits that were - and
    // blank again where there is a segment but the best has none to set it
    // against, which is every split of a first run.
    text(
      row.segment,
      split.segment === null ? "" : formatDuration(split.segment),
    );

    // Left the colour of the panel rather than marked ahead or behind: two
    // coloured deltas on one row read as two verdicts on the run, and only the
    // one above is that. This is an aside about the split just played, and it
    // says which way it went with its own sign.
    text(
      row.segmentDelta,
      split.segmentDelta === null ? "" : formatDelta(split.segmentDelta),
    );

    text(row.delta, split.delta === null ? "" : formatDelta(split.delta));
    sign(row.delta, split.delta);
  }

  // The same two lines as a split row, for a whole world: its running total and
  // where that stands against the best, and under them the world start to end
  // and that against the best the world has been run in. A gold here is on the
  // world segment, the same claim a split gold makes one level down.
  #drawGroup(row: Row, group: GroupView): void {
    mark(row.root, "state", group.state);
    mark(row.root, "gold", String(group.gold));
    text(row.name, group.name);

    text(
      row.time,
      group.at !== null
        ? formatDuration(group.at)
        : group.state === "skipped"
          ? "—"
          : "",
    );

    text(
      row.segment,
      group.segment === null ? "" : formatDuration(group.segment),
    );
    text(
      row.segmentDelta,
      group.segmentDelta === null ? "" : formatDelta(group.segmentDelta),
    );

    text(row.delta, group.delta === null ? "" : formatDelta(group.delta));
    sign(row.delta, group.delta);
  }

  draw(view: TimerView): void {
    // Guarded like every other write below: these two sit on the panel root, so
    // setting them restyles everything under it.
    mark(this.#root, "state", view.state);
    mark(this.#root, "recording", String(view.recording));

    text(
      this.#title,
      view.recording ? `Recording ${view.category.name}` : view.category.name,
    );

    // Only worth saying before anything has been recorded in this category:
    // once a recording is running its splits are what the panel is showing.
    const empty = view.recording || view.route !== null;
    if (this.#empty.hidden !== empty) this.#empty.hidden = empty;

    this.#layout(view);
    view.splits.forEach((split, i) => {
      const row = this.#rows[i];
      if (row !== undefined) this.#drawSplit(row, split);
    });
    if (view.nested) {
      view.groups.forEach((group, g) => {
        const row = this.#groupRows[g];
        if (row !== undefined) this.#drawGroup(row, group);
      });
    }

    // A recording has no split to be at - every one of them is closed as it is
    // written - so it follows the end of the list instead.
    const current = view.splits.findIndex((split) => split.state === "current");
    const at = current < 0 ? view.splits.length - 1 : current;

    if (view.worlds) {
      // The world that split is in, because it is the row that is on screen.
      this.#follow(
        this.#groupRows,
        view.groups.findIndex((group) => at >= group.from && at <= group.to),
      );
    } else {
      this.#follow(this.#rows, at);
    }

    text(this.#clock, formatDuration(view.elapsed));
    text(this.#pace, view.pace === null ? "" : formatDelta(view.pace));
    sign(this.#pace, view.pace);

    // Nothing at all while recording: a route being written has no earlier run
    // to have been slower than, and "no finished run yet" would read as one.
    text(
      this.#best,
      view.recording
        ? ""
        : view.pb === null
          ? "No finished run yet"
          : `Best ${formatDuration(view.pb)}`,
    );
    text(
      this.#sob,
      view.sumOfBest === null
        ? ""
        : `Sum of best ${formatDuration(view.sumOfBest)}`,
    );
  }
}
