// The input editor: one row per frame, one column per button, the way TAS
// tools have long laid a movie out. Clicking a button's cell toggles it, and
// dragging from there paints the same value down the column. Clicking a
// frame's number puts the game at that frame - before its input is played -
// and selects the row, which is what inserting and deleting act on.
//
// Frames are numbered from the start of the level they are in, and the last
// column is how far the player moved right on each: see Session.moved().
//
// Only the rows in view exist: a movie is tens of thousands of frames, and
// the editor is redrawn on every one played.

import { BUTTON_LABELS, BUTTON_LETTERS, BUTTONS, held, type Input } from "./movie.ts";

const ROW_HEIGHT = 20;

/** Rows past the end of the movie, for writing past it. */
const ROWS_PAST_END = 200;

/** What the editor shows. Read afresh on every render. */
export interface RollState {
  readonly inputs: readonly Input[];
  /** The frame the game is waiting to play. */
  readonly frame: number;
  readonly lag: readonly boolean[];
  /** Frames there are snapshots of. */
  readonly keyframes: ReadonlySet<number>;
  /** The bookmark slot of each bookmarked frame. */
  readonly bookmarks: ReadonlyMap<number, number>;
  readonly selected: number;
  /** A frame's number as it is shown, and as a title. */
  label(frame: number): { text: string; title: string };
  speed(frame: number): number | null;
}

/** The player's speed running with room to: PLAYER_RUNSPEED in src/player.h. */
export const FULL_SPEED = 4;

export interface RollActions {
  setInput(frame: number, input: Input): void;
  seek(frame: number): void;
  select(frame: number): void;
}

export class Roll {
  readonly #viewport: HTMLElement;
  readonly #sizer: HTMLElement;
  readonly #rows: HTMLElement[] = [];
  #state: RollState | null = null;
  #paint: { button: number; on: boolean; last: number } | null = null;

