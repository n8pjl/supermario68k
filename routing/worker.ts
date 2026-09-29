// The route search, off the page's thread: it can take a moment, and the page
// is meant to stay usable - and to keep following a game being played in
// another tab - while it does.
//
// The search is Rust, built to routing-search.wasm beside this file (see
// search/ and the Makefile). It is handed the maps, the history's figures and
// the settings as one JSON document, and hands back the plan as another.
//
// One message in, the history's figures and the settings, and where a run in
// progress stands if the route is to follow one; one message out, the plan or
// why there is none.

import type { Start } from "./live.ts";
import { MAPS } from "./maps.ts";
import { type Stats } from "./model.ts";
import { type Plan, type Settings } from "./route.ts";

export interface Request {
  readonly id: number;
  readonly stats: Stats;
  readonly settings: Settings;
  /** The run in progress to route the rest of; null, a new game. */
  readonly from: Start | null;
}

export type Reply =
  | { readonly id: number; readonly plan: Plan }
  | { readonly id: number; readonly error: string };

interface Exports {
  readonly memory: WebAssembly.Memory;
  alloc(len: number): number;
  route(ptr: number, len: number): number;
  reply_len(): number;
}

// Streamed, so it compiles as it downloads. That needs the server to send it as
// application/wasm; one that does not fails here, and the page says why.
const search = WebAssembly.instantiateStreaming(fetch(new URL("./routing-search.wasm", import.meta.url))).then(
  ({ instance }) => instance.exports as unknown as Exports,
);

/**
 * JSON has no Infinity, so the search writes a time that cannot be made - a
 * stage set to be avoided, a choice with no way on - as null. These are the
 * fields that only ever hold a time.
 */
const TIMES = new Set(["ms", "deltaMs", "left", "walkMs", "doMs", "total"]);

/** The plan, less how long it took, which is timed out here; or why there is none. */
function run(wasm: Exports, stats: Stats, settings: Settings, from: Start | null): Omit<Plan, "ms"> | string {
  const request = new TextEncoder().encode(JSON.stringify({ maps: MAPS, stats, settings, from }));
  const ptr = wasm.alloc(request.length);
  new Uint8Array(wasm.memory.buffer, ptr, request.length).set(request);

  const at = wasm.route(ptr, request.length);
  // Read after the call: memory the search grew into is a new buffer.
  const text = new TextDecoder().decode(new Uint8Array(wasm.memory.buffer, at, wasm.reply_len()));
  const reply = JSON.parse(text, (key, value) => (value === null && TIMES.has(key) ? Infinity : value)) as
    | { plan: Omit<Plan, "ms"> }
    | { error: string };
  return "error" in reply ? reply.error : reply.plan;
}

addEventListener("message", async (e: MessageEvent<Request>) => {
  const { id, stats, settings, from } = e.data;
  let reply: Reply;

  try {
    const wasm = await search;
    const began = performance.now();
    const out = run(wasm, stats, settings, from);
    reply = typeof out === "string" ? { id, error: out } : { id, plan: { ...out, ms: performance.now() - began } };
  } catch (error) {
    reply = { id, error: error instanceof Error ? error.message : String(error) };
  }
  postMessage(reply);
});
