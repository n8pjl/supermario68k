// The data page: the run history, read back and added up.
//
// Everything here is read from the attempts history.ts keeps in this browser,
// and from any exported history files opened on the page - so a history from
// another machine can be read without importing anything into this one.
// Nothing is written: the page is a reading of the history, and the history
// belongs to the game.
//
// The filters scope everything under them. Stages are the heart of it - how
// often each is cleared, how fast, what it costs in deaths, and how that
// changes with what the player walked in carrying, which is the question an
// item route is made of.

import { CATEGORIES, category, isCategoryId } from "../speedrun/category.ts";
import { type GameEvent } from "../speedrun/events.ts";
import { readHistory } from "../speedrun/history.ts";
import { SpeedrunStore } from "../speedrun/store.ts";
import { type Attempt, parseAttempt, parseLines, span } from "./attempts.ts";
import {
  type Strip,
  barChart,
  clock,
  progressChart,
  stripChart,
} from "./charts.ts";
import {
  POWER_ORDER,
  type PlaceStats,
  type Visit,
  levelPlace,
  loadoutLabel,
  median,
  monsterPlace,
  placeName,
  statsByPlace,
  stoppedAt,
  visitsOf,
} from "./visits.ts";

function $<T extends HTMLElement>(id: string): T {
  return document.getElementById(id) as T;
}

function element<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  className = "",
  content: string | Node | (string | Node)[] = [],
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);

  if (className !== "") el.className = className;
  el.append(...(Array.isArray(content) ? content : [content]));
  return el;
}

function option(value: string, label: string): HTMLOptionElement {
  const el = document.createElement("option");

  el.value = value;
  el.textContent = label;
  return el;
}

function percent(part: number, whole: number): string {
  return whole === 0 ? "–" : `${Math.round((part / whole) * 100)}%`;
}

function orDash(ms: number | null): string {
  return ms === null ? "–" : clock(ms);
}

/** A length of time played, in words rather than on a clock. */
function played(ms: number): string {
  const minutes = Math.round(ms / 60_000);

  return minutes < 60
    ? `${minutes} min`
    : `${Math.trunc(minutes / 60)} h ${minutes % 60} min`;
}

function day(attempt: Attempt): string {
  return new Date(attempt.started.epochMilliseconds).toLocaleString(undefined, {
    dateStyle: "medium",
    timeStyle: "short",
  });
}

/** Route ids as the player named them, where this browser knows the route. */
const store = new SpeedrunStore();

function routeName(id: string | null): string {
  if (id === null) return "No route";
  return store.find(id)?.name ?? id;
}

function describe(event: GameEvent): string {
  const where = (e: { world: number; level?: number | undefined; monster?: number | undefined }) =>
    placeName(
      e.monster !== undefined
        ? monsterPlace(e.world, e.monster)
        : levelPlace(e.world, e.level ?? 0),
    );

  switch (event.kind) {
    case "run-started":
      return "New game";
    case "run-abandoned":
      return "Back to the menu";
    case "run-ended":
      return "The end";
    case "world-entered":
      return `Into world ${event.world + 1}`;
    case "warp-taken":
      return `Warp to world ${event.world + 1}`;
    case "level-entered":
    case "monster-fought":
      return `Entered ${where(event)} as ${loadoutLabel(event.player)}${
        event.player.items.length > 0 ? ` (items: ${event.player.items.join(", ")})` : ""
      }`;
    case "level-completed":
    case "monster-defeated":
      return `Cleared ${where(event)}`;
    case "player-hit":
      return `Hit in ${where(event)}, lost ${event.player.power}`;
    case "player-died":
      return `Died in ${where(event)} as ${event.player.power}`;
  }
}

// ---------------------------------------------------------------------------
// State

interface Filters {
  mode: "all" | "run" | "practice";
  category: string;
  route: string;
  days: number;
}

let local: Attempt[] = [];
const opened = new Map<string, Attempt>();
const filters: Filters = { mode: "all", category: "", route: "", days: 0 };
let selectedPlace: string | null = null;
let sort: { column: number; descending: boolean } = { column: 0, descending: false };
let shown = 50;

