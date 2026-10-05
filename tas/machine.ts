// The game as the page holds it: a copy of tas/game.ts running in a worker,
// reached by messages.
//
// The game runs in a worker so that it can never take the page down with it.
// A frame that never comes back - the game looping somewhere other than its
// frame boundary - would otherwise hang the whole tab. Here it only stops
// answering: once it has kept the page waiting longer than any frame could
// take, the worker is thrown away and the request fails with Hung, and the
// next thing asked of the machine starts a new one. A new game is the same
// game at power-on, so the snapshots taken of the old one restore into it.

import type { Player, Snapshot } from "./game.ts";
import type { Calc, Input } from "./movie.ts";
import type { Frame, Message, Played, Request, State } from "./worker.ts";

export type { Player, Snapshot, TasEvent } from "./game.ts";
export type { Frame } from "./worker.ts";

/** Beside this script, here and in dist/, where tools/mkdist.py renames it. */
const WORKER_URL = new URL("./tas-worker.js", import.meta.url);

/** How long a request may take before the game is taken to have hung. */
const TIMEOUT_MS = 5000;

/** Starting the game includes compiling it, which takes rather longer. */
const BOOT_TIMEOUT_MS = 60000;

/** The game took longer than it ever should over a request, and was stopped. */
export class Hung extends Error {
  constructor(readonly frame: number) {
    super(`The game stopped responding while playing on from frame ${frame}.`);
  }
}

export interface MachineOptions {
  /** The URL of the TAS build's glue, mario-tas.js. */
  glue: string;
  wasm: ArrayBuffer;
  calc: Calc;
  /** ma_texts.json's `texts` for the movie's language. */
  texts: unknown;
  /**
   * A canvas for a new game to draw on. Each game takes one over for good,
   * so this is asked for again each time the game is restarted.
   */
  canvas(): OffscreenCanvas;
  /** The game resized its screen. */
  onCanvasResize(width: number, height: number): void;
}

interface Pending {
  resolve(message: Message): void;
  reject(e: Error): void;
}

export class Machine {
  /** The frame the game is waiting to play. */
  frame = 0;

  /** The time the scene asked this frame to take, in milliseconds. */
  period = 0;

  /** Set once the game has stopped for good: main() returned or aborted. */
  ended: string | null = null;

  /** The player as the game has it now, or null outside a level. */
  player: Player | null = null;

  /** The game at power-on, frame 0. */
  powerOn!: Snapshot;

  readonly #options: MachineOptions;
  #worker: Worker | null = null;
  #pending: Pending | null = null;

  private constructor(options: MachineOptions) {
    this.#options = options;
  }

  /** Starts the game and returns it at power-on, frame 0. */
  static async boot(options: MachineOptions): Promise<Machine> {
    const machine = new Machine(options);
    machine.powerOn = await machine.#start();
    return machine;
  }

  /**
   * Throws the game away, as for one that hung: the next thing asked of the
   * machine starts a new one, which any snapshot restores into. How a game
   * that has ended - which nothing puts back - is had again.
   */
  restart(): void {
    this.#stop(new Error("the game was restarted"));
    this.ended = null;
  }

  /** Stops the game for good. */
  close(): void {
    this.#stop(new Error("the game was closed"));
  }

  #stop(why: Error): void {
    this.#worker?.terminate();
    this.#worker = null;
    this.#pending?.reject(why);
    this.#pending = null;
  }

  /**
   * A new worker with a new game in it, at power-on. Every game starts out
   * the same, so this is how a hung one is replaced as well as how the first
   * one is started.
   */
  async #start(): Promise<Snapshot> {
    const worker = new Worker(WORKER_URL, { type: "module" });
    worker.addEventListener("message", (e: MessageEvent<Message>) => {
      if (e.data.type === "resize") {
        this.#options.onCanvasResize(e.data.width, e.data.height);
        return;
      }
      const pending = this.#pending;
      this.#pending = null;
      pending?.resolve(e.data);
    });
    worker.addEventListener("error", (e) => {
      this.#stop(new Error(`the game's worker failed: ${e.message}`));
    });
    this.#worker = worker;
    this.ended = null;

    const canvas = this.#options.canvas();
    const request: Request = {
      op: "boot",
      glue: this.#options.glue,
      wasm: this.#options.wasm.slice(0),
      canvas,
      calc: this.#options.calc,
      texts: this.#options.texts,
    };
    return (await this.#ask(request, BOOT_TIMEOUT_MS, [canvas])) as Snapshot;
  }

  async #ask(request: Request, timeout: number, transfer: Transferable[] = []): Promise<unknown> {
    if (!this.#worker) await this.#start();

    const at = this.frame;
    const reply = new Promise<Message>((resolve, reject) => {
      this.#pending = { resolve, reject };
    });
    let timer: ReturnType<typeof setTimeout> | undefined;
    const timedOut = new Promise<never>((_, reject) => {
      timer = setTimeout(() => {
        const hung = new Hung(at);
        this.#stop(hung);
        reject(hung);
      }, timeout);
    });

    this.#worker!.postMessage(request, transfer);
    try {
      const message = await Promise.race([reply, timedOut]);
      if (message.type === "resize") throw new Error("unexpected resize");
      if (message.state) this.#update(message.state);
      if (message.type === "error") throw new Error(message.message);
      return message.result;
    } finally {
      clearTimeout(timer);
    }
  }

  #update(state: State): void {
    this.frame = state.frame;
    this.period = state.period;
    this.ended = state.ended;
    this.player = state.player;
  }

  /**
   * Plays frames from the current one with these inputs, drawing them or not,
   * and taking a snapshot at each of the frames in `capture` it reaches. The
   * game answers after a short while whether or not it got through them all,
   * so the page can show progress: what it did play is in the answer.
   */
  async play(
    inputs: Input[],
    draw: boolean,
    capture: number[],
  ): Promise<{ frames: Frame[]; snapshots: Snapshot[] }> {
    if (this.ended) throw new Error(this.ended);
    return (await this.#ask({ op: "play", inputs, draw, capture }, TIMEOUT_MS)) as Played;
  }

  async capture(): Promise<Snapshot> {
    return (await this.#ask({ op: "capture" }, TIMEOUT_MS)) as Snapshot;
  }

  /**
   * Puts the game back as it was at a snapshot - into a new game, if the old
   * one hung.
   */
  async restore(snapshot: Snapshot): Promise<void> {
    await this.#ask({ op: "restore", snapshot }, TIMEOUT_MS);
  }

  /** Repaints the canvas from what the game has on its screen right now. */
  async refresh(): Promise<void> {
    await this.#ask({ op: "refresh" }, TIMEOUT_MS);
  }
}
