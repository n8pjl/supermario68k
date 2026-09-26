// The routing page: the run history read as a plan for Any% warpless.
//
// Three parts, all read from the same history the data page reads (this
// browser's, and any exported files opened here). The route search runs the
// search in search.ts on a worker and lays out what it chose and what it
// beat. Weak data lists what that route rests on that the history does not
// hold well: stages never played walked in as what the route walks in as,
// thin or practice-only figures, and close calls the next few attempts could
// turn. The worlds show each map as the search sees it, with every stage's
// history and every item on offer.
//
// It follows a game played in another tab the way the data page does - see
// the notes at the top of analysis/index.ts - though with less care for
// cost, since everything here is worked out again once the history stops
// changing for a moment: a search is a second or two of work, and there is
// no sense starting one per event.

import { type Power } from "../speedrun/events.ts";
import { HISTORY_CHANNEL, readHistory } from "../speedrun/history.ts";
import { levelName, monsterName } from "../speedrun/names.ts";
import { type Attempt, parseAttempt, parseLines } from "../analysis/attempts.ts";
import { clock } from "../analysis/charts.ts";
import { type Visit, visitsOf as cutIntoVisits } from "../analysis/visits.ts";
import { MAPS } from "./maps.ts";
import {
  type Entry,
  type Model,
  POWERS,
  type Summary,
  buildModel,
  entryKey,
  entryLabel,
  parseEntryKey,
} from "./model.ts";
import { type Alternative, type Costed, type Plan, type Step } from "./plan.ts";
import { DEFAULTS, type Settings } from "./search.ts";
import { type PlaceInfo, type Strength, drawWorld } from "./view.ts";
import { type Reply, type Request } from "./worker.ts";

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

function row(cells: (string | Node)[], numeric: (i: number) => boolean = () => false, tag: "td" | "th" = "td") {
  return element(
    "tr",
    "",
    cells.map((c, i) => element(tag, numeric(i) ? "num" : "", c)),
  );
}

function percent(part: number, whole: number): string {
  return whole === 0 ? "–" : `${Math.round((part / whole) * 100)}%`;
}

function seconds(ms: number): string {
  return `${(ms / 1000).toFixed(1)} s`;
}

function orDash(ms: number | null): string {
  return ms === null || !Number.isFinite(ms) ? "–" : clock(ms);
}

const ITEM_NAMES: Record<string, string> = {
  mushroom: "mushroom",
  "fire-flower": "fire flower",
  leaf: "leaf",
  star: "star",
  whistle: "whistle",
  hammer: "hammer",
  "p-wing": "P-wing",
  cloud: "cloud",
  anchor: "anchor",
  random: "a mushroom, fire flower or leaf",
};

function itemName(item: string): string {
  return ITEM_NAMES[item] ?? item;
}

/** "L0.3" to "1-4", "M1.2" to "2-Fire Bros.". */
function nameOf(place: string): string {
  const [world, index] = place.slice(1).split(".").map(Number) as [number, number];
  return place.startsWith("M") ? monsterName(world, index) : levelName(world, index);
}

function worldOf(place: string): number {
  return Number(place.slice(1).split(".")[0]);
}

// ---------------------------------------------------------------------------
// The history

const local = new Map<string, Attempt>();
const opened = new Map<string, Attempt>();
let days = 0;

const visitCache = new WeakMap<Attempt, Visit[]>();

function visitsOf(attempt: Attempt): Visit[] {
  let visits = visitCache.get(attempt);
  if (visits === undefined) {
    visits = cutIntoVisits(attempt);
    visitCache.set(attempt, visits);
  }
  return visits;
}

function attempts(): Attempt[] {
  const all = new Map(local);
  for (const [id, a] of opened) all.set(id, a);

  const since = days === 0 ? -Infinity : Date.now() - days * 86_400_000;
  return [...all.values()]
    .filter((a) => a.started.epochMilliseconds >= since)
    .sort((a, b) => a.started.epochMilliseconds - b.started.epochMilliseconds);
}