/** Every attempt the page has, this browser's and the opened files', by id. */
function everything(): Attempt[] {
  const all = new Map(local.map((a) => [a.id, a]));

  for (const [id, attempt] of opened) all.set(id, attempt);
  return [...all.values()].sort(
    (a, b) => a.started.epochMilliseconds - b.started.epochMilliseconds,
  );
}

function filtered(all: readonly Attempt[]): Attempt[] {
  const since = filters.days === 0 ? -Infinity : Date.now() - filters.days * 86_400_000;

  return all.filter(
    (a) =>
      (filters.mode === "all" || a.mode === filters.mode) &&
      (filters.category === "" || a.category === filters.category) &&
      (filters.route === "" || a.route === filters.route) &&
      a.started.epochMilliseconds >= since,
  );
}

// ---------------------------------------------------------------------------
// Drawing

function tile(label: string, value: string, note = ""): HTMLElement {
  return element("div", "tile", [
    element("span", "tile-label", label),
    element("strong", "tile-value", value),
    ...(note === "" ? [] : [element("span", "tile-note", note)]),
  ]);
}

function drawTiles(attempts: readonly Attempt[], visits: readonly Visit[]): void {
  const runs = attempts.filter((a) => a.mode === "run");
  const finished = runs.filter((a) => a.finished && a.total !== null);
  const best =
    filters.category === "" || finished.length === 0
      ? null
      : Math.min(...finished.map((a) => a.total!));

  $("tiles").replaceChildren(
    tile(
      "Attempts",
      String(attempts.length),
      `${runs.length} timed, ${attempts.length - runs.length} practice`,
    ),
    tile("Finished runs", String(finished.length), percent(finished.length, runs.length) + " of runs"),
    tile(
      "Best finish",
      best === null ? "–" : clock(best),
      isCategoryId(filters.category) ? category(filters.category).name : "Pick a category",
    ),
    tile("Played", played(attempts.reduce((sum, a) => sum + span(a), 0))),
    tile("Deaths", String(visits.filter((v) => v.outcome === "died").length)),
    tile("Hits taken", String(visits.reduce((sum, v) => sum + v.hits, 0))),
  );
}

function drawProgress(attempts: readonly Attempt[]): void {
  const host = $("progress-chart");
  const note = $("progress-note");
  const legend = $("progress-legend");
  const finished = attempts.filter(
    (a) => a.mode === "run" && a.finished && a.total !== null,
  );

  legend.hidden = true;
  if (filters.category === "") {
    host.replaceChildren();
    note.textContent =
      "Times from different categories are not the same race: pick a category to see them.";
    return;
  }
  if (finished.length === 0) {
    host.replaceChildren();
    note.textContent = "No finished runs here yet.";
    return;
  }

  note.textContent = "";
  legend.hidden = false;
  progressChart(
    host,
    finished.map((a) => ({
      when: a.started.epochMilliseconds,
      value: a.total!,
      lines: [clock(a.total!), day(a), routeName(a.route)],
    })),
  );
}

function drawStops(attempts: readonly Attempt[]): void {
  const host = $("stops-chart");
  const note = $("stops-note");
  const counts = new Map<string, { label: string; died: number; left: number }>();

  for (const attempt of attempts) {
    if (attempt.mode !== "run" || attempt.finished) continue;

    const last = stoppedAt(attempt, visitsOf(attempt));
    const key = last?.place.key ?? "map";
    const entry = counts.get(key) ?? {
      label: last === null ? "On the map" : placeName(last.place),
      died: 0,
      left: 0,
    };

    if (last?.outcome === "died") entry.died++;
    else entry.left++;
    counts.set(key, entry);
  }

  const bars = [...counts.values()]
    .map((c) => ({
      label: c.label,
      value: c.died + c.left,
      lines: [
        `${c.died + c.left} run(s)`,
        c.label,
        `${c.died} on a death, ${c.left} reset or closed`,
      ],
    }))
    .sort((a, b) => b.value - a.value)
    .slice(0, 12);

  if (bars.length === 0) {
    host.replaceChildren();
    note.textContent = "No unfinished runs here.";
    return;
  }

  note.textContent = "The stage each unfinished run was last in. Top 12.";
  barChart(host, bars, "Where unfinished runs stopped");
}

