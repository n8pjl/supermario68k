// What a route has been run in, laid out to be read.
//
// The panel beside the screen answers a question about the run happening now:
// where it stands, split by split, against the best. Everything it compares
// against is already recorded - the best run itself, the best each split has
// ever been on its own, the best each world has been run in - and until now none
// of it could be looked at except by running the route again and watching the
// deltas go by. This is that record laid out flat: one table of splits, one of
// worlds where the route crosses more than one, and the two sums under them.
//
// Read-only, and drawn from the same record the timer compares against, so
// there is no second idea here of what a time is worth. It is folded away by
// default and shown when it is asked for: it is what is looked at between runs,
// and the routes section around it is what is done between them.

import { entersMultipleWorlds, groupSplits } from "./groups.ts";
import {
  type RouteRecord,
  groupSegments,
  segments,
  sumOfBest,
  sumOfGroupBest,
  sumOfSplitBest,
  timeAt,
} from "./records.ts";
import { type Route, timedSplits } from "./route.ts";
import { formatDay, formatDuration } from "./times.ts";

/** Nothing was recorded here, which is not the same as a time of zero. */
const NOTHING = "—";

function element<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  className = "",
  text = "",
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);

  el.className = className;
  el.textContent = text;
  return el;
}

function time(d: Temporal.Duration | null): string {
  return d === null ? NOTHING : formatDuration(d);
}

/** A row of a table: a name, then whatever figures the table is about. */
function row(
  name: string,
  times: readonly (Temporal.Duration | null)[],
): HTMLTableRowElement {
  const tr = document.createElement("tr");
  const head = element("th", "sr-sheet-name", name);

  head.scope = "row";
  tr.append(head, ...times.map((d) => element("td", "sr-sheet-time", time(d))));
  return tr;
}

/**
 * One table, built empty: a caption, the four headings, and a body to fill.
 *
 * The same four columns whichever of the two tables it is, because they are the
 * same four questions one level apart - what the best run had on the clock
 * there, how long that stretch took it, and the best that stretch has ever
 * been, whoever's run it was. `more` are headings for columns after those,
 * which only one of the tables has a question for.
 */
function table(caption: string, first: string, more: readonly string[] = []): {
  root: HTMLElement;
  body: HTMLTableSectionElement;
} {
  // The table inside a box of its own, which is what is hidden and what scrolls:
  // four columns of times do not fit across a phone, and a table that overflows
  // the section would take the page sideways with it.
  const root = element("div", "sr-sheet-scroll");
  const grid = element("table", "sr-sheet-table");
  const head = document.createElement("thead");
  const tr = document.createElement("tr");

  grid.append(element("caption", "", caption));

  for (const label of [
    first,
    "Best run",
    "Segment",
    "Best segment",
    ...more,
  ]) {
    const th = element("th", "", label);

    th.scope = "col";
    tr.append(th);
  }

  head.append(tr);

  const body = document.createElement("tbody");
  grid.append(head, body);
  root.append(grid);

  return { root, body };
}

export class SpeedrunSheet {
  readonly #root: HTMLElement;
  readonly #best: HTMLElement;
  readonly #sums: HTMLElement;
  readonly #empty: HTMLElement;
  readonly #splits: HTMLElement;
  readonly #splitRows: HTMLTableSectionElement;
  readonly #worlds: HTMLElement;
  readonly #worldRows: HTMLTableSectionElement;

  constructor(container: HTMLElement) {
    this.#root = container;
    this.#root.className = "sr-sheet";
    this.#root.hidden = true;

    this.#best = element("p", "sr-sheet-best");
    this.#sums = element("p", "sr-sheet-sums");
    this.#empty = element(
      "p",
      "sr-sheet-empty",
      "Nothing recorded on this route yet. Run it with the timer on and every " +
        "split you close leaves a time here.",
    );

    const splits = table("Splits", "Split");
    // A world has one figure a split does not: its levels each at their best,
    // added up. Not the same as its best segment, which is one run of the world
    // start to end, and the gap between the two is the time still to be found
    // in it - so the two are set side by side.
    const worlds = table("Worlds", "World", ["Sum of best"]);

    this.#splits = splits.root;
    this.#splitRows = splits.body;
    this.#worlds = worlds.root;
    this.#worldRows = worlds.body;

    container.replaceChildren(
      element("h3", "", "Times"),
      this.#best,
      this.#sums,
      this.#empty,
      this.#splits,
      this.#worlds,
    );
  }

  get shown(): boolean {
    return !this.#root.hidden;
  }

  set shown(shown: boolean) {
    this.#root.hidden = !shown;
  }

  /**
   * Lay out one route's record.
   *
   * Rows are rebuilt rather than reused: this is drawn when it is opened and
   * when a run has just changed what it says, not on every frame the way the
   * panel is, so there is nothing here worth the machinery that keeps the panel
   * off the layout path.
   */
  draw(route: Route | null, record: RouteRecord): void {
    if (route === null) {
      this.#splitRows.replaceChildren();
      this.#worldRows.replaceChildren();
      return;
    }

    const shown = timedSplits(route.splits);
    const groups = groupSplits(shown);
    const pb = record.pb;
    // The best run taken apart the way the record keeps the bests: cumulative
    // times are what it stores, and a segment is the stretch between two of
    // them, so both readings of it come from the one run.
    const pbSplits = pb === null ? null : segments(pb);
    const pbGroups = pb === null ? null : groupSegments(pb, groups);

    // A route that has been run but never finished has segments and no best
    // run; one that has been imported without its times has neither. The tables
    // say nothing in the second case, so the explanation stands in for them.
    const anything =
      pb !== null || record.best.size > 0 || record.bestGroups.size > 0;

    this.#empty.hidden = anything;
    this.#best.hidden = !anything;
    this.#splits.hidden = !anything;
    // Worlds only where there are worlds to tell apart, the same test the panel
    // makes before it nests: a single-world route's one world row is its total
    // said again.
    this.#worlds.hidden = !anything || !entersMultipleWorlds(shown);

    const day = pb === null ? null : formatDay(pb.when);

    this.#best.textContent =
      pb === null
        ? "No finished run yet."
        : day === null
          ? `Best run ${formatDuration(pb.total)}.`
          : `Best run ${formatDuration(pb.total)}, set ${day}.`;

    // Both sums, because they are two honest figures about the same route and
    // neither is the other rounded differently: see sumOfGroupBest() in
    // records.ts. Either can be missing - a sum with a segment never run is not
    // a time the route could be run in - and the line goes when both are.
    const sob = sumOfBest(route, record.best);
    const worldSob = this.#worlds.hidden
      ? null
      : sumOfGroupBest(groups, record.bestGroups);
    const sums: string[] = [];

    if (sob !== null) sums.push(`Sum of best ${formatDuration(sob)}`);
    if (worldSob !== null) {
      sums.push(`Sum of world best ${formatDuration(worldSob)}`);
    }

    this.#sums.textContent = sums.join(" · ");

    this.#splitRows.replaceChildren(
      ...shown.map((split) =>
        row(split.name, [
          pb === null ? null : timeAt(pb, split.id),
          pbSplits?.get(split.id) ?? null,
          record.best.get(split.id) ?? null,
        ]),
      ),
    );

    this.#worldRows.replaceChildren(
      ...groups.map((group) => {
        const last = shown[group.to];

        return row(group.name, [
          pb === null || last === undefined ? null : timeAt(pb, last.id),
          pbGroups?.get(group.id) ?? null,
          record.bestGroups.get(group.id) ?? null,
          sumOfSplitBest(shown.slice(group.from, group.to + 1), record.best),
        ]);
      }),
    );
  }
}
