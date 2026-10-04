// One running copy of the TAS build of the game, held between frames.
//
// The game runs until it reaches its next frame boundary and suspends there,
// in src/tas.cpp's tas_suspend(), until step() lets it go on. While it is
// suspended it is nothing but linear memory and two registers (see src/tas.h),
// which is what capture() copies and restore() writes back.
//
// Frames are counted the way the movie counts them. The game is "at frame n"
// when it has played n frames and is waiting to play frame n, whose input is
// the movie's line n. Frame 0 is power-on: main() suspends once before doing
// anything at all, so the very start is a snapshot like any other.

import { actions, type Calc, type Input } from "./movie.ts";

interface Registers {
  readonly sp: number;
  readonly data: number;
}

/** What src/tas-runtime.js puts on the module as Module.tasRuntime. */
interface Runtime {
  memory(): Uint8Array;
  heapEnd(): number;
  registers(): Registers;
  setRegisters(registers: Registers): void;
  refresh(): void;
}

/** Emscripten's factory: the default export of mario-tas.js. */
export type Factory = (module: object) => Promise<unknown>;

/** The part of the module object this keeps a hold of. */
interface Module {
  tas: { drawing: boolean };
}

/**
 * The game at one frame boundary. Memory is kept as the pages that differ
 * from power-on, which is most of what a snapshot would otherwise be: the game
 * data compiled into the wasm is the larger part of the heap and never changes.
 */
export interface Snapshot {
  readonly frame: number;
  readonly period: number;
  readonly registers: Registers;
  /** Where the snapshot's memory ends, rounded up to a page. */
  readonly end: number;
  /** The index of each page that differs from power-on, ascending. */
  readonly pages: Uint32Array;
  /** Those pages' bytes, back to back. */
  readonly bytes: Uint8Array;
}

const PAGE = 4096;

function pageEnd(address: number): number {
  return Math.ceil(address / PAGE) * PAGE;
}

export interface MachineOptions {
  factory: Factory;
  wasm: ArrayBuffer;
  canvas: HTMLCanvasElement;
  calc: Calc;
  /** ma_texts.json's `texts` for the movie's language. */
  texts: unknown;
  /** The input for a frame, asked for whenever the game reads its keys. */
  input(frame: number): Input;
  onCanvasResize(): void;
}

export class Machine {
  /** The frame the game is waiting to play. */
  frame = 0;

  /**
   * The time the scene asked this frame to take, in milliseconds: what the
   * game would have waited, which playback at speed paces itself by.
   */
  period = 0;

  /** Set once the game has stopped for good: main() returned or aborted. */
  ended: string | null = null;

  readonly #module: Module;
  readonly #runtime: Runtime;
  readonly #base: Uint8Array;
  #resume: (() => void) | null = null;
  #arrived: { resolve(): void; reject(e: Error): void } | null = null;
  #polled = false;

  private constructor(
    module: Module,
    runtime: Runtime,
    resume: () => void,
  ) {
    this.#module = module;
    this.#runtime = runtime;
    this.#resume = resume;
    this.#base = runtime.memory().slice(0, pageEnd(runtime.heapEnd()));
  }