const COLUMNS: readonly {
  label: string;
  value: (s: PlaceStats) => number | string | null;
  show: (s: PlaceStats) => string;
}[] = [
  { label: "Stage", value: () => null, show: (s) => placeName(s.place) },
  { label: "Entered", value: (s) => s.entered, show: (s) => String(s.entered) },
  { label: "Cleared", value: (s) => s.cleared, show: (s) => String(s.cleared) },
  {
    label: "Clear rate",
    value: (s) => s.cleared / s.entered,
    show: (s) => percent(s.cleared, s.entered),
  },
  { label: "Deaths", value: (s) => s.died, show: (s) => String(s.died) },
  { label: "Hits", value: (s) => s.hits, show: (s) => String(s.hits) },
  { label: "Best", value: (s) => s.clears[0] ?? null, show: (s) => orDash(s.clears[0] ?? null) },
  { label: "Median", value: (s) => median(s.clears), show: (s) => orDash(median(s.clears)) },
  { label: "Lost to deaths", value: (s) => s.lost, show: (s) => (s.died === 0 ? "–" : clock(s.lost)) },
];

function drawStages(stats: readonly PlaceStats[]): void {
  const table = $("stages-table");
  const note = $("stages-note");

  note.textContent =
    stats.length === 0
      ? "Nothing entered yet."
      : "Select a row to break it down.";

  const rows = [...stats];
  if (sort.column !== 0) {
    const column = COLUMNS[sort.column]!;
    rows.sort((a, b) => {
      const x = column.value(a);
      const y = column.value(b);

      if (x === null) return 1;
      if (y === null) return -1;
      return (sort.descending ? -1 : 1) * (x < y ? -1 : x > y ? 1 : 0);
    });
  } else if (sort.descending) {
    rows.reverse();
  }

  const head = element(
    "tr",
    "",
    COLUMNS.map((column, i) => {
      const th = element("th", i === 0 ? "" : "num");
      const button = element("button", "sort", column.label);

      th.setAttribute(
        "aria-sort",
        sort.column === i ? (sort.descending ? "descending" : "ascending") : "none",
      );
      button.type = "button";
      button.addEventListener("click", () => {
        sort =
          sort.column === i
            ? { column: i, descending: !sort.descending }
            : { column: i, descending: i !== 0 && i !== 6 && i !== 7 };
        render();
      });
      th.append(button);
      return th;
    }),
  );

  const body = rows.map((s) => {
    const tr = element(
      "tr",
      s.place.key === selectedPlace ? "selected" : "",
      COLUMNS.map((column, i) => element("td", i === 0 ? "" : "num", column.show(s))),
    );

    tr.tabIndex = 0;
    const choose = () => {
      selectedPlace = selectedPlace === s.place.key ? null : s.place.key;
      render();
      if (selectedPlace !== null) $("detail").scrollIntoView({ behavior: "smooth" });
    };
    tr.addEventListener("click", choose);
    tr.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") {
        e.preventDefault();
        choose();
      }
    });
    return tr;
  });

  table.replaceChildren(element("thead", "", head), element("tbody", "", body));
}