function describeSources(): void {
  const parts = [`${local.size} attempt(s) kept in this browser`];
  if (opened.size > 0) parts.push(`${opened.size} from opened files`);
  $("source").textContent = parts.join(" · ") + ".";
}

// ---------------------------------------------------------------------------
// Settings, kept between visits

const SETTINGS_KEY = "sm68k.routing.settings";
/** The fields the form shows in seconds and the search takes in milliseconds. */
const IN_SECONDS = ["unknownMs", "msPerTile", "overheadMs", "respawnMs", "houseMs", "pipeMs", "chaseMs"] as const;

function loadSettings(): Settings {
  try {
    const saved: unknown = JSON.parse(localStorage.getItem(SETTINGS_KEY) ?? "null");
    if (typeof saved === "object" && saved !== null) {
      const out: Record<string, unknown> = { ...DEFAULTS };
      for (const [key, value] of Object.entries(DEFAULTS)) {
        const got = (saved as Record<string, unknown>)[key];
        if (typeof got === typeof value) out[key] = got;
      }
      return out as unknown as Settings;
    }
  } catch {
    /* Defaults, then. */
  }
  return DEFAULTS;
}

let settings = loadSettings();

function settingsToForm(): void {
  const form = $<HTMLFormElement>("settings");
  const field = (name: string) => form.elements.namedItem(name) as HTMLInputElement | HTMLSelectElement;

  (field("objective") as HTMLSelectElement).value = settings.objective;
  (field("unknown") as HTMLSelectElement).value = settings.unknown;
  (field("items") as HTMLInputElement).checked = settings.items;
  (field("detours") as HTMLInputElement).checked = settings.detours;
  (field("detours") as HTMLInputElement).disabled = !settings.items;
  for (const name of IN_SECONDS) {
    field(name).value = String(Math.round(settings[name]) / 1000);
  }
}

function settingsFromForm(): Settings {
  const form = $<HTMLFormElement>("settings");
  const field = (name: string) => form.elements.namedItem(name) as HTMLInputElement;
  const out: Record<string, unknown> = {
    objective: field("objective").value,
    unknown: field("unknown").value,
    items: field("items").checked,
    detours: field("detours").checked,
  };
  for (const name of IN_SECONDS) {
    const n = Number(field(name).value);
    out[name] = Number.isFinite(n) && n >= 0 ? n * 1000 : DEFAULTS[name];
  }
  return out as unknown as Settings;
}

function saveSettings(): void {
  try {
    localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
  } catch {
    /* Only this visit keeps them. */
  }
}

// ---------------------------------------------------------------------------
// The search, on a worker

let worker: Worker | null = null;
let asked = 0;
let plan: Plan | null = null;
let planError: string | null = null;
let searching = false;

function reply(e: MessageEvent<Reply>): void {
  // Only the answer to the latest question: the history or the settings have
  // moved on since an older one was asked.
  if (e.data.id !== asked) return;

  searching = false;
  if ("plan" in e.data) {
    plan = e.data.plan;
    planError = null;
  } else {
    plan = null;
    planError = e.data.error;
  }
  render();
}

function runSearch(): void {
  clearTimeout(pending);

  // A search still going is answering an old question, and a worker does one
  // thing at a time: rather than wait for it, start again on a new one.
  if (searching) {
    worker?.terminate();
    worker = null;
  }
  if (worker === null) {
    worker = new Worker(new URL("./routing-worker.js", import.meta.url), { type: "module" });
    worker.addEventListener("message", reply);
    worker.addEventListener("error", () => {
      searching = false;
      planError = "The search stopped with an error.";
      render();
    });
  }

  searching = true;
  const request: Request = { id: ++asked, stats: model.stats, settings };
  worker.postMessage(request);
  renderStatus();
}

/** Search again once the history has been still for a moment. */
let pending = 0;
function searchSoon(): void {
  clearTimeout(pending);
  pending = setTimeout(runSearch, 1500);
}