  /** Starts the game and returns it at power-on, frame 0. */
  static async boot(options: MachineOptions): Promise<Machine> {
    let machine: Machine | null = null;
    let resume: (() => void) | null = null;
    let poweredOn!: () => void;
    let failed!: (e: Error) => void;
    const ready = new Promise<void>((resolve, reject) => {
      poweredOn = resolve;
      failed = reject;
    });

    const stop = (why: string) => {
      if (machine) {
        machine.ended = why;
        machine.#arrived?.reject(new Error(why));
        machine.#arrived = null;
      } else {
        failed(new Error(why));
      }
    };

    const module = {
      canvas: options.canvas,
      ti89Mode: options.calc === "ti89",
      maTexts: options.texts,
      wasmBinary: options.wasm,
      print: (t: string) => console.log(t),
      printErr: (t: string) => console.error(t),
      onAbort: (what: unknown) => stop(`the game aborted: ${String(what)}`),
      onExit: (status: number) => stop(`the game exited (status ${status})`),
      onCanvasResize: options.onCanvasResize,
      // Nothing reads the keys before power-on, which is the first thing
      // main() does: until then there is no frame for them to belong to.
      gameActions: () => {
        if (!machine) return actions(0);
        machine.#polled = true;
        return actions(options.input(machine.frame));
      },
      tas: {
        drawing: false,
        suspend: (boot: boolean, period: number) =>
          new Promise<void>((resolve) => {
            if (!machine) {
              resume = resolve;
              poweredOn();
              return;
            }
            if (!boot) machine.frame++;
            machine.period = period;
            machine.#resume = resolve;
            machine.#arrived?.resolve();
            machine.#arrived = null;
          }),
      },
    };

    // The factory's own promise only matters if it fails: the game is
    // running from inside it, and power-on is reached long before main()
    // could return.
    options.factory(module).catch((e: unknown) => stop(String(e)));
    await ready;

    const runtime = (module as { tasRuntime?: Runtime }).tasRuntime;
    if (!runtime || !resume) {
      throw new Error("the game build has no TAS runtime in it");
    }
    machine = new Machine(module, runtime, resume);
    return machine;
  }

  /** Whether the frames played from here on reach the canvas. */
  set drawing(on: boolean) {
    this.#module.tas.drawing = on;
  }

  /**
   * Plays the current frame, and resolves once the game is waiting at the
   * next one - with whether the game read its input on the way, which a frame
   * that did not (a lag frame) shows in the editor.
   */
  async step(): Promise<boolean> {
    if (this.ended) throw new Error(this.ended);

    const resume = this.#resume!;
    this.#resume = null;
    this.#polled = false;

    const arrived = new Promise<void>((resolve, reject) => {
      this.#arrived = { resolve, reject };
    });
    resume();
    await arrived;
    return this.#polled;
  }

  capture(): Snapshot {
    const memory = this.#runtime.memory();
    const end = Math.min(pageEnd(this.#runtime.heapEnd()), memory.length);
    const base = this.#base;
    const dirty: number[] = [];

    for (let at = 0; at < end; at += PAGE) {
      const page = new Uint32Array(memory.buffer, memory.byteOffset + at, PAGE / 4);

      if (at >= base.length) {
        if (page.some((word) => word !== 0)) dirty.push(at / PAGE);
        continue;
      }

      const was = new Uint32Array(base.buffer, base.byteOffset + at, PAGE / 4);
      for (let i = 0; i < page.length; i++) {
        if (page[i] !== was[i]) {
          dirty.push(at / PAGE);
          break;
        }
      }
    }

    const bytes = new Uint8Array(dirty.length * PAGE);
    dirty.forEach((page, i) => {
      bytes.set(memory.subarray(page * PAGE, (page + 1) * PAGE), i * PAGE);
    });

    return {
      frame: this.frame,
      period: this.period,
      registers: this.#runtime.registers(),
      end,
      pages: Uint32Array.from(dirty),
      bytes,
    };
  }

  /**
   * Puts the game back as it was at a snapshot. Only meaningful while it is
   * suspended, which is always, outside step(): the game resumes from the
   * snapshot's frame boundary the next time it is stepped, through whichever
   * suspension happens to be pending now.
   *
   * Everything above the break is zero in a game that has never been there,
   * and the game never writes above its break, so clearing up to whichever is
   * higher - the break now or the snapshot's - is all the memory the snapshot
   * does not hold.
   */
  restore(snapshot: Snapshot): void {
    const memory = this.#runtime.memory();
    const top = Math.max(pageEnd(this.#runtime.heapEnd()), snapshot.end);

    memory.set(this.#base, 0);
    if (top > this.#base.length) memory.fill(0, this.#base.length, top);
    snapshot.pages.forEach((page, i) => {
      memory.set(snapshot.bytes.subarray(i * PAGE, (i + 1) * PAGE), page * PAGE);
    });

    this.#runtime.setRegisters(snapshot.registers);
    this.frame = snapshot.frame;
    this.period = snapshot.period;
  }

  /** Repaints the canvas from what the game has on its screen right now. */
  refresh(): void {
    this.drawing = true;
    this.#runtime.refresh();
  }
}