function drawDetail(stats: readonly PlaceStats[]): void {
  const section = $("detail");
  const s = stats.find((one) => one.place.key === selectedPlace);

  section.hidden = s === undefined;
  if (s === undefined) return;

  $("detail-title").textContent = placeName(s.place);
  $("detail-note").textContent =
    `${s.entered} visit(s): ${s.cleared} cleared, ${s.died} died, ` +
    `${s.visits.filter((v) => v.outcome === "warped").length} warped out, ` +
    `${s.visits.filter((v) => v.outcome === "left").length} left or reset.`;

  const strips: Strip[] = POWER_ORDER.flatMap((power) => {
    const points = s.visits
      .filter((v) => v.player.power === power && (v.outcome === "cleared" || v.outcome === "died"))
      .map((v) => ({
        value: v.end - v.start,
        cleared: v.outcome === "cleared",
        lines: [
          clock(v.end - v.start),
          `${v.outcome === "cleared" ? "Cleared" : "Died"} as ${loadoutLabel(v.player)}`,
          `${v.hits} hit(s)`,
          day(v.attempt) + (v.attempt.mode === "practice" ? " · practice" : ""),
        ],
      }));

    return points.length === 0 ? [] : [{ label: `${power} (${points.length})`, points }];
  });

  const chart = $("detail-chart");
  if (strips.length === 0) chart.replaceChildren(element("p", "note", "No clears or deaths to plot."));
  else stripChart(chart, strips);

  const byLoadout = new Map<string, Visit[]>();
  for (const v of s.visits) {
    const label = loadoutLabel(v.player);
    byLoadout.set(label, [...(byLoadout.get(label) ?? []), v]);
  }

  const rows = [...byLoadout].map(([label, visits]) => {
    const one = statsByPlace(visits)[0]!;
    return element("tr", "", [
      element("td", "", label),
      element("td", "num", String(one.entered)),
      element("td", "num", String(one.cleared)),
      element("td", "num", percent(one.cleared, one.entered)),
      element("td", "num", String(one.died)),
      element("td", "num", String(one.hits)),
      element("td", "num", orDash(one.clears[0] ?? null)),
      element("td", "num", orDash(median(one.clears))),
    ]);
  });

  $("detail-table").replaceChildren(
    element(
      "thead",
      "",
      element(
        "tr",
        "",
        ["Walked in as", "Entered", "Cleared", "Clear rate", "Deaths", "Hits", "Best", "Median"].map(
          (h, i) => element("th", i === 0 ? "" : "num", h),
        ),
      ),
    ),
    element("tbody", "", rows),
  );
}

function outcome(attempt: Attempt, visits: readonly Visit[]): string {
  if (attempt.finished) return "Finished";

  const last = stoppedAt(attempt, visits);
  const where = last === null ? "" : ` in ${placeName(last.place)}`;

  if (attempt.ended === null) return `Cut off${where}`;
  return attempt.mode === "practice" ? `Ended${where}` : `Reset${where}`;
}

function drawAttempts(attempts: readonly Attempt[]): void {
  const newest = [...attempts].reverse();
  const tbody = element("tbody");

  for (const attempt of newest.slice(0, shown)) {
    const visits = visitsOf(attempt);
    const what =
      attempt.mode === "practice"
        ? "Practice"
        : `${attempt.category === null ? "?" : category(attempt.category).name} · ${routeName(attempt.route)}` +
          (attempt.recording ? " (recording)" : "");
    const row = element("tr", "attempt", [
      element("td", "", day(attempt)),
      element("td", "", what),
      element("td", "num", clock(span(attempt))),
      element("td", "num", String(visits.filter((v) => v.outcome === "died").length)),
      element("td", "", outcome(attempt, visits)),
    ]);
    const log = element("tr", "log");
    log.hidden = true;

    row.tabIndex = 0;
    row.setAttribute("aria-expanded", "false");
    const toggle = () => {
      if (log.childElementCount === 0) {
        const list = element(
          "ol",
          "events",
          attempt.events.map(({ at, event }) =>
            element("li", event.kind, [element("time", "", clock(at)), describe(event)]),
          ),
        );
        const cell = element("td", "", list);
        cell.colSpan = 5;
        log.append(cell);
      }
      log.hidden = !log.hidden;
      row.setAttribute("aria-expanded", String(!log.hidden));
    };
    row.addEventListener("click", toggle);
    row.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") {
        e.preventDefault();
        toggle();
      }
    });

    tbody.append(row, log);
  }

  $("attempts-table").replaceChildren(
    element(
      "thead",
      "",
      element(
        "tr",
        "",
        ["Started", "What", "Length", "Deaths", "Outcome"].map((h, i) =>
          element("th", i === 2 || i === 3 ? "num" : "", h),
        ),
      ),
    ),
    tbody,
  );

  const more = $<HTMLButtonElement>("attempts-more");
  more.hidden = newest.length <= shown;
  more.textContent = `Show more (${newest.length - shown} left)`;
}