// ---------------------------------------------------------------------------
// Working it out

let model: Model = buildModel([], visitsOf);

function rebuild(): void {
  model = buildModel(attempts(), visitsOf);
}

function summary(place: string, key: string): Summary | undefined {
  return model.stats[place]?.[key];
}

function allSummaries(place: string): Summary[] {
  return Object.values(model.stats[place] ?? {});
}

function strength(place: string): Strength {
  const clears = allSummaries(place).reduce((n, s) => n + s.clears, 0);
  return clears === 0 ? "none" : clears < 3 ? "thin" : "ok";
}

/** What is weak about one stage walked in as one thing. */
function weaknesses(place: string, key: string, costed?: Costed): string[] {
  const s = summary(place, key);
  const out: string[] = [];

  if (costed?.source === "assumed") out.push("never cleared, as anything: time assumed");
  else if (costed?.source === "borrowed") {
    out.push(`never cleared as this: borrowed from ${entryLabel(parseEntryKey(costed.from!))}`);
  } else if (s === undefined || s.clears === 0) out.push("never cleared as this");

  if (s !== undefined && s.clears > 0) {
    if (s.clears < 3) out.push(`only ${s.clears} clear(s)`);
    if (s.practice === s.clears + s.deaths) out.push("practice only");
    if (s.spread !== null && s.spread > 0.25) out.push(`times vary: middle half spans ${Math.round(s.spread * 100)}%`);
    if (s.exitsSeen < s.clears / 2) out.push("what it leaves you as is mostly estimated from hits");
    if (s.newest !== null && Date.now() - s.newest > 60 * 86_400_000) {
      out.push(`last played ${Math.round((Date.now() - s.newest) / 86_400_000)} days ago`);
    }
  }
  return out;
}

function dataNote(place: string, key: string, source: string | null, from: string | null): { text: string; cls: string } {
  if (source === "assumed") return { text: "no history: assumed", cls: "missing" };
  if (source === "borrowed") return { text: `borrowed from ${entryLabel(parseEntryKey(from!))}`, cls: "weak" };

  const s = summary(place, key);
  if (s === undefined) return { text: "–", cls: "dim" };
  const practice = s.practice > 0 ? ` · ${percent(s.practice, s.clears + s.deaths)} practice` : "";
  return { text: `${s.clears} clear(s)${practice}`, cls: s.clears < 3 ? "weak" : "" };
}

// ---------------------------------------------------------------------------
// Drawing: the route

function stepName(kind: Step["kind"], world: number, node: number, place: string | null): string {
  switch (kind) {
    case "stage":
    case "bros":
      return nameOf(place!);
    case "cloud":
      return `Cloud over ${nameOf(place!)}`;
    case "house":
      return "Mushroom house";
    case "rock": {
      const n = MAPS[world]!.nodes[node]!;
      return `Hammer a rock (by ${n.x},${n.y})`;
    }
  }
}

function usesOf(use: readonly string[]): Node[] {
  return use.map((u) => element("span", "use", itemName(u)));
}

/** What an option spends, less what its entry already says: a star, a P-wing. */
function spendNote(use: readonly string[], entry: Entry | null): string {
  const shown = entry === null ? use : use.filter((u) => u !== "star" && u !== "p-wing");
  return shown.length > 0 ? ` (spend ${shown.map(itemName).join(", ")})` : "";
}

function describeAlt(world: number, alt: Alternative): string {
  const what = stepName(alt.kind, world, alt.node, alt.place);
  const as = alt.entry ? ` as ${entryLabel(alt.entry)}` : "";
  return `${what}${as}${spendNote(alt.use, alt.entry)}, +${seconds(alt.deltaMs)}`;
}

/**
 * What a step leaves the player as and with: the powers by how likely they
 * are, and what was handed over, said once where every outcome gets it.
 */
