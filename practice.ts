// Practice mode's panel: where a world, a way around its map, a powerup and an
// item list are picked, and the whole of what the page outside this file talks
// to.
//
// The game asks for this rather than being told: src/practice.cpp calls
// Module.onPracticeRequest once per frame of the world map, hands over what the
// player currently is and where, and takes back a request if there is one
// waiting. So nothing here ever reaches into the running game - a request made
// while a level is being played simply sits in the queue until the map is back
// and the game comes asking, which is the one moment the state it rewrites is
// between uses. See src/practice.h for why that matters.
//
// Roaming is the exception to the queue: it is a state rather than an errand,
// so it is sent on every frame it is on, and the game reads it fresh each time
// rather than remembering an answer this panel could then disagree with.

// ---------------------------------------------------------------------------
// The protocol
//
// Both halves of it are written down here because the other half is C++ and
// cannot be asked: these two interfaces are what snapshot() builds and what
// poll() reads back, in src/practice.cpp. A field renamed on one side and not
// the other is a field that silently reads undefined, which is the one mistake
// the types here are for.
// ---------------------------------------------------------------------------

/** What the game reports about itself, once per frame of the world map. */
export interface Status {
  /** The world being played, counted from zero. */
  readonly world: number;
  /** How many worlds the levelset has, the common file excluded. */
  readonly worlds: number;
  readonly width: number;
  readonly height: number;
  /** The square the player is standing on. */
  readonly x: number;
  readonly y: number;
  /** The tile of that square as the map has it now. */
  readonly tile: number;
  /** And as the map file has it, which is how a beaten level is recognised. */
  readonly original: number;
  /** 1 small, 2 large, 3 either of the suits. */
  readonly life: number;
  /** The Attribs bits below. */
  readonly attribs: number;
  /** All twenty slots, trailing empties included. */
  readonly items: readonly number[];
}

/** Which world to load, and whether to load it fresh. */
export interface WarpRequest {
  readonly world: number;
  readonly reload: boolean;
}

/** `level` is the enum Power in src/practice.cpp: small, large, fire, racoon. */
export interface PowerRequest {
  readonly level: number;
  readonly star: boolean;
  readonly pwing: boolean;
}

/** What the panel asks the game for. Every part of it is optional. */
export interface Request {
  readonly warp?: WarpRequest;
  readonly power?: PowerRequest;
  readonly items?: readonly number[];
  /** Mark the level being stood on as beaten. */
  readonly clear?: true;
  /** Put a beaten one back. */
  readonly unclear?: true;
  /** Keep the game's own map input off and let this panel do the moving. */
  readonly roam?: true;
}

// The item list's own numbering, from the switch in Handle_player_map() that
// spends them. 0 is an empty slot and is not offered.
const ITEMS: readonly { readonly value: number; readonly label: string }[] = [
  { value: 1, label: "Mushroom" },
  { value: 2, label: "Fire flower" },
  { value: 3, label: "Leaf" },
  { value: 4, label: "Star" },
  { value: 5, label: "Whistle" },
  { value: 6, label: "Hammer" },
  { value: 7, label: "P-wing" },
  { value: 8, label: "Cloud" },
  { value: 9, label: "Anchor" },
];

// What the game can be, as src/practice.cpp numbers them.
const POWERS: readonly { readonly value: number; readonly label: string }[] = [
  { value: 0, label: "Small" },
  { value: 1, label: "Super" },
  { value: 2, label: "Fire" },
  { value: 3, label: "Racoon" },
];

// Attribs bits, from struct saveplayer in src/player.h. Only the three the
// panel shows or sets; the rest are the game's business.
const ATTRIB_STAR = 0b10000000;
const ATTRIB_FIRE = 0b01000000;
const ATTRIB_RACOON = 0b00100000;
const ATTRIB_PWING = 0b00001000;

// The level tiles, from map.h. A map square holding one of these is a level
// entrance, which is the whole of what decides whether there is anything to
// clear; which level it is is the tile less the low end.
const LEVELS_LOW = 45;
const LEVELS_HIGH = 64;

// How many slots the game's item list has, from itemlist_length in player.h.
const ITEM_SLOTS = 20;