/** The route picker offers the routes the attempts in view were run on. */
function fillRoutes(all: readonly Attempt[]): void {
  const select = $<HTMLSelectElement>("filter-route");
  const ids = new Set<string>();

  for (const a of all) {
    if (a.route !== null && (filters.category === "" || a.category === filters.category)) {
      ids.add(a.route);
    }
  }
  if (filters.route !== "" && !ids.has(filters.route)) filters.route = "";

  select.replaceChildren(
    option("", "All routes"),
    ...[...ids].map((id) => option(id, routeName(id))),
  );
  select.value = filters.route;
}

let redraw = () => {};

function render(): void {
  const all = everything();
  fillRoutes(all);

  const attempts = filtered(all);
  const visits = attempts.flatMap(visitsOf);
  const stats = statsByPlace(visits);

  if (selectedPlace !== null && !stats.some((s) => s.place.key === selectedPlace)) {
    selectedPlace = null;
  }

  $("empty").hidden = attempts.length > 0;
  $("content").hidden = attempts.length === 0;

  drawTiles(attempts, visits);
  drawStages(stats);
  drawAttempts(attempts);

  // The charts are drawn at their host's width, so they wait for the rest of
  // the page to be laid out and are drawn again whenever that changes.
  redraw = () => {
    drawProgress(attempts);
    drawStops(attempts);
    drawDetail(stats);
  };
  redraw();
}

function describeSources(): void {
  const parts = [`${local.length} attempt(s) kept in this browser`];

  if (opened.size > 0) parts.push(`${opened.size} from opened files`);
  $("source").textContent = parts.join(" · ") + ".";
}

async function start(): Promise<void> {
  if (typeof Temporal === "undefined") {
    $("source").textContent =
      "This browser has no Temporal, which the history is kept with, so there is none to read here.";
    return;
  }

  const categorySelect = $<HTMLSelectElement>("filter-category");
  categorySelect.append(...CATEGORIES.map((c) => option(c.id, c.name)));

  const bind = (id: string, apply: (value: string) => void) => {
    const select = $<HTMLSelectElement>(id);
    select.addEventListener("change", () => {
      apply(select.value);
      shown = 50;
      render();
    });
  };

  bind("filter-mode", (v) => {
    filters.mode = v as Filters["mode"];
    const runsOnly = filters.mode === "practice";
    categorySelect.disabled = runsOnly;
    $<HTMLSelectElement>("filter-route").disabled = runsOnly;
    if (runsOnly) filters.category = filters.route = "";
    categorySelect.value = filters.category;
  });
  bind("filter-category", (v) => {
    filters.category = v;
  });
  bind("filter-route", (v) => {
    filters.route = v;
  });
  bind("filter-days", (v) => {
    filters.days = Number(v);
  });

  $("attempts-more").addEventListener("click", () => {
    shown += 50;
    render();
  });

  const file = $<HTMLInputElement>("file");
  $("open").addEventListener("click", () => file.click());
  file.addEventListener("change", async () => {
    for (const one of file.files ?? []) {
      for (const attempt of parseLines(await one.text())) opened.set(attempt.id, attempt);
    }
    file.value = "";
    describeSources();
    render();
  });

  // Width only: drawing a chart changes the page's height, and redrawing on
  // that would never stop.
  let width = 0;
  new ResizeObserver(([entry]) => {
    const now = entry?.contentRect.width ?? 0;
    if (now !== width) {
      width = now;
      redraw();
    }
  }).observe($("content"));

  const rows = await readHistory();
  if (rows === null) {
    $("source").textContent =
      "This browser is not keeping a history. Open an exported one to read it here.";
  } else {
    local = rows.flatMap((row) => parseAttempt(row) ?? []);
    describeSources();
  }

  render();
}

void start();