function outcomesText(step: Step): string {
  const final = step.place !== null && MAPS[step.world]!.nodes[step.node]!.kind === "bowser";
  if (final) return "";

  const got = (o: Step["outcomes"][number]) => o.got.map(itemName).join(", ");
  const shared = step.outcomes.every((o) => got(o) === got(step.outcomes[0]!)) ? got(step.outcomes[0]!) : null;
  const fights = step.kind === "stage" || step.kind === "bros";

  const parts = new Map<string, number>();
  for (const o of step.outcomes) {
    const label = [
      fights ? entryLabel({ power: o.power, star: false, pwing: false }) : "",
      shared === null && got(o) !== "" ? got(o) : "",
    ]
      .filter(Boolean)
      .join(", ");
    if (label !== "") parts.set(label, (parts.get(label) ?? 0) + o.p);
  }

  const odds =
    parts.size === 1
      ? [...parts.keys()][0]!
      : [...parts]
          .sort((a, b) => b[1] - a[1])
          .map(([label, p]) => `${label} ${Math.round(p * 100)}%`)
          .join(" · ");
  return [odds, shared ? `gets ${shared}` : ""].filter(Boolean).join("; ");
}

function drawTiles(p: Plan): void {
  const stages = p.steps.filter((s) => s.kind === "stage").length;
  const spent = p.steps.flatMap((s) => s.use);
  const weak = routeHoles(p).length;

  const tiles: [string, string, string][] = [
    [
      settings.objective === "expected" ? "Expected time" : settings.objective === "median" ? "Sum of medians" : "Sum of bests",
      (p.bound ? "≤ " : "") + clock(p.total),
      p.bound ? "at most: the route can come in under it" : "map time included",
    ],
    ["Stages played", String(stages), `${p.steps.filter((s) => s.kind === "cloud").length} skipped by cloud`],
    ["Items spent", String(spent.length), spent.length > 0 ? [...new Set(spent)].map(itemName).join(", ") : "none"],
    ["Weak spots", String(weak), "on the route, or deciding it"],
  ];

  $("result-tiles").replaceChildren(
    ...tiles.map(([label, value, note]) =>
      element("div", "tile", [
        element("span", "tile-label", label),
        element("strong", "tile-value", value),
        element("span", "tile-note", note),
      ]),
    ),
  );
}

function drawRoute(p: Plan): void {
  const head = row(
    ["#", "What", "Walk in as", "Spend", "Here", "Leaves you", "History", "Next best"],
    (i) => i === 4,
    "th",
  );
  const body = element("tbody");
  let world = -1;

  p.steps.forEach((step, i) => {
    if (step.world !== world) {
      world = step.world;
      const tr = element("tr", "world-row");
      const td = element("td", "", `World ${world + 1}`);
      td.colSpan = 8;
      tr.append(td);
      body.append(tr);
    }

    const key = step.entry ? entryKey(step.entry) : "";
    const note = step.place && step.costing ? dataNote(step.place, key, step.costing.source, step.costing.from) : { text: "", cls: "" };
    const alt = step.alternatives[0];
    const close = alt !== undefined && alt.deltaMs < Math.max(2000, step.left * 0.01);

    const tr = row(
      [
        String(i + 1),
        stepName(step.kind, step.world, step.node, step.place),
        step.entry ? entryLabel(step.entry) : "",
        element("span", "", usesOf(step.use)),
        seconds(step.walkMs + step.doMs),
        outcomesText(step),
        element("span", note.cls, note.text),
        element("span", close ? "weak" : "dim", alt ? describeAlt(step.world, alt) : "–"),
      ],
      (j) => j === 4,
    );
    tr.cells[5]!.className = "wrap";
    tr.cells[7]!.className = "wrap";
    tr.title = step.holding.length > 0 ? `Holding: ${step.holding.map(itemName).join(", ")}` : "Holding nothing";
    body.append(tr);
  });

  $("route-table").replaceChildren(element("thead", "", head), body);
}

