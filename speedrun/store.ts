// Where routes and times are kept, and the file they are carried between
// machines in.
//
// Browser storage is per-origin and per-machine, which is exactly the thing an
// exported document is for: the same routes and the same personal bests, moved
// by hand to wherever the game is being played next. Storage being unavailable
// - private browsing, blocked cookies - costs the saving, not the timing: a run
// is still timed, it just is not kept.

import {
  CATEGORIES,
  type Category,
  type CategoryId,
  FALLBACK_CATEGORY,
  category,
  isCategoryId,
} from "./category.ts";
import {
  FORMAT,
  FORMAT_VERSION,
  type RouteRecord,
  emptyRecord,
  parseRouteRecord,
  recordToJSON,
} from "./records.ts";
import { type Route, parseRoute, routeToJSON } from "./route.ts";

const ROUTES_KEY = "sm68k.speedrun.routes";
const RECORDS_KEY = "sm68k.speedrun.records";
const CATEGORY_KEY = "sm68k.speedrun.category";
const SELECTED_KEY = "sm68k.speedrun.route";

function read(key: string): unknown {
  try {
    const raw = localStorage.getItem(key);

    return raw === null ? null : JSON.parse(raw);
  } catch {
    return null;
  }
}

function write(key: string, value: unknown): void {
  try {
    localStorage.setItem(key, JSON.stringify(value));
  } catch {
    /* Nothing to be done, and nothing that depends on it having worked. */
  }
}

export interface SpeedrunDocument {
  readonly routes: readonly Route[];
  readonly records: readonly RouteRecord[];
}

export function parseDocument(value: unknown): SpeedrunDocument | null {
  if (typeof value !== "object" || value === null) return null;

  const raw = value as Record<string, unknown>;

  // The format tag is what tells one of these from any other JSON file that
  // happens to have a "routes" key. A newer version is refused rather than read
  // hopefully: what it means by a field this one knows is not this one's to
  // guess.
  if (raw["format"] !== FORMAT) return null;
  if (typeof raw["version"] !== "number" || raw["version"] > FORMAT_VERSION) {
    return null;
  }

  const routes: Route[] = [];
  for (const entry of Array.isArray(raw["routes"]) ? raw["routes"] : []) {
    const route = parseRoute(entry);
    if (route !== null) routes.push(route);
  }

  const records: RouteRecord[] = [];
  for (const entry of Array.isArray(raw["records"]) ? raw["records"] : []) {
    const record = parseRouteRecord(entry);
    if (record !== null) records.push(record);
  }

  return { routes, records };
}

export function documentToJSON(doc: SpeedrunDocument): string {
  return JSON.stringify(
    {
      format: FORMAT,
      version: FORMAT_VERSION,
      routes: doc.routes.map(routeToJSON),
      records: doc.records.map(recordToJSON),
    },
    null,
    2,
  );
}

/**
 * Everything the timer knows about, held together so the two stay in step.
 *
 * Routes are held under categories: the categories are hardcoded and always all
 * there, and the routes under them are whatever the player has recorded or
 * imported. The game ships no routes - a route is a claim about how the game is
 * run, and the only honest source for that is a run somebody played - so there
 * is a state with a category selected and nothing in it, which is the state the
 * game is in the first time it is opened, and everything that reads a route has
 * to have an answer for it.
 */
export class SpeedrunStore {
  /**
   * The routes, under the category each one is run for.
   *
   * Every category has an entry, empty or not: the categories are the fixed
   * thing here and the routes are what comes and goes, so a category with
   * nothing in it is a normal state rather than a missing key.
   */
  readonly #routes: Map<CategoryId, Route[]>;
  #records: Map<string, RouteRecord>;

  /** The rules being run. There is always one - they are hardcoded. */
  #category: CategoryId;
  /**
   * The route last chosen in each category.
   *
   * Kept per category rather than as one choice, so that going to warpless to
   * try a route and coming back lands on the Any% route that was being run,
   * not on whichever one happens to sort first.
   */
  readonly #selected: Map<CategoryId, string>;

