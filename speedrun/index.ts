// The speedrun timer, assembled.
//
// The game reports what happened and nothing more - a run started, a level was
// beaten, Bowser went down; see src/speedrun.h for the events and where they
// are raised. What any of it means for the clock is decided by the route being
// run, which is data: an ordered list of splits, each closed by one kind of
// event. Routes are recorded, saved, exported and imported as JSON, so a route
// and the personal best set on it move between machines as one file.
//
// This is the whole of what the page outside speedrun/ talks to.

import { type GameEvent } from "./events.ts";
import { SpeedrunManager } from "./manage.ts";
import { emptyRecord } from "./records.ts";
import { SpeedrunPanel } from "./panel.ts";
import { type Route } from "./route.ts";
import { SpeedrunStore } from "./store.ts";
import { SpeedrunTimer } from "./timer.ts";

export { type GameEvent } from "./events.ts";
export { type Route } from "./route.ts";

export interface SpeedrunElements {
  /** The split panel beside the screen. */
  readonly panel: HTMLElement;
  /** The section that curates routes and times. */
  readonly manage: HTMLElement;
  /** Which route is being run, in the settings menu. */
  readonly routes: HTMLSelectElement;
}

export class Speedrun {
  readonly #store: SpeedrunStore;
  readonly #panel: SpeedrunPanel;
  readonly #manager: SpeedrunManager;
  readonly #routes: HTMLSelectElement;

  #timer: SpeedrunTimer;
  #recording = false;

  constructor(elements: SpeedrunElements) {
    this.#store = new SpeedrunStore();
    this.#panel = new SpeedrunPanel(elements.panel);
    this.#routes = elements.routes;

    this.#timer = this.#build();

    this.#manager = new SpeedrunManager(elements.manage, this.#store, {
      setRecording: (recording) => {
        this.#recording = recording;
        this.#timer.arm(recording);
        this.#draw();
      },
      changed: () => {
        // Not while a run is going: rebuilding would throw away the run in
        // progress, and the routes section is reachable during a game so that
        // a recording can be stopped from it.
        if (!this.#timer.running) this.#timer = this.#build();

        this.#fillRoutes();
        this.#manager.draw();
        this.#draw();
      },
    });

    this.#routes.addEventListener("change", () => {
      this.#store.select(this.#routes.value);
      this.#timer = this.#build();
      this.#manager.draw();
      this.#draw();
    });

    this.#fillRoutes();
    this.#draw();
  }

  /**
   * A timer for whichever route is selected, with that route's times.
   *
   * There may be no route: the game ships none, so until one is recorded or
   * imported the timer has a clock and nothing to split on. It is still built,
   * because recording a route is done by a run that is being timed.
   */
  #build(): SpeedrunTimer {
    const route = this.#store.selected;
    const timer = new SpeedrunTimer(
      route,
      route === null ? emptyRecord("") : this.#store.recordFor(route.id),
      () => this.#onTimerChanged(),
    );

    timer.arm(this.#recording);
    return timer;
  }

  #onTimerChanged(): void {
    this.#draw();

    if (!this.#timer.settled) return;

    // A run that has stopped has either set times worth keeping or written a
    // route worth saving, and neither is the timer's to store.
    if (this.#timer.recordedRun) {
      // A recorded route joins the category of the one it was recorded
      // alongside, there being nothing else to go on; the first route recorded
      // on a fresh copy of the game has nothing beside it at all.
      const recorded = this.#timer.takeRecording(
        `Recorded ${new Date().toLocaleDateString()}`,
        this.#store.selected?.category ?? "Any%",
      );

      if (recorded === null) {
        this.#manager.recordingEmpty();
      } else {
        this.#manager.recorded(recorded);
      }
      return;
    }

    // Times set with no route selected belong to no route, so there is nowhere
    // to write them down; the run was a clock and nothing more.
    if (this.#timer.route !== null) this.#store.putRecord(this.#timer.record);
  }

  #fillRoutes(): void {
    const routes = this.#store.routes;

    // Nothing recorded or imported yet. An empty picker offers no explanation
    // for being empty, so it says what is missing instead.
    if (routes.length === 0) {
      const none = document.createElement("option");

      none.textContent = "No routes yet";
      none.disabled = true;
      this.#routes.replaceChildren(none);
      return;
    }

    const groups = new Map<string, HTMLOptGroupElement>();

    for (const route of routes) {
      let group = groups.get(route.category);

      if (group === undefined) {
        group = document.createElement("optgroup");
        group.label = route.category;
        groups.set(route.category, group);
      }

      const option = document.createElement("option");
      option.value = route.id;
      option.textContent = route.name;
      group.append(option);
    }

    this.#routes.replaceChildren(...groups.values());
    this.#routes.value = this.#store.selected?.id ?? "";
  }

  #draw(): void {
    this.#panel.draw(this.#timer.view());
  }

  /** The Module.onSpeedrunEvent hook: one event from the game. */
  handle(event: GameEvent): void {
    this.#timer.handle(event);
  }

  get selected(): Route | null {
    return this.#store.selected;
  }
}