function renderStatus(): void {
  const status = $("search-status");
  if (searching) status.textContent = "Searching…";
  else if (planError !== null) status.textContent = planError;
  else if (plan !== null) {
    status.textContent =
      `Searched ${plan.states.toLocaleString()} states in ${seconds(plan.ms)}.` +
      (settings.detours
        ? " Detours are weighed one at a time against the best route without further ones, so two that only pay off together can be missed."
        : "");
  } else status.textContent = "";
}

// ---------------------------------------------------------------------------
// Drawing: weak data

interface Hole {
  readonly rank: number;
  readonly place: string;
  readonly key: string;
  readonly issues: readonly string[];
  readonly why: string;
}

/** What the route itself rests on, and the close calls that decided it. */
function routeHoles(p: Plan): Hole[] {
  const out: Hole[] = [];
  const costedBy = new Map(p.costed.map((c) => [`${c.place}|${c.entry}`, c]));

  for (const step of p.steps) {
    if (step.place !== null && step.entry !== null && step.costing !== null) {
      const key = entryKey(step.entry);
      const issues = weaknesses(step.place, key, costedBy.get(`${step.place}|${key}`));
      if (issues.length > 0) out.push({ rank: 0, place: step.place, key, issues, why: "on the route" });
    }

    const alt = step.alternatives[0];
    if (alt !== undefined && alt.place !== null && alt.entry !== null && alt.deltaMs < Math.max(2000, step.left * 0.01)) {
      const key = entryKey(alt.entry);
      const issues = weaknesses(alt.place, key, costedBy.get(`${alt.place}|${key}`));
      if (issues.length > 0) {
        out.push({
          rank: 1,
          place: alt.place,
          key,
          issues,
          why: `close call: only ${seconds(alt.deltaMs)} behind step ${p.steps.indexOf(step) + 1}`,
        });
      }
    }
  }

  // Once each: a stage can be on the route and a close call elsewhere too,
  // and it is the first reason, the route, that counts.
  const seen = new Set<string>();
  return out.filter((h) => {
    const id = `${h.place}|${h.key}`;
    if (seen.has(id)) return false;
    seen.add(id);
    return true;
  });
}

let holesShown = 25;

function drawHoles(): void {
  const note = $("holes-note");
  const table = $("holes-table");

  if (plan === null) {
    note.textContent = "Shown once a route has been searched.";
    table.replaceChildren();
    $("holes-more").hidden = true;
    return;
  }

  const holes = routeHoles(plan);
  const seen = new Set(holes.map((h) => `${h.place}|${h.key}`));

  // Everything else the search could have walked into, and had to guess at:
  // one row a stage, with every way in it guessed.
  const guessed = new Map<string, Costed[]>();
  for (const c of plan.costed) {
    const id = `${c.place}|${c.entry}`;
    if (seen.has(id) || c.source === "data") continue;
    seen.add(id);
    guessed.set(c.place, [...(guessed.get(c.place) ?? []), c]);
  }
  for (const [place, list] of guessed) {
    const assumed = list.every((c) => c.source === "assumed");
    holes.push({
      rank: 2,
      place,
      key: list.map((c) => c.entry).join(","),
      issues: [assumed ? "never cleared, as anything: time assumed" : "never cleared walked in as these: figures borrowed"],
      why: "reachable, not on the route",
    });
  }

  holes.sort((a, b) => a.rank - b.rank || worldOf(a.place) - worldOf(b.place));
  note.textContent =
    holes.length === 0
      ? "Nothing: every stage the route goes through has three or more clears walked in as the route walks in."
      : "Practising these firms the route up. First what it goes through, then the options it only just beat, then the rest it had to guess at.";

  const head = row(["Where", "Walked in as", "What is weak", "History", "Why it matters"], () => false, "th");
  const body = element(
    "tbody",
    "",
    holes.slice(0, holesShown).map((h) => {
      const keys = h.key.split(",");
      const s = keys.length === 1 ? summary(h.place, h.key) : undefined;
      const tr = row([
        nameOf(h.place),
        keys.map((k) => entryLabel(parseEntryKey(k))).join("; "),
        element("span", h.issues.some((i) => i.startsWith("never")) ? "missing" : "weak", h.issues.join("; ")),
        s === undefined ? "–" : `${s.clears} clear(s), ${s.deaths} death(s)`,
        h.why,
      ]);
      tr.cells[1]!.className = "wrap";
      tr.cells[2]!.className = "wrap";
      return tr;
    }),
  );
  table.replaceChildren(element("thead", "", head), body);

  const more = $<HTMLButtonElement>("holes-more");
  more.hidden = holes.length <= holesShown;
  more.textContent = `Show more (${holes.length - holesShown} left)`;
}

