// Curating routes and the times set on them: record a new one, take one in from
// a file, send one out to another machine, throw one away.
//
// Its own section of the page rather than part of the settings menu: the menu
// is what a run is started from and wants to stay small, and this is what is
// done between runs. Made inert while a game is running, the way the control
// legend is.

import { type Route, type RouteSplit } from "./route.ts";
import { type SpeedrunStore, documentToJSON, parseDocument } from "./store.ts";

const FILENAME = "sm68k-speedrun.json";

function button(label: string, className = ""): HTMLButtonElement {
  const el = document.createElement("button");

  el.type = "button";
  el.className = className;
  el.textContent = label;
  return el;
}

/**
 * Hand the browser a file to save.
 *
 * Built here and released again straight away: the blob is only needed for as
 * long as the click that reads it, and an object URL that is never revoked
 * holds its contents for the life of the document.
 */
function download(name: string, text: string): void {
  const url = URL.createObjectURL(
    new Blob([text], { type: "application/json" }),
  );
  const link = document.createElement("a");

  link.href = url;
  link.download = name;
  link.click();
  URL.revokeObjectURL(url);
}

export interface ManageHooks {
  /** Start or stop recording the next run rather than timing it. */
  readonly setRecording: (recording: boolean) => void;
  /** Routes or times changed underneath: redraw everything that lists them. */
  readonly changed: () => void;
}

export class SpeedrunManager {
  readonly #store: SpeedrunStore;
  readonly #hooks: ManageHooks;

  readonly #root: HTMLElement;
  readonly #routeName: HTMLElement;
  readonly #status: HTMLElement;
  readonly #editor: HTMLElement;
  readonly #routeInput: HTMLInputElement;
  readonly #splits: HTMLElement;
  readonly #record: HTMLButtonElement;
  readonly #remove: HTMLButtonElement;
  readonly #clear: HTMLButtonElement;
  readonly #exportOne: HTMLButtonElement;
  readonly #file: HTMLInputElement;

  #recording = false;
  /**
   * The route the split editor below is currently showing.
   *
   * The rows are rebuilt only when this changes, so that renaming a split -
   * which redraws everything - does not tear the field being typed in out from
   * under the caret.
   */
  #listed: string | null = null;
  /** The fields of that list, held rather than looked up again to write into. */
  #fields: HTMLInputElement[] = [];

  constructor(container: HTMLElement, store: SpeedrunStore, hooks: ManageHooks) {
    this.#store = store;
    this.#hooks = hooks;
    this.#root = container;

    const heading = document.createElement("h2");
    heading.textContent = "Speedrun routes";

    this.#routeName = document.createElement("p");
    this.#routeName.className = "sr-current";

    this.#record = button("Record a route");
    this.#remove = button("Delete route");
    this.#clear = button("Clear times");
    this.#exportOne = button("Export route");

    const exportAll = button("Export everything");
    const importFile = button("Import…");

    this.#file = document.createElement("input");
    this.#file.type = "file";
    this.#file.accept = "application/json,.json";
    this.#file.hidden = true;

    this.#status = document.createElement("p");
    this.#status.className = "sr-status";
    this.#status.setAttribute("role", "status");

    // Renaming, which is the whole of editing a route: what a split answers to
    // is recorded from the run and what it is called is not something the game
    // can know.
    this.#routeInput = document.createElement("input");
    this.#routeInput.type = "text";
    this.#routeInput.className = "sr-route-name";
    this.#routeInput.setAttribute("aria-label", "Route name");
    this.#routeInput.addEventListener("change", () => this.#renameRoute());

    this.#splits = document.createElement("ol");
    this.#splits.className = "sr-split-names";

    const editorHead = document.createElement("h3");
    editorHead.textContent = "Names";

    this.#editor = document.createElement("div");
    this.#editor.className = "sr-editor";
    this.#editor.append(editorHead, this.#routeInput, this.#splits);

    const row = document.createElement("div");
    row.className = "sr-actions";
    row.append(
      this.#record,
      this.#exportOne,
      exportAll,
      importFile,
      this.#clear,
      this.#remove,
    );

    container.replaceChildren(
      heading,
      this.#routeName,
      row,
      this.#file,
      this.#status,
      this.#editor,
    );

    this.#record.addEventListener("click", () => this.#toggleRecording());
    this.#exportOne.addEventListener("click", () => this.#exportSelected());
    exportAll.addEventListener("click", () => this.#export());
    importFile.addEventListener("click", () => this.#file.click());
    this.#file.addEventListener("change", () => void this.#import());
    this.#clear.addEventListener("click", () => this.#clearTimes());
    this.#remove.addEventListener("click", () => this.#removeRoute());

    this.draw();
  }

  #say(message: string): void {
    this.#status.textContent = message;
  }

  /** The selected route with one split's name replaced, saved and redrawn. */
  #rename(at: number, name: string): void {
    const route = this.#store.selected;
    if (route === null) return;

    const splits: RouteSplit[] = route.splits.map((split, i) =>
      i === at ? { ...split, name } : split,
    );

    this.#store.putRoute({ ...route, splits });
    this.#hooks.changed();
  }

