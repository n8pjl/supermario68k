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
//
// It follows along while the game is played in another tab: history.ts
// announces every write, and the rows it wrote are merged in as they come.
// That can be several times a second, beside a game that wants the machine,
// so the work is split by who is waiting on it. Reading the rows is nobody's
// hurry: it runs as a background task (the Scheduler API, where there is
// one), yielding as it goes. Showing them is done once per frame, and
// rebuilds nothing. Every row and figure on the page is made once and kept,
// and an update changes the text that changed and nothing else; a chart is
// drawn again only when what it shows has changed. An event that moves one
// stage's numbers touches that stage's row, the attempt it belongs to, and a
// few figures at the top.

import { CATEGORIES, category, isCategoryId } from "../speedrun/category.ts";
import { type GameEvent } from "../speedrun/events.ts";
import { HISTORY_CHANNEL, readHistory } from "../speedrun/history.ts";
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
  visitsOf as cutIntoVisits,
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

/** This browser's history, by id, kept up to date as the game writes to it. */
const local = new Map<string, Attempt>();
const opened = new Map<string, Attempt>();
/**
 * Attempts this page has been told about as they were written. One of these
 * without an `ended` is being played now, rather than one the page closed on.
 */
const live = new Set<string>();
const filters: Filters = { mode: "all", category: "", route: "", days: 0 };
let selectedPlace: string | null = null;
let sort: { column: number; descending: boolean } = { column: 0, descending: false };
let shown = 50;

/**
 * Each attempt cut into visits once. An attempt that changes arrives as a new
 * object, so it is cut again and the rest are not.
 */
const visitCache = new WeakMap<Attempt, Visit[]>();

function visitsOf(attempt: Attempt): Visit[] {
  let visits = visitCache.get(attempt);

  if (visits === undefined) {
    visits = cutIntoVisits(attempt);
    visitCache.set(attempt, visits);
  }
  return visits;
}

function playing(attempt: Attempt): boolean {
  return attempt.ended === null && live.has(attempt.id);
}