// ---------------------------------------------------------------------------
// Drawing: the worlds

let world = 0;
let selected: string | null = null;

function placeInfo(): Map<string, PlaceInfo> {
  const info = new Map<string, PlaceInfo>();
  for (const place of Object.keys(model.stats)) {
    const lines = Object.entries(model.stats[place]!)
      .filter(([, s]) => s.clears + s.deaths > 0)
      .map(([key, s]) => `${entryLabel(parseEntryKey(key))}: ${s.clears} clear(s), median ${orDash(s.median)}, ${s.deaths} death(s)`);
    info.set(place, { strength: strength(place), lines });
  }
  return info;
}

function drawTabs(): void {
  $("world-tabs").replaceChildren(
    ...MAPS.map((_, w) => {
      const b = element("button", "", `World ${w + 1}`);
      b.type = "button";
      b.setAttribute("role", "tab");
      b.setAttribute("aria-selected", String(w === world));
      b.addEventListener("click", () => {
        world = w;
        selected = null;
        render();
      });
      return b;
    }),
  );
}

/** The world's places in the order the map reads. */
function placesOf(w: number): string[] {
  const map = MAPS[w]!;
  const stages = map.nodes
    .filter((n) => n.level !== undefined)
    .map((n) => `L${w}.${n.level}`);
  const order = (p: string) => {
    const name = nameOf(p).replace(/^\d+-/, "");
    const n = Number(name);
    return Number.isNaN(n) ? 100 + ["Fortress", "Fortress 1", "Fortress 2", "Pyramid", "Quicksand", "Bonus", "Castle", "Bowser"].indexOf(name) : n;
  };
  return [
    ...[...new Set(stages)].sort((a, b) => order(a) - order(b)),
    ...map.bros.map((b) => `M${w}.${b.monster}`),
  ];
}

function medianCell(place: string, power: Power): string {
  const s = summary(place, power);
  return s === undefined || s.clears === 0 ? "–" : `${clock(s.median!)} (${s.clears})`;
}

function drawStagesTable(): void {
  const head = row(["Stage", "Clears", "Deaths", "Practice", "Small", "Super", "Fire", "Raccoon", "Star or P-wing"], (i) => i > 0, "th");
  const body = element(
    "tbody",
    "",
    placesOf(world).map((place) => {
      const all = allSummaries(place);
      const clears = all.reduce((n, s) => n + s.clears, 0);
      const deaths = all.reduce((n, s) => n + s.deaths, 0);
      const practice = all.reduce((n, s) => n + s.practice, 0);
      const extras = Object.entries(model.stats[place] ?? {})
        .filter(([key, s]) => key.includes("+") && s.clears > 0)
        .reduce((n, [, s]) => n + s.clears, 0);

      const tr = row(
        [
          nameOf(place),
          String(clears),
          String(deaths),
          percent(practice, clears + deaths),
          ...POWERS.map((p) => medianCell(place, p)),
          extras === 0 ? "–" : `${extras} clear(s)`,
        ],
        (i) => i > 0,
      );
      tr.tabIndex = 0;
      tr.classList.toggle("selected", place === selected);
      const pick = () => {
        selected = selected === place ? null : place;
        render();
      };
      tr.addEventListener("click", pick);
      tr.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " ") {
          e.preventDefault();
          pick();
        }
      });
      if (clears === 0) tr.cells[1]!.classList.add("missing");
      else if (clears < 3) tr.cells[1]!.classList.add("weak");
      return tr;
    }),
  );
  $("stages-table").replaceChildren(element("thead", "", head), body);
}