  #renameRoute(): void {
    const route = this.#store.selected;
    const name = this.#routeInput.value.trim();

    if (route === null || name === "" || name === route.name) return;

    this.#store.putRoute({ ...route, name });
    this.#hooks.changed();
  }

  /**
   * One field per split, rebuilt only when the route being shown changes.
   *
   * Renaming redraws, and rebuilding on a redraw would drop the field mid-edit,
   * so the rows are reused and only their values written back. The change event
   * rather than input: a rename per keystroke would be a storage write per
   * keystroke, and a half-typed name saved at every step of the way.
   */
  #drawEditor(route: Route | null): void {
    this.#editor.hidden = route === null;
    if (route === null) return;

    this.#routeInput.value = route.name;

    if (this.#listed !== route.id || this.#fields.length !== route.splits.length) {
      this.#listed = route.id;
      this.#fields = route.splits.map((_, i) => {
        const field = document.createElement("input");

        field.type = "text";
        field.setAttribute("aria-label", `Name of split ${i + 1}`);
        field.addEventListener("change", () =>
          this.#rename(i, field.value.trim()),
        );

        return field;
      });

      this.#splits.replaceChildren(
        ...this.#fields.map((field) => {
          const row = document.createElement("li");

          row.append(field);
          return row;
        }),
      );
    }

    route.splits.forEach((split, i) => {
      const field = this.#fields[i];

      if (field !== undefined) field.value = split.name;
    });
  }

  #toggleRecording(): void {
    this.#recording = !this.#recording;

    // Said before the timer is told, not after: stopping part way through a run
    // ends the recording there and then, which saves the route and reports it
    // from inside the call below. Clearing the line afterwards would wipe that.
    this.#say(
      this.#recording
        ? "Recording: start a new game, and every level you beat becomes a split."
        : "",
    );

    this.#hooks.setRecording(this.#recording);
    this.draw();
  }

  /** A recording stopped without a single level beaten, so there is no route. */
  recordingEmpty(): void {
    this.#recording = false;
    this.#hooks.setRecording(false);
    // Puts the selected route back in the panel, which until now has been
    // showing the empty one that was being recorded into.
    this.#hooks.changed();
    this.#say("Nothing was recorded: no level was completed.");
    this.draw();
  }

  /** Called back when a recorded run finished, with the route it produced. */
  recorded(route: Route): void {
    this.#recording = false;
    this.#hooks.setRecording(false);
    this.#store.putRoute(route);
    this.#store.select(route.id);
    this.#hooks.changed();
    this.#say(
      `Saved "${route.name}" with ${route.splits.length} splits, and selected it. ` +
        "Rename them below.",
    );
    this.draw();
  }

  /** The selected route and its times, if there is one to export. */
  #exportSelected(): void {
    const route = this.#store.selected;

    if (route === null) {
      this.#say("There is no route to export yet.");
      return;
    }

    this.#export(route);
  }

  #export(only?: Route): void {
    const doc = this.#store.document(only);

    download(FILENAME, documentToJSON(doc));
    this.#say(
      only === undefined
        ? `Exported ${doc.routes.length} route(s) and ${doc.records.length} set(s) of times.`
        : `Exported "${only.name}".`,
    );
  }

  async #import(): Promise<void> {
    const file = this.#file.files?.[0];
    if (file === undefined) return;

    // The picker keeps its last choice, so without this, importing the same
    // file twice in a row raises no change event the second time.
    this.#file.value = "";

    let doc = null;
    try {
      doc = parseDocument(JSON.parse(await file.text()));
    } catch {
      /* Not JSON at all; the same answer as JSON that is not one of these. */
    }

    if (doc === null) {
      this.#say(`"${file.name}" is not a Super Mario 68K speedrun file.`);
      return;
    }

    const added = this.#store.merge(doc);
    this.#hooks.changed();
    this.#say(
      `Imported ${added.routes} route(s) and ${added.records} set(s) of times.`,
    );
    this.draw();
  }

  #clearTimes(): void {
    const route = this.#store.selected;
    if (route === null) return;

    this.#store.clearRecord(route.id);
    this.#hooks.changed();
    this.#say(`Cleared the best run and best segments for "${route.name}".`);
  }

  #removeRoute(): void {
    const route = this.#store.selected;
    if (route === null) return;

    this.#store.removeRoute(route.id);
    this.#hooks.changed();
    this.#say(`Deleted "${route.name}" and the times set on it.`);
    this.draw();
  }

  draw(): void {
    const route = this.#store.selected;

    this.#routeName.textContent =
      route === null
        ? "No routes yet. Record one: turn recording on, play, and every level " +
          "you beat becomes a split."
        : `Running: ${route.name} (${route.category}) — ${route.splits.length} splits`;
    this.#record.textContent = this.#recording
      ? "Stop recording"
      : "Record a route";
    this.#record.setAttribute("aria-pressed", String(this.#recording));

    // Everything but recording and importing acts on the selected route, and
    // until one has been recorded or imported there is not one.
    this.#remove.disabled = route === null;
    this.#clear.disabled = route === null;
    this.#exportOne.disabled = route === null;
    this.#root.dataset["recording"] = String(this.#recording);

    this.#drawEditor(route);
  }
}