  constructor() {
    this.#routes = new Map(CATEGORIES.map((one) => [one.id, []]));

    const stored = read(ROUTES_KEY);
    for (const entry of Array.isArray(stored) ? stored : []) {
      const route = parseRoute(entry);
      if (route !== null) this.#bucket(route.category).push(route);
    }

    this.#records = new Map();
    const records = read(RECORDS_KEY);
    for (const entry of Array.isArray(records) ? records : []) {
      const record = parseRouteRecord(entry);
      if (record !== null) this.#records.set(record.route, record);
    }

    const chosen = read(CATEGORY_KEY);
    this.#category = isCategoryId(chosen) ? chosen : FALLBACK_CATEGORY.id;
    this.#selected = new Map();

    const selected = read(SELECTED_KEY);

    if (typeof selected === "object" && selected !== null) {
      for (const [id, route] of Object.entries(selected)) {
        if (isCategoryId(id) && typeof route === "string") {
          this.#selected.set(id, route);
        }
      }
    }
  }

  #bucket(id: CategoryId): Route[] {
    let held = this.#routes.get(id);

    if (held === undefined) {
      held = [];
      this.#routes.set(id, held);
    }

    return held;
  }

  /** Every route there is, category by category. */
  get routes(): readonly Route[] {
    return CATEGORIES.flatMap((one) => this.#routes.get(one.id) ?? []);
  }

  /** The routes filed under one category, which may be none. */
  routesIn(id: CategoryId): readonly Route[] {
    return this.#routes.get(id) ?? [];
  }

  find(id: string): Route | null {
    return this.routes.find((route) => route.id === id) ?? null;
  }

  /** The rules being run. Never null: a category is always selected. */
  get category(): Category {
    return category(this.#category);
  }

  selectCategory(id: CategoryId): void {
    this.#category = id;
    write(CATEGORY_KEY, id);
  }

  /**
   * The route being run, or null while the selected category holds none.
   *
   * Falls back to the first route in the category rather than to nothing when
   * the stored choice has been deleted or moved elsewhere: something is
   * selected whenever there is anything to select, so the picker is never empty
   * while the category has routes.
   */
  get selected(): Route | null {
    const held = this.routesIn(this.#category);
    const chosen = this.#selected.get(this.#category);

    return held.find((route) => route.id === chosen) ?? held[0] ?? null;
  }

  /** Chooses a route, and with it the category that route is filed under. */
  select(id: string): void {
    const route = this.find(id);
    if (route === null) return;

    this.selectCategory(route.category);
    this.#selected.set(route.category, route.id);
    this.#saveSelected();
  }

  recordFor(id: string): RouteRecord {
    return this.#records.get(id) ?? emptyRecord(id);
  }

  putRecord(record: RouteRecord): void {
    this.#records.set(record.route, record);
    this.#saveRecords();
  }

  clearRecord(id: string): void {
    this.#records.delete(id);
    this.#saveRecords();
  }

  /**
   * Adds a route, or replaces the one already held under its id.
   *
   * The replacement is looked for everywhere rather than in its own category,
   * because the two need not be the same one: a route that comes back from a
   * file under different rules moves, taking its times with it.
   */
  putRoute(route: Route): void {
    this.#drop(route.id);
    this.#bucket(route.category).push(route);
    this.#saveRoutes();
  }

  /** Drops a route and the times set on it. */
  removeRoute(id: string): void {
    this.#drop(id);
    this.#records.delete(id);
    this.#saveRoutes();
    this.#saveRecords();
  }

  /** Takes a route out of whichever category is holding it. */
  #drop(id: string): void {
    for (const held of this.#routes.values()) {
      const at = held.findIndex((route) => route.id === id);

      if (at >= 0) held.splice(at, 1);
    }
  }

  /**
   * Take in a document: anything held under the same id is replaced, and
   * everything else is left where it is. That is what makes it safe to import
   * a file from another machine without losing what is on this one.
   */
  merge(doc: SpeedrunDocument): { routes: number; records: number } {
    let routes = 0;

    for (const route of doc.routes) {
      this.putRoute(route);
      routes++;
    }

    for (const record of doc.records) this.#records.set(record.route, record);
    this.#saveRecords();

    return { routes, records: doc.records.length };
  }

  /** What to export: one route and its times, or the lot. */
  document(only?: Route): SpeedrunDocument {
    const routes = only === undefined ? this.routes : [only];

    return {
      routes,
      records: routes
        .map((route) => this.#records.get(route.id))
        .filter((record) => record !== undefined),
    };
  }

  #saveRoutes(): void {
    write(ROUTES_KEY, this.routes.map(routeToJSON));
  }

  #saveRecords(): void {
    write(RECORDS_KEY, [...this.#records.values()].map(recordToJSON));
  }

  #saveSelected(): void {
    write(SELECTED_KEY, Object.fromEntries(this.#selected));
  }
}