  constructor(
    host: HTMLElement,
    readonly actions: RollActions,
  ) {
    const head = document.createElement("div");
    head.className = "roll-row roll-head";
    head.append(cell("roll-frame", "Frame"));
    BUTTONS.forEach((button, i) => {
      const c = cell("roll-button", BUTTON_LETTERS[i]!);
      c.title = BUTTON_LABELS[button];
      head.append(c);
    });
    const speed = cell("roll-speed", "ΔX");
    speed.title = "How far the player's X moved on the frame, in pixels";
    head.append(speed);

    // The head scrolls with the rows, stuck to the top, so that the rows'
    // scrollbar narrows both alike and the columns stay lined up.
    this.#viewport = document.createElement("div");
    this.#viewport.className = "roll-viewport";
    this.#sizer = document.createElement("div");
    this.#sizer.className = "roll-sizer";
    this.#viewport.append(head, this.#sizer);

    host.append(this.#viewport);

    this.#viewport.addEventListener("scroll", () => this.#draw());
    new ResizeObserver(() => this.#draw()).observe(this.#viewport);

    this.#sizer.addEventListener("pointerdown", (e) => this.#down(e));
    this.#sizer.addEventListener("pointermove", (e) => this.#move(e));
    const up = () => {
      this.#paint = null;
    };
    this.#sizer.addEventListener("pointerup", up);
    this.#sizer.addEventListener("pointercancel", up);
  }

  render(state: RollState): void {
    this.#state = state;
    this.#draw();
  }

  /** Scrolls the current frame into view, if it is not already. */
  follow(frame: number): void {
    const top = frame * ROW_HEIGHT;
    const view = this.#viewport;
    const rows = view.clientHeight - ROW_HEIGHT;
    if (top < view.scrollTop || top + ROW_HEIGHT > view.scrollTop + rows) {
      view.scrollTop = Math.max(0, top - rows / 2);
    }
  }

  #draw(): void {
    const state = this.#state;
    if (!state) return;

    const count = Math.max(state.inputs.length, state.frame + 1) + ROWS_PAST_END;
    this.#sizer.style.height = `${count * ROW_HEIGHT}px`;

    const first = Math.floor(this.#viewport.scrollTop / ROW_HEIGHT);
    const visible = Math.ceil(this.#viewport.clientHeight / ROW_HEIGHT);
    const last = Math.min(count, first + visible);

    while (this.#rows.length < last - first) {
      const row = document.createElement("div");
      row.className = "roll-row";
      row.append(cell("roll-frame", ""));
      for (let i = 0; i < BUTTONS.length; i++) row.append(cell("roll-button", ""));
      row.append(cell("roll-speed", ""));
      this.#sizer.append(row);
      this.#rows.push(row);
    }

    this.#rows.forEach((row, i) => {
      const frame = first + i;
      if (frame >= last) {
        row.hidden = true;
        return;
      }
      row.hidden = false;
      row.style.transform = `translateY(${frame * ROW_HEIGHT}px)`;
      row.dataset["frame"] = String(frame);

      const input = state.inputs[frame] ?? 0;
      const lag = state.lag[frame];
      row.classList.toggle("current", frame === state.frame);
      row.classList.toggle("played", frame < state.frame);
      row.classList.toggle("lag", lag === true);
      row.classList.toggle("past-end", frame >= state.inputs.length);
      row.classList.toggle("selected", frame === state.selected);

      const label = row.children[0] as HTMLElement;
      const slot = state.bookmarks.get(frame);
      const { text, title } = state.label(frame);
      label.textContent = text;
      label.title = title;
      label.classList.toggle("keyframe", state.keyframes.has(frame));
      if (slot === undefined) delete label.dataset["bookmark"];
      else label.dataset["bookmark"] = String((slot + 1) % 10);

      for (let b = 0; b < BUTTONS.length; b++) {
        const c = row.children[b + 1] as HTMLElement;
        const on = held(input, b);
        c.classList.toggle("on", on);
        c.textContent = on ? BUTTON_LETTERS[b]! : "";
      }

      const speed = row.children[BUTTONS.length + 1] as HTMLElement;
      const moved = state.speed(frame);
      speed.textContent = moved === null ? "" : String(moved);
      speed.dataset["speed"] =
        moved === null ? "" : moved >= FULL_SPEED ? "full" : moved > 0 ? "slow" : "stopped";
    });
  }

  #at(e: PointerEvent): { frame: number; button: number } | null {
    const target = (e.target as HTMLElement).closest<HTMLElement>(".roll-row > *");
    const row = target?.parentElement;
    if (!target || !row?.dataset["frame"]) return null;

    return {
      frame: Number(row.dataset["frame"]),
      button: [...row.children].indexOf(target) - 1,
    };
  }

  #down(e: PointerEvent): void {
    const at = this.#at(e);
    const state = this.#state;
    if (!at || !state || e.button !== 0 || at.button >= BUTTONS.length) return;

    this.actions.select(at.frame);
    if (at.button < 0) {
      this.actions.seek(at.frame);
      return;
    }

    const input = state.inputs[at.frame] ?? 0;
    const on = !held(input, at.button);
    this.#paint = { button: at.button, on, last: at.frame };
    this.#set(at.frame, at.button, on);
    this.#sizer.setPointerCapture(e.pointerId);
  }

  /**
   * Under pointer capture the event's target stays the cell the drag began
   * in, so the frame is worked out from where the pointer is instead, and
   * every row between it and the last one painted is filled in: a fast drag
   * skips rows between events.
   */
  #move(e: PointerEvent): void {
    const paint = this.#paint;
    if (!paint) return;

    const y = e.clientY - this.#sizer.getBoundingClientRect().top;
    const frame = Math.max(0, Math.floor(y / ROW_HEIGHT));
    if (frame === paint.last) return;

    const step = frame > paint.last ? 1 : -1;
    for (let f = paint.last + step; f !== frame + step; f += step) {
      this.#set(f, paint.button, paint.on);
    }
    paint.last = frame;
  }

  #set(frame: number, button: number, on: boolean): void {
    const state = this.#state!;
    const input = state.inputs[frame] ?? 0;
    const next = on ? input | (1 << button) : input & ~(1 << button);
    if (next !== input) this.actions.setInput(frame, next);
  }
}

function cell(className: string, text: string): HTMLElement {
  const c = document.createElement("span");
  c.className = className;
  c.textContent = text;
  return c;
}
