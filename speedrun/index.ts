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

import { CATEGORIES } from "./category.ts";
import { type GameEvent } from "./events.ts";
import { SpeedrunManager } from "./manage.ts";
import { emptyRecord } from "./records.ts";
import { SpeedrunPanel } from "./panel.ts";
import { type Route } from "./route.ts";
import { SpeedrunStore } from "./store.ts";
import { SpeedrunTimer } from "./timer.ts";

export { type GameEvent } from "./events.ts";
export { type Route } from "./route.ts";

/** An entry in the route picker that is not a route: why there is not one. */
function placeholder(text: string): HTMLOptionElement {
  const option = document.createElement("option");

  option.value = "";
  option.textContent = text;
  option.disabled = true;
  return option;
}

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
   * because recording a route is done by a run that is being timed - and it is
   * built for the selected category either way, which is what such a recording
   * is written for.
   */
  #build(): SpeedrunTimer {
    const route = this.#store.selected;
    const timer = new SpeedrunTimer(
      this.#store.category.id,
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
      // Recorded for whichever category is selected - the player said what they
      // were attempting before they started - and filed elsewhere only if the
      // run turned out not to obey it.
      const recorded = this.#timer.takeRecording(
        `Recorded ${new Date().toLocaleDateString()}`,
      );

      if (recorded === null) {
        this.#manager.recordingEmpty();
      } else {
        this.#manager.recorded(recorded.route, recorded.record);
      }
      return;
    }

    // Times set with no route selected belong to no route, so there is nowhere
    // to write them down; the run was a clock and nothing more.
    if (this.#timer.route !== null) this.#store.putRecord(this.#timer.record);
  }

  /**
   * The route picker: every route there is, under the category it is run for.
   *
   * All the categories rather than only the selected one, so that the picker
   * beside "Start game" is the whole of the choice - picking a route here picks
   * the rules with it, and the routes section does not have to be visited to
   * change what is being run.
   */
  #fillRoutes(): void {
    // Nothing recorded or imported yet. An empty picker offers no explanation
    // for being empty, so it says what is missing instead.
    if (this.#store.routes.length === 0) {
      this.#routes.replaceChildren(placeholder("No routes yet"));
      this.#routes.selectedIndex = 0;
      return;
    }

    // A category with nothing in it is left out rather than shown empty: it is
    // in the routes section, which is where it can be recorded into.
    const groups: (HTMLOptGroupElement | HTMLOptionElement)[] =
      CATEGORIES.flatMap((one) => {
        const routes = this.#store.routesIn(one.id);
        if (routes.length === 0) return [];

        const group = document.createElement("optgroup");
        group.label = one.name;
        group.append(
          ...routes.map((route) => {
            const option = document.createElement("option");

            option.value = route.id;
            option.textContent = route.name;
            return option;
          }),
        );

        return [group];
      });

    const selected = this.#store.selected;

    // The selected category is empty while others are not, so there is nothing
    // to be running. Said rather than left blank: an unset picker with routes
    // in it reads as a picker that failed to load one.
    if (selected === null) {
      groups.unshift(placeholder(`No ${this.#store.category.name} routes yet`));
    }

    this.#routes.replaceChildren(...groups);

    // Index rather than value for the placeholder: it is disabled, and a
    // browser will not settle on a disabled option by itself - the picker would
    // sit there blank, saying nothing at all where the whole point of the entry
    // is to say why there is nothing to pick.
    if (selected === null) {
      this.#routes.selectedIndex = 0;
    } else {
      this.#routes.value = selected.id;
    }
  }

  #draw(): void {
    this.#panel.draw(this.#timer.view());
  }

  /**
   * The Module.onSpeedrunEvent hook: one event from the game.
   *
   * Answers true when the game should stop where it stands, which is only ever
   * a recording that has just written the last split its category has. The
   * panel is showing a finished route at that point and the game is not part of
   * the run any more, so it is ended rather than left running with the player
   * holding the keys; the game unwinds out of the level and the page takes its
   * keyboard back. Anything else is answered false and the game plays on.
   *
   * The timer is held across the call rather than read back off the field
   * afterwards: saving the route it just recorded rebuilds the timer, and it is
   * the one that did the recording that knows how it ended.
   */
  handle(event: GameEvent): boolean {
    const timer = this.#timer;

    timer.handle(event);
    return timer.completedRecording;
  }

  get selected(): Route | null {
    return this.#store.selected;
  }
}