function drawItems(): void {
  const map = MAPS[world]!;
  const given: (string | Node)[] = [];

  if (map.reward !== null) {
    given.push(`Beating the castle: ${itemName(map.reward)}${map.airship ? ". The castle is an airship, which flies off on a death unless anchored" : ""}.`);
  }
  for (const b of map.bros) {
    given.push(`${monsterName(world, b.monster)}: ${itemName(b.treasure)}.`);
  }
  const houses = map.nodes.filter((n) => n.kind === "house").length;
  if (houses > 0) {
    given.push(`${houses} mushroom house(s): one chest each, a mushroom or fire flower (1 in 4 each) or a leaf (1 in 2).`);
  }
  for (const e of map.events) {
    const when = e.coins === null ? "Beating" : `Beating with exactly ${e.coins} coins`;
    const what =
      e.kind === "house"
        ? `a hidden mushroom house with ${itemName(e.item!)}`
        : e.kind === "card-game"
          ? "a card game (mushrooms, flowers, stars)"
          : "a money ship (coins)";
    given.push(`${when} ${levelName(world, e.level)} adds ${what}. Not planned on: nothing reports coins.`);
  }

  $("items-given").replaceChildren(...given.map((g) => element("li", "", g)));

  const uses = model.uses.filter((u) => worldOf(u.place) === world);
  const counts = new Map<string, { item: string; place: string; n: number; practice: number }>();
  for (const u of uses) {
    const id = `${u.item}|${u.place}`;
    const c = counts.get(id) ?? { item: u.item, place: u.place, n: 0, practice: 0 };
    c.n++;
    if (u.practice) c.practice++;
    counts.set(id, c);
  }

  $("items-used-note").textContent =
    counts.size === 0
      ? "Nothing spent from the item list before a stage in this world yet."
      : "Items spent from the list on the map, and what was entered next.";
  const head = row(["Item", "Before", "Times", "Practice"], (i) => i > 1, "th");
  const body = element(
    "tbody",
    "",
    [...counts.values()]
      .sort((a, b) => b.n - a.n)
      .map((c) => row([itemName(c.item), nameOf(c.place), String(c.n), percent(c.practice, c.n)], (i) => i > 1)),
  );
  $("items-used").replaceChildren(counts.size === 0 ? "" : element("thead", "", head), body);
}

function drawDetail(): void {
  const section = $("detail");
  if (selected === null) {
    section.hidden = true;
    return;
  }
  section.hidden = false;
  $("detail-title").textContent = nameOf(selected);

  const byEntry = model.cells.get(selected);
  const reachable = new Set(
    (plan?.costed ?? []).filter((c) => c.place === selected && c.source !== "data").map((c) => c.entry),
  );
  $("detail-note").textContent =
    reachable.size === 0
      ? ""
      : `The search could walk in here as ${[...reachable].map((k) => entryLabel(parseEntryKey(k))).join(", ")}, which the history has no clears for.`;

  const head = row(
    ["Walked in as", "Clears", "Deaths", "Death rate", "Best", "Median", "Mean", "Spread", "Leaves you", "Seen", "Practice"],
    (i) => i > 0 && i < 8,
    "th",
  );
  const rows = [...(byEntry?.values() ?? [])].map((cell) => {
    const s = summary(selected!, entryKey(cell.entry))!;
    const exits = POWERS.filter((p) => (s.exits[p] ?? 0) > 0)
      .map((p) => `${entryLabel({ power: p, star: false, pwing: false } as Entry)} ${percent(s.exits[p]!, s.clears)}`)
      .join(", ");
    return row(
      [
        entryLabel(cell.entry),
        String(s.clears),
        String(s.deaths),
        percent(s.deaths, s.clears + s.deaths),
        orDash(s.best),
        orDash(s.median),
        orDash(s.mean),
        s.spread === null ? "–" : `${Math.round(s.spread * 100)}%`,
        exits || "–",
        percent(s.exitsSeen, s.clears),
        percent(s.practice, s.clears + s.deaths),
      ],
      (i) => i > 0 && i < 8,
    );
  });
  $("detail-table").replaceChildren(
    element("thead", "", head),
    element("tbody", "", rows.length > 0 ? rows : [row(["Never played."])]),
  );
}