/** Every attempt the page has, this browser's and the opened files', by id. */
function everything(): Attempt[] {
  const all = new Map(local);

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
// Changing only what changed

/**
 * Text set only when it is different. Setting it to what it already was still
 * replaces the text node, which the browser has to lay out again.
 */
function setText(el: Element, text: string): void {
  if (el.textContent !== text) el.textContent = text;
}

function setHidden(el: HTMLElement, hidden: boolean): void {
  if (el.hidden !== hidden) el.hidden = hidden;
}

/**
 * Put these nodes in the parent in this order, moving only the ones that are
 * out of place and removing whatever else is there. Rows that are already
 * where they belong are not touched, so a table that has not changed order is
 * not rebuilt.
 */
function arrange(parent: Element, nodes: readonly Node[]): void {
  let at = parent.firstChild;

  for (const node of nodes) {
    if (node === at) at = at.nextSibling;
    else parent.insertBefore(node, at);
  }
  while (at !== null) {
    const next: ChildNode | null = at.nextSibling;
    at.remove();
    at = next;
  }
}

/** Width of each chart's host, as the ResizeObserver last reported it. */
const widths = new Map<Element, number>();
/** What each chart was last drawn from; see drawChart(). */
const drawn = new WeakMap<Element, string>();

/**
 * Draw a chart if what it would show differs from what it shows now.
 *
 * `data` is everything the chart is drawn from, and the width it is drawn at
 * joins it, so a resize redraws too. A host with no width yet - hidden, or not
 * laid out - is left for the ResizeObserver to come back to.
 */
function drawChart(host: HTMLElement, data: unknown, draw: (width: number) => void): void {
  const width = widths.get(host) ?? 0;
  const key = JSON.stringify([width, data]);

  if (width === 0 || drawn.get(host) === key) return;

  drawn.set(host, key);
  draw(width);
}

function clearChart(host: HTMLElement): void {
  if (host.firstChild !== null) host.replaceChildren();
  drawn.delete(host);
}

/** Enter and space on a focusable row do what a click does. */
function activate(el: HTMLElement, action: () => void): void {
  el.tabIndex = 0;
  el.addEventListener("click", action);
  el.addEventListener("keydown", (e) => {
    if (e.key === "Enter" || e.key === " ") {
      e.preventDefault();
      action();
    }
  });
}

function cells(count: number, numeric: (i: number) => boolean): HTMLTableCellElement[] {
  return Array.from({ length: count }, (_, i) => element("td", numeric(i) ? "num" : ""));
}

// ---------------------------------------------------------------------------
// Drawing

interface Tile {
  readonly value: HTMLElement;
  readonly note: HTMLElement;
}

const TILES = ["Attempts", "Finished runs", "Best finish", "Played", "Deaths", "Hits taken"];
const tiles: Tile[] = [];

function buildTiles(): void {
  for (const label of TILES) {
    const tile = {
      value: element("strong", "tile-value"),
      note: element("span", "tile-note"),
    };

    tiles.push(tile);
    $("tiles").append(
      element("div", "tile", [element("span", "tile-label", label), tile.value, tile.note]),
    );
  }
}

function drawTiles(attempts: readonly Attempt[], visits: readonly Visit[]): void {
  const runs = attempts.filter((a) => a.mode === "run");
  const finished = runs.filter((a) => a.finished && a.total !== null);
  const best =
    filters.category === "" || finished.length === 0
      ? null
      : Math.min(...finished.map((a) => a.total!));

  const figures: [string, string][] = [
    [String(attempts.length), `${runs.length} timed, ${attempts.length - runs.length} practice`],
    [String(finished.length), percent(finished.length, runs.length) + " of runs"],
    [
      best === null ? "–" : clock(best),
      isCategoryId(filters.category) ? category(filters.category).name : "Pick a category",
    ],
    [played(attempts.reduce((sum, a) => sum + span(a), 0)), ""],
    [String(visits.filter((v) => v.outcome === "died").length), ""],
    [String(visits.reduce((sum, v) => sum + v.hits, 0)), ""],
  ];

  figures.forEach(([value, note], i) => {
    setText(tiles[i]!.value, value);
    setText(tiles[i]!.note, note);
  });
}

function drawProgress(attempts: readonly Attempt[]): void {
  const host = $("progress-chart");
  const note = $("progress-note");
  const finished = attempts.filter(
    (a) => a.mode === "run" && a.finished && a.total !== null,
  );

  const empty =
    filters.category === ""
      ? "Times from different categories are not the same race: pick a category to see them."
      : finished.length === 0
        ? "No finished runs here yet."
        : "";

  setText(note, empty);
  setHidden($("progress-legend"), empty !== "");
  if (empty !== "") {
    clearChart(host);
    return;
  }

  const points = finished.map((a) => ({
    when: a.started.epochMilliseconds,
    value: a.total!,
    lines: [clock(a.total!), day(a), routeName(a.route)],
  }));

  drawChart(host, points, (width) => progressChart(host, width, points));
}

function drawStops(attempts: readonly Attempt[]): void {
  const host = $("stops-chart");
  const note = $("stops-note");
  const counts = new Map<string, { label: string; died: number; left: number }>();

  for (const attempt of attempts) {
    if (attempt.mode !== "run" || attempt.finished || playing(attempt)) continue;

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
    clearChart(host);
    setText(note, "No unfinished runs here.");
    return;
  }

  setText(note, "The stage each unfinished run was last in. Top 12.");
  drawChart(host, bars, (width) => barChart(host, width, bars, "Where unfinished runs stopped"));
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

interface KeyedRow {
  readonly tr: HTMLTableRowElement;
  readonly cells: readonly HTMLTableCellElement[];
}

const stageHeads: HTMLTableCellElement[] = [];
const stageBody = element("tbody");
const stageRows = new Map<string, KeyedRow>();

function buildStages(): void {
  const head = element(
    "tr",
    "",
    COLUMNS.map((column, i) => {
      const th = element("th", i === 0 ? "" : "num");
      const button = element("button", "sort", column.label);

      button.type = "button";
      button.addEventListener("click", () => {
        sort =
          sort.column === i
            ? { column: i, descending: !sort.descending }
            : { column: i, descending: i !== 0 && i !== 6 && i !== 7 };
        render();
      });
      th.append(button);
      stageHeads.push(th);
      return th;
    }),
  );

  $("stages-table").append(element("thead", "", head), stageBody);
}

function stageRow(key: string): KeyedRow {
  let row = stageRows.get(key);

  if (row === undefined) {
    const tds = cells(COLUMNS.length, (i) => i !== 0);
    const tr = element("tr", "", tds);

    activate(tr, () => {
      selectedPlace = selectedPlace === key ? null : key;
      render();
      if (selectedPlace !== null) $("detail").scrollIntoView({ behavior: "smooth" });
    });
    row = { tr, cells: tds };
    stageRows.set(key, row);
  }
  return row;
}

function drawStages(stats: readonly PlaceStats[]): void {
  setText(
    $("stages-note"),
    stats.length === 0 ? "Nothing entered yet." : "Select a row to break it down.",
  );

  stageHeads.forEach((th, i) => {
    const state = sort.column === i ? (sort.descending ? "descending" : "ascending") : "none";
    if (th.getAttribute("aria-sort") !== state) th.setAttribute("aria-sort", state);
  });

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

  arrange(
    stageBody,
    rows.map((s) => {
      const row = stageRow(s.place.key);

      COLUMNS.forEach((column, i) => setText(row.cells[i]!, column.show(s)));
      row.tr.classList.toggle("selected", s.place.key === selectedPlace);
      return row.tr;
    }),
  );
}

const loadoutBody = element("tbody");
const loadoutRows = new Map<string, KeyedRow>();

function buildDetail(): void {
  $("detail-table").append(
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
    loadoutBody,
  );
}

function drawDetail(stats: readonly PlaceStats[]): void {
  const s = stats.find((one) => one.place.key === selectedPlace);

  setHidden($("detail"), s === undefined);
  if (s === undefined) return;

  setText($("detail-title"), placeName(s.place));
  setText(
    $("detail-note"),
    `${s.entered} visit(s): ${s.cleared} cleared, ${s.died} died, ` +
      `${s.visits.filter((v) => v.outcome === "warped").length} warped out, ` +
      `${s.visits.filter((v) => v.outcome === "left").length} left or reset.`,
  );

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
  if (strips.length === 0) {
    const key = "none";
    if (drawn.get(chart) !== key) {
      chart.replaceChildren(element("p", "note", "No clears or deaths to plot."));
      drawn.set(chart, key);
    }
  } else {
    drawChart(chart, strips, (width) => stripChart(chart, width, strips));
  }

  const byLoadout = new Map<string, Visit[]>();
  for (const v of s.visits) {
    const label = loadoutLabel(v.player);
    byLoadout.set(label, [...(byLoadout.get(label) ?? []), v]);
  }

  arrange(
    loadoutBody,
    [...byLoadout].map(([label, visits]) => {
      const one = statsByPlace(visits)[0]!;
      let row = loadoutRows.get(label);

      if (row === undefined) {
        const tds = cells(8, (i) => i !== 0);
        row = { tr: element("tr", "", tds), cells: tds };
        loadoutRows.set(label, row);
      }

      [
        label,
        String(one.entered),
        String(one.cleared),
        percent(one.cleared, one.entered),
        String(one.died),
        String(one.hits),
        orDash(one.clears[0] ?? null),
        orDash(median(one.clears)),
      ].forEach((text, i) => setText(row.cells[i]!, text));
      return row.tr;
    }),
  );
}

function outcome(attempt: Attempt, visits: readonly Visit[]): string {
  if (attempt.finished) return "Finished";

  const last = stoppedAt(attempt, visits);
  const where = last === null ? "" : ` in ${placeName(last.place)}`;

  if (playing(attempt)) return last === null ? "Playing, on the map" : `Playing${where}`;
  if (attempt.ended === null) return `Cut off${where}`;
  return attempt.mode === "practice" ? `Ended${where}` : `Reset${where}`;
}

/**
 * One attempt's two rows: the summary, and the event log under it, which is
 * only filled in the first time it is opened and is added to from then on.
 */
interface AttemptRows {
  attempt: Attempt;
  readonly row: HTMLTableRowElement;
  readonly cells: readonly HTMLTableCellElement[];
  readonly log: HTMLTableRowElement;
  list: HTMLOListElement | null;
}

const attemptBody = element("tbody");
const attemptRows = new Map<string, AttemptRows>();

function buildAttempts(): void {
  $("attempts-table").append(
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
    attemptBody,
  );
}

function eventItem({ at, event }: Attempt["events"][number]): HTMLLIElement {
  return element("li", event.kind, [element("time", "", clock(at)), describe(event)]);
}

/** The log, if it is open, brought up to the attempt's events. */
function fillLog(entry: AttemptRows): void {
  const list = entry.list;
  if (list === null) return;

  const events = entry.attempt.events;

  if (list.childElementCount > events.length) list.replaceChildren();
  list.append(...events.slice(list.childElementCount).map(eventItem));
}

function attemptRow(attempt: Attempt): AttemptRows {
  let entry = attemptRows.get(attempt.id);

  if (entry === undefined) {
    const tds = cells(5, (i) => i === 2 || i === 3);
    const made: AttemptRows = {
      attempt,
      row: element("tr", "attempt", tds),
      cells: tds,
      log: element("tr", "log"),
      list: null,
    };

    made.log.hidden = true;
    made.row.setAttribute("aria-expanded", "false");
    activate(made.row, () => {
      if (made.list === null) {
        made.list = element("ol", "events");
        const cell = element("td", "", made.list);
        cell.colSpan = 5;
        made.log.append(cell);
        fillLog(made);
      }
      made.log.hidden = !made.log.hidden;
      made.row.setAttribute("aria-expanded", String(!made.log.hidden));
    });

    entry = made;
    attemptRows.set(attempt.id, entry);
  }

  entry.attempt = attempt;
  return entry;
}

function drawAttempts(attempts: readonly Attempt[]): void {
  const newest = [...attempts].reverse();

  arrange(
    attemptBody,
    newest.slice(0, shown).flatMap((attempt) => {
      const entry = attemptRow(attempt);
      const visits = visitsOf(attempt);
      const what =
        attempt.mode === "practice"
          ? "Practice"
          : `${attempt.category === null ? "?" : category(attempt.category).name} · ${routeName(attempt.route)}` +
            (attempt.recording ? " (recording)" : "");

      [
        day(attempt),
        what,
        clock(span(attempt)),
        String(visits.filter((v) => v.outcome === "died").length),
        outcome(attempt, visits),
      ].forEach((text, i) => setText(entry.cells[i]!, text));
      fillLog(entry);
      return [entry.row, entry.log];
    }),
  );

  const more = $<HTMLButtonElement>("attempts-more");
  setHidden(more, newest.length <= shown);
  setText(more, `Show more (${newest.length - shown} left)`);
}

/**
 * The route picker offers the routes the attempts in view were run on. Its
 * options are only replaced when that list changes: replacing them under a
 * player with the picker open would close it on them.
 */
let routeOptions = "";

function fillRoutes(all: readonly Attempt[]): void {
  const select = $<HTMLSelectElement>("filter-route");
  const ids = new Set<string>();

  for (const a of all) {
    if (a.route !== null && (filters.category === "" || a.category === filters.category)) {
      ids.add(a.route);
    }
  }
  if (filters.route !== "" && !ids.has(filters.route)) filters.route = "";

  const key = JSON.stringify([...ids].map((id) => [id, routeName(id)]));
  if (key !== routeOptions) {
    routeOptions = key;
    select.replaceChildren(
      option("", "All routes"),
      ...[...ids].map((id) => option(id, routeName(id))),
    );
  }
  if (select.value !== filters.route) select.value = filters.route;
}

let current: { attempts: Attempt[]; stats: PlaceStats[] } = { attempts: [], stats: [] };

/** The charts, which need their widths and so are also drawn on a resize. */
function drawCharts(): void {
  drawProgress(current.attempts);
  drawStops(current.attempts);
  drawDetail(current.stats);
}

function render(): void {
  stale = false;

  const all = everything();
  fillRoutes(all);

  const attempts = filtered(all);
  const visits = attempts.flatMap(visitsOf);
  const stats = statsByPlace(visits);

  if (selectedPlace !== null && !stats.some((s) => s.place.key === selectedPlace)) {
    selectedPlace = null;
  }

  setHidden($("empty"), attempts.length > 0);
  setHidden($("content"), attempts.length === 0);

  current = { attempts, stats };
  drawTiles(attempts, visits);
  drawStages(stats);
  drawAttempts(attempts);
  drawCharts();
}

/**
 * Render once the frame comes round, however many updates arrive before it.
 * A tab in the background gets no frames, so one that is not being looked at
 * does no work at all, and catches up in one go when it is.
 */
let stale = false;
let frame = 0;

function schedule(): void {
  stale = true;
  if (frame !== 0) return;

  frame = requestAnimationFrame(() => {
    frame = 0;
    if (stale) render();
  });
}

function describeSources(): void {
  const parts = [`${local.size} attempt(s) kept in this browser`];

  if (opened.size > 0) parts.push(`${opened.size} from opened files`);
  setText($("source"), parts.join(" · ") + ".");
}

// ---------------------------------------------------------------------------
// Reading rows in, out of the way

/** The Scheduler API, where this browser has it. */
const tasks = typeof scheduler === "undefined" ? null : scheduler;

/** Run this at that priority: "background" is when nothing else is waiting. */
function post(task: () => Promise<void>, priority: TaskPriority): void {
  if (tasks === null) setTimeout(() => void task(), 0);
  else void tasks.postTask(task, { priority });
}

/** Let the page, and anything more pressing, have the thread for a moment. */
function yieldToPage(): Promise<void> {
  return tasks === null
    ? new Promise((resolve) => setTimeout(resolve, 0))
    : tasks.yield();
}

/** Rows parsed between yields: a few milliseconds' worth. */
const ROWS_PER_SLICE = 25;

/**
 * Rows waiting to be read, by attempt id. A later row for an attempt replaces
 * an earlier one still waiting, so an attempt that changed five times since
 * the last read is read once.
 */
const waiting = new Map<string, unknown>();
let reading = false;

/**
 * Rows of this browser's history, to be merged in when there is time - or
 * sooner, for the first read, which is what the page is waiting to show.
 */
function receive(
  rows: readonly unknown[],
  announced: boolean,
  priority: TaskPriority = "background",
): void {
  for (const row of rows) {
    const id = typeof row === "object" && row !== null ? (row as { id?: unknown }).id : null;
    if (typeof id !== "string") continue;

    waiting.set(id, row);
    if (announced) live.add(id);
  }

  if (reading || waiting.size === 0) return;
  reading = true;
  post(readWaiting, priority);
}

async function readWaiting(): Promise<void> {
  let changed = false;
  let sliced = 0;

  while (waiting.size > 0) {
    const [id, row] = waiting.entries().next().value!;
    waiting.delete(id);
    changed = merge(row) || changed;

    if (++sliced % ROWS_PER_SLICE === 0) await yieldToPage();
  }

  reading = false;
  if (changed) {
    describeSources();
    schedule();
  }
}

/**
 * One row merged in. An attempt that has not changed keeps the object it had,
 * so nothing worked out from it has to be again. Says whether it changed.
 */
function merge(row: unknown): boolean {
  const attempt = parseAttempt(row);
  if (attempt === null) return false;

  const held = local.get(attempt.id);
  const same =
    held !== undefined &&
    held.events.length === attempt.events.length &&
    held.ended?.epochMilliseconds === attempt.ended?.epochMilliseconds &&
    held.total === attempt.total &&
    held.route === attempt.route;

  if (!same) local.set(attempt.id, attempt);
  return !same;
}

/** The whole history read again, for whatever the announcements missed. */
async function reload(priority: TaskPriority = "background"): Promise<boolean> {
  const rows = await readHistory();
  if (rows === null) return false;

  receive(rows, false, priority);
  // Nothing to read - an empty history - still has to be shown as one.
  if (!reading) {
    describeSources();
    schedule();
  }
  return true;
}

async function start(): Promise<void> {
  if (typeof Temporal === "undefined") {
    $("source").textContent =
      "This browser has no Temporal, which the history is kept with, so there is none to read here.";
    return;
  }

  buildTiles();
  buildStages();
  buildDetail();
  buildAttempts();

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

  // Each chart is drawn at its host's width, which is only ever read here:
  // the observer is told it after layout, where asking for it would force one.
  // Only a change of width matters - a chart changes its own height, and
  // redrawing on that would never stop.
  const observer = new ResizeObserver((entries) => {
    let changed = false;

    for (const entry of entries) {
      const width = Math.round(entry.contentRect.width);
      if (widths.get(entry.target) !== width) {
        widths.set(entry.target, width);
        changed = true;
      }
    }
    if (changed) drawCharts();
  });
  for (const id of ["progress-chart", "stops-chart", "detail-chart"]) observer.observe($(id));

  // Every write the game makes, as it makes it; see HISTORY_CHANNEL.
  try {
    new BroadcastChannel(HISTORY_CHANNEL).addEventListener("message", (e) => {
      if (Array.isArray(e.data)) receive(e.data, true);
    });
  } catch {
    /* No live updates: coming back to the tab still reloads, below. */
  }

  // An announcement can go unheard - a page the browser froze in the
  // background is not woken for one - so coming back to the tab reads the
  // history again, and whatever changed is merged like any other update.
  document.addEventListener("visibilitychange", () => {
    if (!document.hidden) void reload();
  });

  if (!(await reload("user-visible"))) {
    $("source").textContent =
      "This browser is not keeping a history. Open an exported one to read it here.";
    render();
  }
}

void start();
