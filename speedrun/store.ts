// Where routes and times are kept, and the file they are carried between
// machines in.
//
// Browser storage is per-origin and per-machine, which is exactly the thing an
// exported document is for: the same routes and the same personal bests, moved
// by hand to wherever the game is being played next. Storage being unavailable
// - private browsing, blocked cookies - costs the saving, not the timing: a run
// is still timed, it just is not kept.

import {
  FORMAT,
  FORMAT_VERSION,
  type RouteRecord,
  emptyRecord,
  parseRouteRecord,
  recordToJSON,
} from "./records.ts";
import { BUILT_IN_ROUTES, type Route, parseRoute, routeToJSON } from "./route.ts";

const ROUTES_KEY = "sm68k.speedrun.routes";
const RECORDS_KEY = "sm68k.speedrun.records";
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
 * The shipped routes are not stored: they come from the code every time, so a
 * later version of the game can improve one without a stale copy in storage
 * standing in its way. Only what the player made or imported is written down.
 */
export class SpeedrunStore {
  #routes: Route[];
  #records: Map<string, RouteRecord>;
  #selected: string;

  constructor() {
    this.#routes = [];
    for (const entry of Array.isArray(read(ROUTES_KEY)) ? (read(ROUTES_KEY) as unknown[]) : []) {
      const route = parseRoute(entry);
      if (route !== null) this.#routes.push(route);
    }

    this.#records = new Map();
    for (const entry of Array.isArray(read(RECORDS_KEY)) ? (read(RECORDS_KEY) as unknown[]) : []) {
      const record = parseRouteRecord(entry);
      if (record !== null) this.#records.set(record.route, record);
    }

    const selected = read(SELECTED_KEY);
    this.#selected =
      typeof selected === "string" && this.find(selected) !== null
        ? selected
        : BUILT_IN_ROUTES[0]!.id;
  }

  /** The shipped routes first, then the player's, which is how they are listed. */
  get routes(): readonly Route[] {
    return [...BUILT_IN_ROUTES, ...this.#routes];
  }

  find(id: string): Route | null {
    return this.routes.find((route) => route.id === id) ?? null;
  }

  get selected(): Route {
    return this.find(this.#selected) ?? BUILT_IN_ROUTES[0]!;
  }

  select(id: string): void {
    if (this.find(id) === null) return;

    this.#selected = id;
    write(SELECTED_KEY, id);
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

  /** Adds a route, or replaces the one already held under its id. */
  putRoute(route: Route): void {
    if (this.find(route.id)?.builtIn) return;

    const at = this.#routes.findIndex((held) => held.id === route.id);

    if (at < 0) {
      this.#routes.push(route);
    } else {
      this.#routes[at] = route;
    }

    this.#saveRoutes();
  }

  /** Drops a route and the times set on it; a shipped one cannot be dropped. */
  removeRoute(id: string): void {
    if (this.find(id)?.builtIn) return;

    this.#routes = this.#routes.filter((route) => route.id !== id);
    this.#records.delete(id);
    this.#saveRoutes();
    this.#saveRecords();
  }

  /**
   * Take in a document: anything held under the same id is replaced, and
   * everything else is left where it is. That is what makes it safe to import
   * a file from another machine without losing what is on this one.
   */
  merge(doc: SpeedrunDocument): { routes: number; records: number } {
    let routes = 0;

    for (const route of doc.routes) {
      // A shipped route cannot be overwritten by a file, but the times set on
      // one can be imported, which is the case worth having.
      if (this.find(route.id)?.builtIn) continue;

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
      routes: routes.filter((route) => route.builtIn !== true),
      records: routes
        .map((route) => this.#records.get(route.id))
        .filter((record) => record !== undefined),
    };
  }

  #saveRoutes(): void {
    write(ROUTES_KEY, this.#routes.map(routeToJSON));
  }

  #saveRecords(): void {
    write(RECORDS_KEY, [...this.#records.values()].map(recordToJSON));
  }
}
