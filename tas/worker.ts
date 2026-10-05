// The game, run out of the page's way. Whatever the game does here - including
// never coming back from a frame - the page stays responsive, and can give up
// on this worker and start another: see machine.ts.
//
// Requests are answered one at a time, in the order they came.

import { Game, type Factory, type Snapshot } from "./game.ts";
import type { Calc, Input } from "./movie.ts";

/** How long one play request runs before it answers with what it has. */
const PLAY_SLICE_MS = 30;

export type Request =
  | {
      op: "boot";
      glue: string;
      wasm: ArrayBuffer;
      canvas: OffscreenCanvas;
      calc: Calc;
      texts: unknown;
    }
  | {
      op: "play";
      /** The input of each frame to play, from the current one on. */
      inputs: Input[];
      draw: boolean;
      /** The frames to take a snapshot at, once the game is waiting there. */
      capture: number[];
    }
  | { op: "capture" }
  | { op: "restore"; snapshot: Snapshot }
  | { op: "refresh" };

/** What playing one frame did. */
export interface Frame {
  /** Whether the game read its input: a frame that did not is a lag frame. */
  readonly polled: boolean;
  /** The kinds of speedrun event the game raised, in order. */
  readonly events: readonly string[];
  /** The period the game asked for while waiting to play this frame. */
  readonly period: number;
}

/** Where the game is after any request. */
export interface State {
  readonly frame: number;
  readonly period: number;
  readonly ended: string | null;
}

export interface Played extends State {
  /** Each frame played, in order: possibly fewer than were asked for. */
  readonly frames: Frame[];
  readonly snapshots: Snapshot[];
}

export type Message =
  | { type: "reply"; state: State; result?: unknown }
  | { type: "error"; state: State | null; message: string }
  | { type: "resize"; width: number; height: number };

let game: Game | null = null;
let canvas: OffscreenCanvas | null = null;
let inputs: Input[] = [];
let first = 0;

function state(): State {
  return { frame: game!.frame, period: game!.period, ended: game!.ended };
}

function transfer(snapshot: Snapshot): ArrayBuffer[] {
  return [snapshot.pages.buffer as ArrayBuffer, snapshot.bytes.buffer as ArrayBuffer];
}

async function boot(request: Extract<Request, { op: "boot" }>): Promise<Snapshot> {
  const factory = ((await import(request.glue)) as { default: Factory }).default;
  canvas = request.canvas;
  game = await Game.boot({
    factory,
    wasm: request.wasm,
    canvas,
    calc: request.calc,
    texts: request.texts,
    input: (frame) => inputs[frame - first] ?? 0,
    onCanvasResize: () => {
      const message: Message = { type: "resize", width: canvas!.width, height: canvas!.height };
      postMessage(message);
    },
  });
  return game.capture();
}

async function play(request: Extract<Request, { op: "play" }>): Promise<Played> {
  const g = game!;
  const frames: Frame[] = [];
  const snapshots: Snapshot[] = [];
  const capture = new Set(request.capture);
  const start = performance.now();

  inputs = request.inputs;
  first = g.frame;
  g.drawing = request.draw;

  try {
    while (frames.length < inputs.length) {
      const period = g.period;
      const played = await g.step();
      frames.push({ polled: played.polled, events: played.events, period });
      if (capture.has(g.frame)) snapshots.push(g.capture());
      if (performance.now() - start > PLAY_SLICE_MS) break;
    }
  } catch {
    // The game stopped for good: what it played until then still stands, and
    // the state says why it went no further.
  }
  return { ...state(), frames, snapshots };
}

async function handle(request: Request): Promise<void> {
  try {
    let result: unknown;
    let transfers: ArrayBuffer[] = [];

    switch (request.op) {
      case "boot": {
        const snapshot = await boot(request);
        result = snapshot;
        transfers = transfer(snapshot);
        break;
      }
      case "play": {
        const played = await play(request);
        result = played;
        transfers = played.snapshots.flatMap(transfer);
        break;
      }
      case "capture": {
        const snapshot = game!.capture();
        result = snapshot;
        transfers = transfer(snapshot);
        break;
      }
      case "restore":
        game!.restore(request.snapshot);
        break;
      case "refresh":
        game!.refresh();
        break;
    }

    const message: Message = { type: "reply", state: state(), result };
    postMessage(message, { transfer: transfers });
  } catch (e) {
    const message: Message = {
      type: "error",
      state: game ? state() : null,
      message: e instanceof Error ? e.message : String(e),
    };
    postMessage(message);
  }
}

let queue = Promise.resolve();
addEventListener("message", (e: MessageEvent<Request>) => {
  queue = queue.then(() => handle(e.data));
});