// Squares worth naming in the readout, from enum Maptiles in src/map.h. The
// castles are deliberately absent: their tile numbers fall inside the level
// range above, and a castle is a level, so the level reading is the right one.
const TILE_NAMES: Readonly<Record<number, string>> = {
  19: "road",
  22: "demolished castle",
  23: "castle (visited)",
  27: "dock",
  32: "demolished castle",
  33: "pipe",
  34: "big castle",
  35: "mushroom house",
  36: "game house",
  65: "grass",
  66: "rock",
  78: "locked door",
  87: "water",
  97: "locked door",
};

// How many worlds to offer before the game has said. sm68k ships eight and says
// so on the first frame of the first map; this is only what the picker holds
// until then.
const DEFAULT_WORLDS = 8;

function element<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  className: string,
  text = "",
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);

  if (className) el.className = className;
  if (text) el.textContent = text;
  return el;
}

function option(value: number, label: string): HTMLOptionElement {
  const el = document.createElement("option");

  el.value = String(value);
  el.textContent = label;
  return el;
}

/** A labelled control, laid out as one row of the panel's grid. */
function field(label: string, control: HTMLElement): HTMLLabelElement {
  const wrap = element("label", "practice-field");

  wrap.append(element("span", "", label), control);
  return wrap;
}

/** A checkbox and the label it is written on, which is what gets appended. */
interface Check {
  readonly input: HTMLInputElement;
  readonly root: HTMLLabelElement;
}

function checkbox(label: string): Check {
  const input = document.createElement("input");

  input.type = "checkbox";

  const root = element("label", "practice-check");

  root.append(input, document.createTextNode(" " + label));
  return { input, root };
}

function button(label: string, onClick: () => void): HTMLButtonElement {
  const el = element("button", "", label);

  el.type = "button";
  el.addEventListener("click", onClick);
  return el;
}

/** Whether a map square holds a level entrance, castles included. */
function isLevel(tile: number): boolean {
  return tile >= LEVELS_LOW && tile <= LEVELS_HIGH;
}

function tileName(tile: number): string {
  if (isLevel(tile)) return `level ${tile - LEVELS_LOW + 1}`;

  return TILE_NAMES[tile] ?? `tile ${tile}`;
}

/**
 * What can be done to the square the player is standing on, or null.
 *
 * A level standing on the map can be written off. One that has been beaten -
 * which is a square the map file says was a level and the game has since paved
 * over - can be put back. Nothing else is either.
 */
function levelAction(status: Status): "clear" | "unclear" | null {
  if (isLevel(status.tile)) return "clear";
  if (isLevel(status.original)) return "unclear";

  return null;
}

/** What the player is, read back out of the two fields the game keeps it in. */
function powerFromStatus(status: Status): PowerRequest {
  const attribs = status.attribs;

  let level = status.life >= 2 ? 1 : 0;

  if (status.life >= 3 && attribs & ATTRIB_FIRE) level = 2;
  if (status.life >= 3 && attribs & ATTRIB_RACOON) level = 3;

  return {
    level,
    star: (attribs & ATTRIB_STAR) !== 0,
    pwing: (attribs & ATTRIB_PWING) !== 0,
  };
}

/**
 * Hands the player the run history as a file, and answers with what to say
 * about it. Owned by the speedrun module, which is what keeps the history; see
 * speedrun/history.ts.
 */
export type ExportHistory = () => Promise<string>;

export class Practice {
  readonly #root: HTMLElement;
  readonly #state = element("p", "practice-state");
  readonly #exportHistory: ExportHistory | null;

  // The controls, built where they are declared: every one of them is read or
  // written after the build, and a field assigned in a helper the constructor
  // calls is a field the compiler cannot see being assigned at all.
  readonly #world = document.createElement("select");
  readonly #reload = checkbox("Reload the map");