function drawWorldSection(): void {
  drawTabs();
  drawWorld(
    $("map"),
    world,
    placeInfo(),
    plan?.steps ?? [],
    selected,
    (place) => {
      selected = selected === place ? null : place;
      render();
    },
  );
  drawStagesTable();
  drawItems();
  drawDetail();
}

function render(): void {
  renderStatus();
  $("result").hidden = plan === null;
  if (plan !== null) {
    drawTiles(plan);
    drawRoute(plan);
  }
  drawHoles();
  drawWorldSection();
}

/** The history changed: work it out again, and search again once it settles. */
function changed(): void {
  describeSources();
  rebuild();
  render();
  searchSoon();
}

// ---------------------------------------------------------------------------
// Reading the history in

function merge(rows: readonly unknown[]): boolean {
  let any = false;
  for (const r of rows) {
    const attempt = parseAttempt(r);
    if (attempt === null) continue;
    const held = local.get(attempt.id);
    if (held !== undefined && held.events.length === attempt.events.length && held.ended?.epochMilliseconds === attempt.ended?.epochMilliseconds) {
      continue;
    }
    local.set(attempt.id, attempt);
    any = true;
  }
  return any;
}

async function reload(): Promise<boolean> {
  const rows = await readHistory();
  if (rows === null) return false;
  if (merge(rows)) changed();
  return true;
}

async function start(): Promise<void> {
  settingsToForm();

  const form = $<HTMLFormElement>("settings");
  form.addEventListener("change", () => {
    settings = settingsFromForm();
    saveSettings();
    settingsToForm();
    runSearch();
  });
  form.addEventListener("submit", (e) => e.preventDefault());
  $("reset-settings").addEventListener("click", () => {
    settings = DEFAULTS;
    saveSettings();
    settingsToForm();
    runSearch();
  });

  $<HTMLSelectElement>("filter-days").addEventListener("change", (e) => {
    days = Number((e.target as HTMLSelectElement).value);
    changed();
  });

  $("holes-more").addEventListener("click", () => {
    holesShown += 25;
    drawHoles();
  });

  const file = $<HTMLInputElement>("file");
  $("open").addEventListener("click", () => file.click());
  file.addEventListener("change", async () => {
    for (const one of file.files ?? []) {
      for (const attempt of parseLines(await one.text())) opened.set(attempt.id, attempt);
    }
    file.value = "";
    changed();
  });

  if (typeof Temporal === "undefined") {
    $("source").textContent =
      "This browser has no Temporal, which the history is kept with, so there is none to read here. The route below assumes every stage.";
  } else {
    try {
      new BroadcastChannel(HISTORY_CHANNEL).addEventListener("message", (e) => {
        if (Array.isArray(e.data) && merge(e.data)) changed();
      });
    } catch {
      /* No live updates: coming back to the tab still reloads, below. */
    }
    document.addEventListener("visibilitychange", () => {
      if (!document.hidden) reload();
    });

    if (!(await reload())) {
      $("source").textContent = "This browser is not keeping a history. Open an exported one to read it here.";
    }
  }

  describeSourcesIfEmpty();
  rebuild();
  render();
  runSearch();
}

/** An empty history still has to say so. */
function describeSourcesIfEmpty(): void {
  if (local.size === 0 && opened.size === 0 && $("source").textContent === "Reading the run history…") {
    describeSources();
  }
}

start();
