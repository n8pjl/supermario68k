// The panel beside the screen. Draws a TimerView and nothing else: every
// decision about what a run is worth was made before it got here.

import { type SplitView, type TimerView } from "./timer.ts";
import { formatDelta, formatDuration } from "./times.ts";

interface Row {
  readonly root: HTMLElement;
  readonly name: HTMLElement;
  readonly delta: HTMLElement;
  readonly time: HTMLElement;
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
  readonly #pace: HTMLElement;
  readonly #clock: HTMLElement;
  readonly #best: HTMLElement;
  readonly #sob: HTMLElement;

  /** One row per split of the route last drawn, rebuilt when that changes. */
  #rows: Row[] = [];

  constructor(container: HTMLElement) {
    this.#root = container;
    this.#title = element("h2", "sr-title");
    this.#list = element("ol", "sr-splits");
    this.#pace = element("span", "sr-pace");
    this.#clock = element("div", "sr-clock");
    this.#best = element("div", "sr-best");
    this.#sob = element("div", "sr-sob");

    const foot = element("div", "sr-foot");
    foot.append(this.#pace, this.#clock, this.#best, this.#sob);

    container.replaceChildren(this.#title, this.#list, foot);
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

      root.append(name, delta, time);
      return { root, name, delta, time };
    });

    this.#list.replaceChildren(...this.#rows.map((row) => row.root));
  }

  #drawSplit(row: Row, split: SplitView): void {
    row.root.dataset["state"] = split.state;
    // Marked on the row rather than on the time, so the whole line reads as the
    // best that split has been - which is the thing being claimed.
    row.root.dataset["gold"] = String(split.gold);
    row.name.textContent = split.name;

    const segment = split.segment;
    row.time.title =
      segment === null ? "" : `Segment ${formatDuration(segment)}`;
    row.time.textContent =
      split.at !== null
        ? formatDuration(split.at)
        : split.state === "skipped"
          ? "—"
          : "";

    row.delta.textContent = split.delta === null ? "" : formatDelta(split.delta);
    sign(row.delta, split.delta);
  }

  draw(view: TimerView): void {
    this.#root.dataset["state"] = view.state;
    this.#root.dataset["recording"] = String(view.recording);

    this.#title.textContent = view.recording
      ? "Recording a route"
      : view.route.name;

    this.#fit(view.splits.length);
    view.splits.forEach((split, i) => {
      const row = this.#rows[i];
      if (row !== undefined) this.#drawSplit(row, split);
    });

    this.#clock.textContent = formatDuration(view.elapsed);
    this.#pace.textContent = view.pace === null ? "" : formatDelta(view.pace);
    sign(this.#pace, view.pace);

    this.#best.textContent =
      view.pb === null ? "No finished run yet" : `Best ${formatDuration(view.pb)}`;
    this.#sob.textContent =
      view.sumOfBest === null
        ? ""
        : `Sum of best ${formatDuration(view.sumOfBest)}`;
  }
}