  // The one control here that is a mode rather than an errand. What it costs is
  // said on the label: while it is on the game is not reading the map's keys at
  // all, so the key that enters a level does nothing either.
  readonly #roam = checkbox(
    "Free roam - arrows go anywhere, off the roads and through the locks" +
      " (the jump key is off while it is on)",
  );

  // One button for the two errands, because a square only ever offers one of
  // them: a level standing on the map can be written off, and one that has been
  // beaten can be put back. Which of the two it is doing is on its face, and it
  // is read off the square rather than off anything remembered here - see
  // levelAction(), and #draw(), which is what keeps the two in step.
  readonly #level = button("Clear this level", () => this.#markLevel());

  readonly #power = document.createElement("select");
  readonly #star = checkbox("Star");
  readonly #pwing = checkbox("P-wing");
  readonly #chips = element("div", "practice-chips");
  readonly #itemPick = document.createElement("select");

  // The request the game has not come for yet, or null. One request at a time:
  // a second edit before the first is taken merges into it, so a player who
  // sets a powerup and then an item list while a level is running gets both.
  #pending: Request | null = null;

  // The last thing the game said about itself, or null before it has said
  // anything. What the controls mirror while nothing is queued.
  #status: Status | null = null;

  // The item list as the panel has it, which is the game's while nothing is
  // queued and the player's edit once something is.
  #items: readonly number[] = [];

  // How many worlds the levelset has, once the game has said.
  #worlds = DEFAULT_WORLDS;

  /**
   * `exportHistory` is null where there is no history being kept - it needs
   * Temporal, the same as the timer - and the panel then offers no export.
   */
  constructor(root: HTMLElement, exportHistory: ExportHistory | null = null) {
    this.#root = root;
    this.#exportHistory = exportHistory;
    this.#build();
    this.#draw();

    // The game's keys are read off window, and the panel is used while the game
    // is running: a press meant for a control here would otherwise also be a
    // press meant for Mario. Stopped here rather than in shell.js because it is
    // this panel's own controls that want the keyboard, and only while they
    // have it.
    for (const type of ["keydown", "keyup"]) {
      root.addEventListener(type, (e) => e.stopPropagation());
    }
  }

  /**
   * Called by the game, once per frame of the world map.
   *
   * Takes what the player currently is and returns what they should be, or null
   * for the overwhelming majority of frames where nothing was asked for.
   */
  handle(status: Status): Request | null {
    this.#status = status;

    if (status.worlds > 0 && status.worlds !== this.#worlds) {
      this.#worlds = status.worlds;
      this.#fillWorlds();
    }

    this.#draw();

    const request = this.#pending;

    // Cleared as it is handed over rather than after: the next status the game
    // sends is the one it sent after doing this, and the controls go back to
    // mirroring it.
    this.#pending = null;

    // Roaming rides along with whatever was queued, and goes on its own on
    // every other frame: the game switches its own map input off for exactly as
    // long as this keeps saying so, and back on the moment it stops.
    if (this.#roam.input.checked) return { ...(request ?? {}), roam: true };

    return request;
  }

  // -------------------------------------------------------------------------
  // Building
  // -------------------------------------------------------------------------

  #build(): void {
    const header = element("div", "practice-header");

    header.append(element("h2", "", "Practice"), this.#state);

    this.#root.append(header, this.#mapSection(), this.#marioSection());

    if (this.#exportHistory !== null) {
      this.#root.append(this.#historySection(this.#exportHistory));
    }
  }

  /**
   * The way out for what practice is kept for. Every game played in practice
   * mode goes into the run history marked as practice, level entries and what
   * was carried into each of them included, and this is where it is read out.
   */
  #historySection(exportHistory: ExportHistory): HTMLElement {
    const box = element("fieldset", "practice-box");
    const said = element(
      "p",
      "practice-note",
      "Every practice game is kept, marked as practice, beside the timed runs.",
    );

    said.setAttribute("role", "status");

    box.append(
      element("legend", "", "History"),
      said,
      button("Export history", () => {
        void exportHistory().then((message) => {
          said.textContent = message;
        });
      }),
    );

    return box;
  }

  #mapSection(): HTMLElement {
    const box = element("fieldset", "practice-box");

    box.append(element("legend", "", "Map"));

    this.#fillWorlds();
    this.#roam.input.addEventListener("change", () => this.#draw());

    box.append(
      field("World", this.#world),
      this.#reload.root,
      button("Warp to that world", () => this.#warp()),
      element("hr", "practice-rule"),
      this.#roam.root,
      this.#level,
    );

    return box;
  }

  #marioSection(): HTMLElement {
    const box = element("fieldset", "practice-box");

    box.append(element("legend", "", "Mario"));

    for (const { value, label } of POWERS) {
      this.#power.append(option(value, label));
    }

    for (const control of [this.#power, this.#star.input, this.#pwing.input]) {
      control.addEventListener("change", () => this.#setPower());
    }

    for (const { value, label } of ITEMS) {
      this.#itemPick.append(option(value, label));
    }

    const adder = element("div", "practice-add");

    adder.append(
      this.#itemPick,
      button("Add", () =>
        this.#setItems([...this.#items, Number(this.#itemPick.value)]),
      ),
      button("Clear", () => this.#setItems([])),
    );

    box.append(
      field("Powerup", this.#power),
      this.#star.root,
      this.#pwing.root,
      element("p", "practice-note", "Items"),
      this.#chips,
      adder,
    );

    return box;
  }

  #fillWorlds(): void {
    const chosen = this.#world.value;

    this.#world.replaceChildren();
    for (let i = 0; i < this.#worlds; i++) {
      this.#world.append(option(i, `World ${i + 1}`));
    }

    // Kept across a refill where it still exists, so the picker does not jump
    // back to world 1 the moment the game says how many there are.
    if (chosen !== "" && Number(chosen) < this.#worlds) {
      this.#world.value = chosen;
    } else if (this.#status) {
      this.#world.value = String(this.#status.world);
    }
  }

  // -------------------------------------------------------------------------
  // Asking for things
  //
  // Each of these queues rather than does. A request is merged into whatever is
  // already queued, so the parts of the panel are independent: setting a
  // powerup does not cancel a warp that has not been collected yet.
  // -------------------------------------------------------------------------

  #queue(part: Request): void {
    this.#pending = { ...(this.#pending ?? {}), ...part };
    this.#draw();
  }

  #warp(): void {
    this.#queue({
      warp: {
        world: Number(this.#world.value),
        reload: this.#reload.input.checked,
      },
    });
  }

  // Whichever of the two this square is offering, taken at the moment of the
  // press rather than from what the button was last drawn as: the game moves
  // the player, and the square under them can have changed since.
  #markLevel(): void {
    const action = this.#status && levelAction(this.#status);

    if (action === "clear") this.#queue({ clear: true });
    if (action === "unclear") this.#queue({ unclear: true });
  }

  #setPower(): void {
    this.#queue({
      power: {
        level: Number(this.#power.value),
        star: this.#star.input.checked,
        pwing: this.#pwing.input.checked,
      },
    });
  }

  #setItems(items: readonly number[]): void {
    // The game's list is twenty slots and drops what does not fit, so the panel
    // refuses the twenty-first rather than showing an item that will not be
    // there.
    this.#items = items.slice(0, ITEM_SLOTS);
    this.#queue({ items: this.#items });
  }

  // -------------------------------------------------------------------------
  // Drawing
  // -------------------------------------------------------------------------

  #draw(): void {
    const status = this.#status;

    // Nothing queued means the panel is a readout of the game, and the controls
    // follow it. Something queued means the player is mid-edit and the panel is
    // theirs until the game comes and takes it.
    if (status && !this.#pending) {
      const power = powerFromStatus(status);

      this.#power.value = String(power.level);
      this.#star.input.checked = power.star;
      this.#pwing.input.checked = power.pwing;
      this.#items = status.items.filter((item) => item !== 0);
    }

    // What the one level button is currently for, and whether it is for
    // anything at all: nothing is known about the square until the game has
    // said, and most squares are neither a level nor a beaten one.
    const action = status && levelAction(status);

    this.#level.disabled = action === null;
    this.#level.textContent =
      action === "unclear" ? "Mark as uncleared" : "Clear this level";

    this.#drawState(status);
    this.#drawChips();
  }

  #drawState(status: Status | null): void {
    if (this.#pending) {
      this.#state.textContent =
        "Queued. Applies on the world map - leave the level you are in.";
      return;
    }

    if (!status) {
      this.#state.textContent = "Waiting for a game to reach the world map.";
      return;
    }

    const roaming = this.#roam.input.checked ? "roaming - " : "";

    // A square that was a level and is not one now is one that has been beaten,
    // and saying which level it was is the whole of what tells the player what
    // putting it back would give them.
    const was =
      !isLevel(status.tile) && isLevel(status.original)
        ? `, was ${tileName(status.original)}`
        : "";

    this.#state.textContent =
      `${roaming}world ${status.world + 1}, square ${status.x}, ${status.y} ` +
      `of ${status.width} x ${status.height} - ${tileName(status.tile)}${was}`;
  }

  #drawChips(): void {
    this.#chips.replaceChildren();

    if (this.#items.length === 0) {
      this.#chips.append(element("span", "practice-empty", "empty"));
      return;
    }

    this.#items.forEach((item, index) => {
      const name = ITEMS.find((entry) => entry.value === item);
      const chip = element("span", "practice-chip", name?.label ?? `#${item}`);

      const drop = button("×", () =>
        this.#setItems(this.#items.filter((_, i) => i !== index)),
      );

      drop.title = "Remove";
      chip.append(drop);
      this.#chips.append(chip);
    });
  }
}
