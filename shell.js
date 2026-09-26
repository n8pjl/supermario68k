// Host-page entry module. mario.mjs is the Emscripten build compiled with
// -sEXPORT_ES6: its default export is a factory that takes the Module config
// and returns a promise for the running instance. The game's own data is
// compiled into the wasm (see src/compat/assets.c), so there is nothing to
// fetch alongside it. All runtime output goes to the browser console.
import createMario from "./mario.js";
import maTexts from "./ma_texts.json" with { type: "json" };
import { RunHistory, Speedrun } from "./speedrun.js";
import { Practice } from "./practice.js";

// ---------------------------------------------------------------------------
// Actions and bindings
//
// The game knows eight actions, the members of struct keystate in scankeys.h.
// Everything that can press one - a key, a controller button or stick, an
// on-screen pad - is resolved to an action here, and scankeys.c asks only for
// the resolved answer. That is what makes the bindings editable: nothing below
// this file has an opinion about what a key means.
// ---------------------------------------------------------------------------
const ACTIONS = ["left", "right", "up", "down", "jump", "run", "enter", "esc"];

const ACTION_LABELS = {
  left: "Left",
  right: "Right",
  up: "Up",
  down: "Down",
  jump: "Jump",
  run: "Run / fire",
  enter: "Enter",
  esc: "Esc",
};

// A binding is one string. A keyboard one is a KeyboardEvent.code - the
// physical key rather than the character on it, so a binding does not move when
// the layout or a modifier does. A controller one is prefixed: gp:b<n> for a
// button, gp:a<n>+ or gp:a<n>- for one direction of an axis.
const GP = "gp:";

// Half travel, which is where Math.round() used to put the old fixed mapping's
// threshold, and far enough that a stick resting off-centre does not walk.
const AXIS_THRESHOLD = 0.5;

// The mapping the port shipped with, plus the Z and X the control legend has
// always claimed and never had. Axes 0/1 are the left stick under the standard
// mapping and 2/3 the right one; both moved the player before, so both still
// do.
const DEFAULT_BINDINGS = {
  left: ["ArrowLeft", "gp:b14", "gp:a0-", "gp:a2-"],
  right: ["ArrowRight", "gp:b15", "gp:a0+", "gp:a2+"],
  up: ["ArrowUp", "gp:b12", "gp:a1-", "gp:a3-"],
  down: ["ArrowDown", "gp:b13", "gp:a1+", "gp:a3+"],
  jump: ["Space", "KeyZ", "gp:b0", "gp:b1"],
  run: ["ShiftLeft", "ShiftRight", "KeyX", "gp:b2", "gp:b3"],
  enter: ["Enter", "gp:b8"],
  esc: ["Escape", "gp:b9"],
};

const BINDINGS_KEY = "sm68k.bindings";

let bindings = structuredClone(DEFAULT_BINDINGS);

// The physical keys some action is currently bound to. Kept alongside the
// bindings so the keydown handler can answer "is this one of ours?" without
// walking every action on every keystroke.
let boundCodes = new Set();

// Which keys are down, and which actions the on-screen pads are holding. Both
// are read by gameActions() below rather than pushed anywhere, so a binding
// change takes effect on the next frame with no state to migrate.
const pressedCodes = new Set();
const touchActions = new Set();

// Every key currently down, whether or not anything is bound to it. Kept apart
// from pressedCodes above because it answers a different question and lives a
// different life: that one is the game's, held only while a game is running and
// only for keys the game reads, while this one is the input display's and holds
// whatever the keyboard is actually doing. Empty unless the display is on.
const heldCodes = new Set();

function isGamepadBinding(binding) {
  return binding.startsWith(GP);
}

function gamepadBindingHeld(binding, pads) {
  const spec = binding.slice(GP.length);

  for (const gp of pads) {
    if (gp?.mapping !== "standard") continue;

    if (spec.startsWith("b")) {
      if (gp.buttons[Number(spec.slice(1))]?.pressed) return true;
      continue;
    }

    const axis = gp.axes[Number(spec.slice(1, -1))];
    if (axis === undefined) continue;

    if (spec.endsWith("+") ? axis > AXIS_THRESHOLD : axis < -AXIS_THRESHOLD) {
      return true;
    }
  }

  return false;
}

// Handed to the runtime and called once per frame by ScanKeys(), and by the two
// waits in scankeys.c. Reading the controller here rather than keeping a copy
// means it never goes stale: the Gamepad API only updates its snapshots when
// they are asked for.
function gameActions() {
  const pads = navigator.getGamepads();
  const state = {};

  for (const action of ACTIONS) {
    state[action] =
      touchActions.has(action) ||
      bindings[action].some((binding) =>
        isGamepadBinding(binding)
          ? gamepadBindingHeld(binding, pads)
          : pressedCodes.has(binding),
      );
  }

  return state;
}

// Passed by the resolve function by code in scankeys.c
const keyPressPromises = { keydown: null };

// WaitKeyPress() has nothing to poll for a keyboard or a finger, so it is woken
// here. Firing on every press rather than only on a new one matches what the
// keyboard's auto-repeat used to do. A controller is polled instead: it has no
// event to raise.
function notifyPress() {
  keyPressPromises.keydown?.();
  keyPressPromises.keydown = null;
}

// The game's key handling is only bound while it runs: outside of that the
// settings menu needs Enter and the arrow keys for itself, and the bindings
// editor needs every key it can get. The controller is what unbinds all three
// listeners again when main() returns.
//
// Which keys count is whatever is bound at the time, so a binding changed while
// the game runs is picked up without rebinding the listeners.
function listenForGameKeys() {
  const keys = new AbortController();
  const opts = { signal: keys.signal };

  addEventListener(
    "keydown",
    (e) => {
      if (!boundCodes.has(e.code)) return;

      e.preventDefault();
      pressedCodes.add(e.code);
      notifyPress();
    },
    opts,
  );

  addEventListener(
    "keyup",
    (e) => {
      if (!boundCodes.has(e.code)) return;

      e.preventDefault();
      pressedCodes.delete(e.code);
    },
    opts,
  );

  // A window that loses focus never sees the keyup, which would otherwise
  // leave the game running into a wall. A backgrounded phone browser stops
  // delivering pointer events just the same, so the pads are let go too.
  addEventListener(
    "blur",
    () => {
      pressedCodes.clear();
      clearTouchActions();
    },
    opts,
  );

  return keys;
}

// main() reads Module.ti89Mode once at startup and the texts are handed over
// with it, so both the calculator and the language have to be picked before the
// runtime is created. The choices are remembered for next time; storage can be
// unavailable (private mode, blocked cookies), in which case the menu just falls
// back to its defaults.
const CALC_KEY = "sm68k.calc";
const LANG_KEY = "sm68k.lang";
const LATCH_KEY = "sm68k.latch";
const SPEEDRUN_KEY = "sm68k.speedrun";
const PRACTICE_KEY = "sm68k.practice";
const KEYBOARD_INPUTS_KEY = "sm68k.keyboardinputs";
const GAMEPAD_INPUTS_KEY = "sm68k.gamepadinputs";
const stage = document.querySelector(".stage");
const consoleBox = document.querySelector(".console");
const settings = document.getElementById("settings");
const select = settings.elements.calc;
const langSelect = settings.elements.lang;
const latchOption = settings.elements.latch;
const canvas = document.getElementById("canvas");
const unsupported = document.getElementById("unsupported");
const speedrunPanel = document.getElementById("speedrun");
const speedrunOption = settings.elements.speedrun;
const keyboardInputsOption = settings.elements.keyboardinputs;
const gamepadInputsOption = settings.elements.gamepadinputs;
const inputsPanel = document.getElementById("inputs");
const keyboardInputs = document.getElementById("input-keyboard");
const gamepadInputs = document.getElementById("input-gamepad");
const inputKeys = document.getElementById("input-keys");
const padSvg = document.getElementById("pad");
const padNote = document.getElementById("pad-note");
const routeOption = document.getElementById("speedrun-route-option");
const routeSelect = settings.elements.route;
const worldsOption = document.getElementById("speedrun-worlds-option");
const worldsCheck = settings.elements.worlds;
const speedrunManage = document.getElementById("speedrun-manage");
const practicePanel = document.getElementById("practice");
const practiceOption = settings.elements.practice;

// The screens the two builds draw to, from init_calc_screen_constants() in
// render.c, and the largest scale they are displayed at. Sizing the canvas from
// here shows the difference between the versions while choosing, and leaves the
// canvas at the size the game wants, so gray.c never resizes it.
const SCALE = 4;
const CALCS = {
  ti92: { width: 240, height: 128 },
  ti89: { width: 160, height: 100 },
};

// How much of the viewport height the menu leaves the canvas. The rest is for
// the title above it and, on a narrow screen where the menu no longer fits over
// the canvas, the settings below it. Neither wants much: what is held back here
// is mostly so that a page of nothing but screen still reads as a page.
const MENU_HEIGHT_SHARE = 0.7;

// A phone screen is nowhere near 960px wide, so the canvas is displayed at
// whatever scale up to 4x the space actually allows. The pixel size is left
// alone - that is the calculator's screen and the game draws to it - so this
// only ever changes the CSS size, and image-rendering: pixelated keeps the
// result sharp. Called again whenever the space changes: a rotation, a
// fullscreen transition, or gray.c handing over a differently sized screen.
function fitCanvas() {
  const width = canvas.width;
  const height = canvas.height;
  const immersive = document.body.classList.contains("immersive");

  // The console's padding and the body's are the only things beside the canvas
  // on its row, and both are read rather than assumed so the media queries in
  // shell.css stay the single place they are set.
  const body = getComputedStyle(document.body);
  const box = getComputedStyle(consoleBox);
  const chrome =
    parseFloat(body.paddingLeft) +
    parseFloat(body.paddingRight) +
    parseFloat(box.paddingLeft) +
    parseFloat(box.paddingRight) +
    parseFloat(box.borderLeftWidth) +
    parseFloat(box.borderRightWidth);

  // The width the panel wants, and the gap it would keep from the console. Read
  // from shell.css so the two cannot drift apart, and needed here whether or not
  // the panel is currently standing beside anything.
  const stageStyle = getComputedStyle(stage);
  const panelWidth = parseFloat(
    stageStyle.getPropertyValue("--panel-width") || 0,
  );
  const inputsWidth = parseFloat(
    stageStyle.getPropertyValue("--inputs-width") || 0,
  );
  const gap = parseFloat(stageStyle.columnGap) || 0;

  // The panel's width is only the canvas's to give up in fullscreen, where the
  // canvas is fitted to the whole viewport and so there is no room beside it
  // that was not already the screen's. On the page there is room to the side,
  // and the canvas is fitted as though the panel were not there at all: turning
  // the timer on never costs the screen a scale step. There is one panel a
  // side, so what the canvas gives up is whichever of the two are there.
  const beside =
    (speedrunPanel.hidden || !immersive ? 0 : panelWidth + gap) +
    (inputsPanel.hidden || !immersive ? 0 : inputsWidth + gap);

  const availWidth = document.body.clientWidth - chrome - beside;
  const availHeight = immersive ? innerHeight : innerHeight * MENU_HEIGHT_SHARE;
  // 4x is as large as the calculator's screen wants to be in the middle of a
  // page, but a screen given over to the game entirely should fill it.
  const cap = isFullscreen() ? Infinity : SCALE;
  const scale = Math.max(
    Math.min(cap, availWidth / width, availHeight / height),
    1,
  );

  canvas.style.width = Math.round(width * scale) + "px";
  canvas.style.height = Math.round(height * scale) + "px";

  // Where the panel goes, now that the size of the thing it is going beside is
  // settled. Reading the console back rather than adding the canvas up again:
  // it is the width actually laid out, bezel and all. Nothing here resizes the
  // canvas, so there is no loop in measuring it.
  const free =
    document.body.clientWidth -
    parseFloat(body.paddingLeft) -
    parseFloat(body.paddingRight) -
    consoleBox.getBoundingClientRect().width;

  // The console keeps the middle of the page whether the timer is there or not,
  // so what the panel has to stand in is the free space on its side alone: half.
  // Anything less and it goes under the console instead, which costs the canvas
  // nothing either. Fullscreen is already settled - the canvas gave up the width
  // above, and shell.css holds it open.
  //
  // The timer stands in the free space on one side and the input display in the
  // other, which is why each is measured against half of it rather than all of
  // it: neither can be given room the other is standing in.
  stage.classList.toggle(
    "beside",
    !speedrunPanel.hidden && (immersive || free / 2 >= panelWidth + gap),
  );
  stage.classList.toggle(
    "inputs-beside",
    !inputsPanel.hidden && (immersive || free / 2 >= inputsWidth + gap),
  );
}

function sizeCanvas(calc) {
  const { width, height } = CALCS[calc];

  canvas.width = width;
  canvas.height = height;
  fitCanvas();
}

// Rotating a phone, opening the on-screen keyboard and entering or leaving
// fullscreen all arrive here as a resize.
addEventListener("resize", fitCanvas);

// One entry per language in ma_texts.json, in the order the file lists them.
// Each names itself in its own language, so the list reads the same whichever
// one is selected.
const LANGS = Object.keys(maTexts);

for (const lang of LANGS) {
  const option = document.createElement("option");

  option.value = lang;
  option.textContent = maTexts[lang].language;
  langSelect.append(option);
}

// The first language the browser asks for that the game has texts for, English
// otherwise. Only the primary subtag is matched: there is one set of texts per
// language, so a request for fr-CA is served by fr.
function localeLang() {
  const tags = navigator.languages?.length
    ? navigator.languages
    : [navigator.language].filter(Boolean);

  // ma_texts.json calls Norwegian "no"; browsers name the two written forms.
  const LANG_ALIASES = { nb: "no", nn: "no" };

  for (const tag of tags) {
    const primary = tag.toLowerCase().split("-")[0];
    const lang = LANG_ALIASES[primary] ?? primary;

    if (LANGS.includes(lang)) return lang;
  }

  return "en";
}

langSelect.value = localeLang();

try {
  const saved = localStorage.getItem(CALC_KEY);
  if (saved && Object.hasOwn(CALCS, saved)) select.value = saved;

  const savedLang = localStorage.getItem(LANG_KEY);
  if (savedLang && Object.hasOwn(maTexts, savedLang)) {
    langSelect.value = savedLang;
  }

  // Absent means never chosen, which is the checked default in index.html.
  const savedLatch = localStorage.getItem(LATCH_KEY);
  if (savedLatch !== null) latchOption.checked = savedLatch === "true";

  speedrunOption.checked = localStorage.getItem(SPEEDRUN_KEY) === "true";
  practiceOption.checked = localStorage.getItem(PRACTICE_KEY) === "true";

  // Absent means never asked for, which is off: the display is for looking at
  // the controls rather than for playing, so it is not something to be found
  // switched on by a player who never went looking for it.
  keyboardInputsOption.checked =
    localStorage.getItem(KEYBOARD_INPUTS_KEY) === "true";
  gamepadInputsOption.checked =
    localStorage.getItem(GAMEPAD_INPUTS_KEY) === "true";
} catch {
  /* empty */
}

// ---------------------------------------------------------------------------
// The speedrun timer
//
// speedrun.js owns the panel, the route picker, the routes section, and every
// decision about what a game event means to a run. What is left here is whether
// the player asked for a timer at all, and handing the runtime the hook to
// report through.
// ---------------------------------------------------------------------------

// The timer holds and stores its times as Temporal.Duration, and there is no
// sensible fallback for that, so where it is missing the option is taken away
// rather than offered and then found not to work. Nothing else on the page
// needs it: the game itself still runs.
const speedrunSupported = typeof Temporal !== "undefined";

let speedrun = null;

// Every attempt, timed or practised, kept for reading back later; see
// speedrun/history.ts. One of it for the page, shared by the timer and by
// practice mode, and kept on the same terms as the timer: it stamps what it
// keeps with Temporal, so where the timer is not offered it is not kept.
const runHistory = speedrunSupported ? new RunHistory() : null;

// Built on first use and kept afterwards, so the times of a run just finished
// are still on the page when the menu comes back, and so that turning the
// option off and on again does not lose them. It owns the panel, the route
// picker and the routes section; what is left here is whether the player asked
// for one at all.
function showSpeedrun(wanted) {
  if (wanted && !speedrun) {
    speedrun = new Speedrun(
      {
        panel: speedrunPanel,
        manage: speedrunManage,
        routes: routeSelect,
        worlds: worldsCheck,
      },
      runHistory,
    );
  }

  speedrunPanel.hidden = !wanted;
  routeOption.hidden = !wanted;
  worldsOption.hidden = !wanted;
  speedrunManage.hidden = !wanted;

  // The other half of the exclusion below: practice mode is held off while the
  // timer is on, the same way and for the same reason, so whichever of the two
  // was asked for first is the one that stands and the other reads as
  // unavailable rather than as merely unticked.
  practiceOption.disabled = wanted;
  if (wanted && practiceOption.checked) {
    practiceOption.checked = false;
    showPractice(false);
  }

  fitCanvas();
}

if (!speedrunSupported) {
  speedrunOption.checked = false;
  document.getElementById("speedrun-option").hidden = true;
}

// Shown as soon as it is asked for rather than when the game starts, so the
// panel is where the player put it, showing the splits they will be running,
// before they commit to a run.
speedrunOption.addEventListener("change", () =>
  showSpeedrun(speedrunOption.checked),
);

showSpeedrun(speedrunOption.checked);

// ---------------------------------------------------------------------------
// Practice mode
//
// practice.js owns the panel and everything the player does in it; what is left
// here is whether they asked for it, and handing the runtime the hook the game
// polls - see src/practice.h for what the game does with it.
//
// It is deliberately not a timer's friend: warping past a level and handing
// yourself a P-wing are exactly the things a run is timed for not doing, so the
// two options exclude each other rather than letting practice write a record.
// The exclusion runs both ways - each switches the other off and holds it there
// while it is on - so it is the player's choice which of the two they keep,
// rather than one of them always being the one to give way.
// ---------------------------------------------------------------------------

let practice = null;

function showPractice(wanted) {
  if (wanted && !practice) {
    practice = new Practice(
      practicePanel,
      runHistory ? () => runHistory.export() : null,
    );
  }

  practicePanel.hidden = !wanted;

  // The timer is not taken away, only switched off and held there while
  // practice mode is on: turning practice back off gives the player their
  // timer setting back, rather than making them find it again. showSpeedrun()
  // does the same to this option, and the two calls settle rather than chase
  // each other: the one switched off is called with false, which disables
  // nothing and so asks nothing of the one that switched it off.
  speedrunOption.disabled = wanted;
  if (wanted && speedrunOption.checked) {
    speedrunOption.checked = false;
    showSpeedrun(false);
  }
}

practiceOption.addEventListener("change", () =>
  showPractice(practiceOption.checked),
);

showPractice(practiceOption.checked);

sizeCanvas(select.value);
select.addEventListener("change", () => sizeCanvas(select.value));

// ---------------------------------------------------------------------------
// The bindings editor
//
// The control legend on the page is the editor: every action lists what it
// answers to, the x on a binding drops it and + waits for the next key or
// controller input to add one. Saved per browser; anything unreadable there
// falls back to the defaults rather than leaving the game unplayable.
// ---------------------------------------------------------------------------
const controlsPanel = document.getElementById("controls");
const bindingList = document.getElementById("bindings");

// Names for the standard gamepad mapping, which is the only one bound: the
// indices are fixed by the spec, so these read the same on any pad claiming it.
// Every one of them is shown behind a "Pad" - a controller's X and a keyboard's
// X are different bindings, and two chips reading X would be a puzzle.
const GP_BUTTON_LABELS = {
  0: "A",
  1: "B",
  2: "X",
  3: "Y",
  4: "LB",
  5: "RB",
  6: "LT",
  7: "RT",
  8: "Select",
  9: "Start",
  10: "L3",
  11: "R3",
  12: "\u2191",
  13: "\u2193",
  14: "\u2190",
  15: "\u2192",
  16: "Home",
};

const KEY_LABELS = {
  ArrowUp: "\u2191",
  ArrowDown: "\u2193",
  ArrowLeft: "\u2190",
  ArrowRight: "\u2192",
  Space: "Space",
  Escape: "Esc",
  ShiftLeft: "L Shift",
  ShiftRight: "R Shift",
  ControlLeft: "L Ctrl",
  ControlRight: "R Ctrl",
  AltLeft: "L Alt",
  AltRight: "R Alt",
};

function bindingLabel(binding) {
  if (!isGamepadBinding(binding)) {
    if (KEY_LABELS[binding]) return KEY_LABELS[binding];

    // KeyZ, Digit4, Numpad7 - the prefix is the kind of key, which the label
    // does not need to repeat. Anything else is already its own name.
    return binding.replace(/^(Key|Digit)/, "").replace(/^Numpad/, "Num ");
  }

  const spec = binding.slice(GP.length);
  if (spec.startsWith("b")) {
    const n = Number(spec.slice(1));

    return "Pad " + (GP_BUTTON_LABELS[n] ?? n);
  }

  // A stick has no keyboard namesake to be confused with, so it names itself.

  const axis = Number(spec.slice(1, -1));
  const stick = axis < 2 ? "Stick" : "R-Stick";
  const way =
    axis % 2 === 0
      ? spec.endsWith("+")
        ? "\u2192"
        : "\u2190"
      : spec.endsWith("+")
        ? "\u2193"
        : "\u2191";

  return stick + " " + way;
}

function saveBindings() {
  try {
    localStorage.setItem(BINDINGS_KEY, JSON.stringify(bindings));
  } catch {
    /* empty */
  }
}

function loadBindings() {
  try {
    const saved = JSON.parse(localStorage.getItem(BINDINGS_KEY) ?? "null");
    if (saved === null || typeof saved !== "object") return;

    for (const action of ACTIONS) {
      if (!Array.isArray(saved[action])) continue;

      bindings[action] = saved[action].filter((b) => typeof b === "string");
    }
  } catch {
    /* Unreadable or unparseable: the defaults already in place stand. */
  }
}

// Redrawing and refreshing what the keydown handler filters on, in one place so
// no caller can do one without the other.
function refreshBindings() {
  boundCodes = new Set();
  for (const action of ACTIONS) {
    for (const binding of bindings[action]) {
      if (!isGamepadBinding(binding)) boundCodes.add(binding);
    }
  }

  renderBindings();
  renderKeyboardKeys();
}

// The same, for a change the player made. Loading the page is not one of those:
// writing the defaults back on every visit would make "never touched this" look
// like a choice, and freeze anyone on whatever the defaults were the first time
// they loaded it.
function bindingsChanged() {
  refreshBindings();
  saveBindings();
}

// A key or button does one thing, so taking it for an action takes it off
// whatever had it before. Without this a stray rebind leaves the player pressing
// two actions at once with no sign of why.
function assignBinding(action, binding) {
  for (const other of ACTIONS) {
    bindings[other] = bindings[other].filter((b) => b !== binding);
  }
  bindings[action].push(binding);
}

// The capture in progress, if any: one at a time, so starting another stops the
// one before it rather than leaving two listeners racing for the same keypress.
let capture = null;

function stopCapture() {
  capture?.abort();
  capture = null;
}

function captureGamepadBinding(signal, done) {
  const pressedNow = () => {
    const active = new Set();

    for (const gp of navigator.getGamepads()) {
      if (gp?.mapping !== "standard") continue;

      gp.buttons.forEach((button, i) => {
        if (button.pressed) active.add(GP + "b" + i);
      });
      gp.axes.forEach((value, i) => {
        if (value > AXIS_THRESHOLD) active.add(GP + "a" + i + "+");
        if (value < -AXIS_THRESHOLD) active.add(GP + "a" + i + "-");
      });
    }

    return active;
  };

  // What is already held when capture starts is not what the player is offering
  // - they are still holding the button that opened the editor, as likely as
  // not - so only a change from that counts.
  let before = pressedNow();

  const poll = () => {
    if (signal.aborted) return;

    const now = pressedNow();
    for (const binding of now) {
      if (!before.has(binding)) return done(binding);
    }

    before = now;
    requestAnimationFrame(poll);
  };

  requestAnimationFrame(poll);
}

function listenForBinding(action, row) {
  stopCapture();
  capture = new AbortController();
  const signal = capture.signal;

  const note = document.createElement("span");
  note.className = "listening";
  note.textContent = "Press a key or button\u2026";

  const cancel = document.createElement("button");
  cancel.type = "button";
  cancel.className = "cancel-binding";
  cancel.textContent = "Cancel";
  cancel.addEventListener("click", () => {
    stopCapture();
    renderBindings();
  });

  row.replaceChildren(note, cancel);

  const done = (binding) => {
    stopCapture();
    assignBinding(action, binding);
    bindingsChanged();
  };

  // Capture phase, so a key on its way to something else on the page - Enter
  // reaching the settings form, say - is taken here first.
  addEventListener(
    "keydown",
    (e) => {
      e.preventDefault();
      e.stopPropagation();
      done(e.code);
    },
    { signal, capture: true },
  );

  captureGamepadBinding(signal, done);
}

function bindingChip(action, binding) {
  const chip = document.createElement("span");
  chip.className = isGamepadBinding(binding) ? "chip pad" : "chip";
  chip.append(bindingLabel(binding));

  const remove = document.createElement("button");
  remove.type = "button";
  remove.className = "remove-binding";
  remove.textContent = "\u00d7";
  remove.title = "Remove " + bindingLabel(binding);
  remove.addEventListener("click", () => {
    bindings[action] = bindings[action].filter((b) => b !== binding);
    bindingsChanged();
  });

  chip.append(remove);
  return chip;
}

function renderBindings() {
  const rows = [];

  for (const action of ACTIONS) {
    const name = document.createElement("dt");
    name.textContent = ACTION_LABELS[action];

    const row = document.createElement("dd");
    for (const binding of bindings[action]) {
      row.append(bindingChip(action, binding));
    }

    const add = document.createElement("button");
    add.type = "button";
    add.className = "add-binding";
    add.textContent = "+";
    add.title = "Add a key or button for " + ACTION_LABELS[action];
    add.addEventListener("click", () => listenForBinding(action, row));
    row.append(add);

    rows.push(name, row);
  }

  bindingList.replaceChildren(...rows);
}

document.getElementById("reset-bindings").addEventListener("click", () => {
  stopCapture();
  bindings = structuredClone(DEFAULT_BINDINGS);
  bindingsChanged();
});

loadBindings();
refreshBindings();

// ---------------------------------------------------------------------------
// On-screen controls
//
// The pads name actions in data-actions, the same eight everything else
// resolves to, so a pad needs no binding of its own. A pad may name more than
// one - that is how a direction pad corner would hold a diagonal - which is why
// the attribute is read as a token list.
// ---------------------------------------------------------------------------
const touchpad = document.getElementById("touchpad");
const touchIsPrimary = matchMedia("(pointer: coarse)");

// B is a switch by default rather than a key to be held: running in this game
// means holding Run for as long as you want to move, which is a lot to ask of a
// thumb that is also aiming at A. The pad marked data-latch is the one this
// applies to, and the menu option turns it back into a held key.
let latchRun = true;
const latchedActions = new Set();

function isLatching(pad) {
  return latchRun && pad.hasAttribute("data-latch");
}

function padActions(pad) {
  return pad.dataset.actions.split(" ");
}

// Which pad each finger currently rests on. Hit testing on every move rather
// than per-element enter/leave listeners, so that sliding a thumb from left to
// right across the direction pad - or from B onto A - hands over cleanly, the
// way a physical pad does.
const padUnderPointer = new Map();

function padAt(x, y) {
  return document.elementFromPoint(x, y)?.closest("[data-actions]") ?? null;
}

function syncTouchActions() {
  // A latched pad is asking for its action until it is tapped off again,
  // whether or not a finger is still on it, so it joins what the pointers are
  // asking for rather than being tracked apart from it.
  const wanted = new Set(latchedActions);

  for (const pad of padUnderPointer.values()) {
    for (const action of padActions(pad)) wanted.add(action);
  }

  for (const action of touchActions) {
    if (!wanted.has(action)) touchActions.delete(action);
  }

  let pressed = false;
  for (const action of wanted) {
    if (touchActions.has(action)) continue;

    touchActions.add(action);
    pressed = true;
  }
  if (pressed) notifyPress();

  const held = new Set(padUnderPointer.values());
  for (const pad of touchpad.querySelectorAll("[data-actions]")) {
    const latched =
      isLatching(pad) && padActions(pad).every((a) => latchedActions.has(a));

    pad.classList.toggle("held", held.has(pad) || latched);
  }
}

function clearTouchActions() {
  padUnderPointer.clear();
  latchedActions.clear();
  syncTouchActions();
}

// Capturing the pointer to the layer means the moves and the release keep
// arriving here even once the finger has slid off the pad it started on, or
// past the edge of the layer entirely.
touchpad.addEventListener("pointerdown", (e) => {
  const pad = padAt(e.clientX, e.clientY);
  if (!pad) return;

  e.preventDefault();

  // A latching pad answers to the tap itself, not to how long it is held, so
  // it never joins the pointer bookkeeping below.
  if (isLatching(pad)) {
    for (const action of padActions(pad)) {
      if (!latchedActions.delete(action)) latchedActions.add(action);
    }
    syncTouchActions();
    return;
  }

  try {
    touchpad.setPointerCapture(e.pointerId);
  } catch {
    /* A pointer that is no longer active cannot be captured; the pad still
       presses, and the release below still frees it. */
  }
  padUnderPointer.set(e.pointerId, pad);
  syncTouchActions();
});

touchpad.addEventListener("pointermove", (e) => {
  if (!padUnderPointer.has(e.pointerId)) return;

  const pad = padAt(e.clientX, e.clientY);
  if (pad && !isLatching(pad)) {
    padUnderPointer.set(e.pointerId, pad);
  } else {
    padUnderPointer.delete(e.pointerId);
  }
  syncTouchActions();
});

for (const type of ["pointerup", "pointercancel"]) {
  touchpad.addEventListener(type, (e) => {
    if (padUnderPointer.delete(e.pointerId)) syncTouchActions();
  });
}

// ---------------------------------------------------------------------------
// The input display
//
// What is being pressed, right now, as a picture of the hardware rather than a
// reading of the eight actions: a key bound to nothing still lights up, and a
// stick is drawn at how far it is pushed rather than at whether that is far
// enough to count. Something that only reported the actions would answer the
// question the game asks and not the one the player does - why did that input
// not come out, why did the run drift left - which is the whole reason to have
// it on screen.
//
// The two halves are asked for separately and both start off. They cost nothing
// while they are off, and that is deliberate rather than incidental: the
// keyboard's listeners are not bound and the controller's loop is not running,
// so a player who never turns them on never pays for a frame of them.
// ---------------------------------------------------------------------------

// The keyboard half. Every key some action is bound to gets a chip, in the
// order the actions are listed, so the row is the same shape from one frame to
// the next and a glance finds a key where it was last time. Chips for unbound
// keys are appended as they are pressed and dropped as they are let go: the
// row's fixed part is the control map, its tail is everything else the keyboard
// is doing.
//
// The chips are the state - each carries the code it stands for - so nothing
// here has a copy of the row to be kept in step with it.
function renderKeyboardKeys() {
  const chips = new Map();

  for (const action of ACTIONS) {
    for (const binding of bindings[action]) {
      if (isGamepadBinding(binding)) continue;

      // One key can press two actions; it is still one key, and one chip
      // naming both of them.
      const seen = chips.get(binding);
      if (seen) {
        seen.title += " / " + ACTION_LABELS[action];
        continue;
      }

      const chip = document.createElement("span");
      chip.className = "input-key";
      chip.dataset.code = binding;
      chip.textContent = bindingLabel(binding);
      chip.title = ACTION_LABELS[action];
      chips.set(binding, chip);
    }
  }

  inputKeys.replaceChildren(...chips.values());
  paintKeyboard();
}

// Lighting the row for what is held. Called from the key listeners rather than
// from a frame loop: a keyboard says when it changes, so there is nothing to
// poll for and nothing to redraw in between.
function paintKeyboard() {
  // A copy, because the unbound chips are removed from the live list below.
  for (const chip of [...inputKeys.children]) {
    const held = heldCodes.has(chip.dataset.code);

    // An unbound chip is only there for as long as the key is down: it is
    // saying that a key was pressed, not listing the keyboard.
    if (!held && chip.classList.contains("unbound")) {
      chip.remove();
      continue;
    }

    chip.classList.toggle("held", held);
  }

  for (const code of heldCodes) {
    const selector = '[data-code="' + CSS.escape(code) + '"]';
    if (inputKeys.querySelector(selector)) continue;

    const chip = document.createElement("span");
    chip.className = "input-key unbound held";
    chip.dataset.code = code;
    chip.textContent = bindingLabel(code);
    chip.title = "Not bound to anything";
    inputKeys.append(chip);
  }
}

let keyWatch = null;

function watchKeyboardInputs(wanted) {
  if (!wanted) {
    keyWatch?.abort();
    keyWatch = null;
    heldCodes.clear();
    return;
  }

  if (keyWatch) return;

  keyWatch = new AbortController();
  const opts = { signal: keyWatch.signal };

  // Bubble phase, and nothing here calls preventDefault: the display only
  // watches. What a key means - whether the game takes it, whether the page
  // scrolls on it, whether the bindings editor is waiting for it - is settled
  // by the listeners that own it, and showing a key must not change the
  // answer. The editor's capture-phase listener stops its keypress reaching
  // here at all, which is right: the key it swallowed was never pressed at
  // the game, and its keyup arrives to find nothing held.
  addEventListener(
    "keydown",
    (e) => {
      // Auto-repeat is the same key still down, not a new press.
      if (heldCodes.has(e.code)) return;

      heldCodes.add(e.code);
      paintKeyboard();
    },
    opts,
  );

  addEventListener(
    "keyup",
    (e) => {
      if (heldCodes.delete(e.code)) paintKeyboard();
    },
    opts,
  );

  // A key let go of while the window is looking elsewhere never reports it,
  // and the display would show it held for good.
  addEventListener(
    "blur",
    () => {
      if (heldCodes.size === 0) return;

      heldCodes.clear();
      paintKeyboard();
    },
    opts,
  );
}

// The controller half. The picture is in index.html, and every part of it names
// what it stands for; these are those parts, looked up the once. The loop below
// runs every frame and has no business asking the document for them again each
// time.
const padButtons = new Map();
const padSticks = [];

for (const el of padSvg.querySelectorAll("[data-b]")) {
  padButtons.set(Number(el.dataset.b), el);
}

for (const el of padSvg.querySelectorAll(".pad-stick")) {
  padSticks.push({
    group: el,
    dot: el.querySelector(".pad-dot"),
    xAxis: Number(el.dataset.xAxis),
    yAxis: Number(el.dataset.yAxis),
    press: Number(el.dataset.press),
  });
}

// How far the dot travels from the middle of its gate at full deflection, in
// the SVG's own units: the gate's radius less the dot's, so a stick pushed all
// the way sits inside the ring rather than over it.
const STICK_TRAVEL = 10;

// Every connected pad read as one, because that is how the bindings read them:
// any pad can press an action, so any pad can light the picture. A button is
// held if it is held anywhere, and an axis is taken at the furthest it is
// pushed anywhere, which is the same rule.
function mergedPad(pads) {
  const pressed = [];
  const axes = [];
  let count = 0;

  for (const gp of pads) {
    // The standard mapping is the only one the bindings resolve against,
    // and the only one these indices mean anything under.
    if (gp?.mapping !== "standard") continue;

    count++;

    gp.buttons.forEach((button, i) => {
      if (button.pressed) pressed[i] = true;
    });

    gp.axes.forEach((value, i) => {
      if (Math.abs(value) > Math.abs(axes[i] ?? 0)) axes[i] = value;
    });
  }

  return { pressed, axes, count };
}

function paintGamepad(pad) {
  for (const [index, el] of padButtons) {
    el.classList.toggle("held", Boolean(pad.pressed[index]));
  }

  for (const stick of padSticks) {
    let x = pad.axes[stick.xAxis] ?? 0;
    let y = pad.axes[stick.yAxis] ?? 0;

    // A stick held into a corner reports a pair the round gate it is drawn
    // as has no room for. Scaling it back keeps the dot on the ring rather
    // than outside it, and keeps the direction it is being held, which is
    // the part being read.
    const pull = Math.hypot(x, y);
    if (pull > 1) {
      x /= pull;
      y /= pull;
    }

    const dx = x * STICK_TRAVEL;
    const dy = y * STICK_TRAVEL;

    stick.dot.style.transform = "translate(" + dx + "px," + dy + "px)";

    // Two different things, and worth telling apart: the stick pressed in
    // as a button, and the stick pushed past the point a binding on it
    // calls a press. The second is what says why the player is walking.
    stick.group.classList.toggle("held", Boolean(pad.pressed[stick.press]));
    stick.group.classList.toggle("active", pull > AXIS_THRESHOLD);
  }
}

// Only ever what the picture cannot say for itself. A pad that is connected and
// being read needs no line under it: the diagram lighting up is the whole of
// that message, and a pad's own name for itself is two lines of vendor and
// product ids that say nothing about what is being pressed.
function padNoteText(pad, pads) {
  if (pad.count > 1) return pad.count + " gamepads, shown as one";
  if (pad.count === 1) return "";

  // A pad the browser has no standard mapping for cannot be bound either, so
  // the picture being dark with one plainly plugged in wants explaining.
  if (pads.some(Boolean)) {
    return "Gamepad connected, but not in the standard layout the game reads.";
  }

  return "No gamepad connected. Press a button on one to wake it up.";
}

// Polled, because a controller has no events to raise for what it is doing, and
// because navigator.getGamepads() only refreshes its snapshots when it is
// asked. Not polled when there is nothing to poll: with no pad the mapping
// applies to, this waits on a connection rather than burning a frame callback
// against an empty list - the same trade listenForGamepadStart() below makes.
async function watchGamepadInputs(signal) {
  while (!signal.aborted) {
    const pads = navigator.getGamepads();
    const pad = mergedPad(pads);

    paintGamepad(pad);
    padSvg.classList.toggle("idle", pad.count === 0);

    // Read back rather than written every frame: the note only changes when a
    // pad is plugged in or pulled out, and the rest of this loop is already
    // touching the document sixty times a second.
    const note = padNoteText(pad, pads);
    if (padNote.textContent !== note) padNote.textContent = note;

    if (pad.count === 0) {
      await new Promise((resolve) => {
        addEventListener("gamepadconnected", resolve, {
          once: true,
          signal,
        });
        // So the note stops naming a pad that has been unplugged.
        addEventListener("gamepaddisconnected", resolve, {
          once: true,
          signal,
        });
        signal.addEventListener("abort", resolve, { once: true });
      });
      continue;
    }

    await new Promise((resolve) => requestAnimationFrame(resolve));
  }
}

let padWatch = null;

// Shown as soon as it is asked for rather than when the game starts, the same
// as the timer: a rebind is worth watching take effect, and a stick worth
// checking before finding out mid-level that it drifts.
function showInputs() {
  const keyboard = keyboardInputsOption.checked;
  const gamepad = gamepadInputsOption.checked;

  keyboardInputs.hidden = !keyboard;
  gamepadInputs.hidden = !gamepad;
  inputsPanel.hidden = !keyboard && !gamepad;

  watchKeyboardInputs(keyboard);
  if (keyboard) paintKeyboard();

  if (gamepad && !padWatch) {
    padWatch = new AbortController();
    watchGamepadInputs(padWatch.signal);
  } else if (!gamepad && padWatch) {
    padWatch.abort();
    padWatch = null;
  }

  fitCanvas();
}

keyboardInputsOption.addEventListener("change", showInputs);
gamepadInputsOption.addEventListener("change", showInputs);

showInputs();

// ---------------------------------------------------------------------------
// Fullscreen and orientation
//
// A phone shows the game far larger with the browser chrome out of the way and
// the screen turned sideways, so the corner toggle asks for both at once. It is
// a toggle rather than something in the way of starting a game: nothing here
// happens unless it is pressed. The orientation lock is best-effort - iOS has
// no such API, and a browser may refuse it outside fullscreen.
// ---------------------------------------------------------------------------
const fullscreenToggle = document.getElementById("fullscreen-toggle");

function isFullscreen() {
  return document.fullscreenElement !== null;
}

// The toggle belongs to a running game, so the menu does not carry it. The one
// exception is a page still fullscreen after the game has exited: hiding the
// way back out would leave a phone with no obvious one.
//
// The immersive class goes on for a game running either on a touch device or
// fullscreen. That second half is what the fullscreen button buys a desktop
// player: without it, going fullscreen would only put the same 960px canvas in
// the middle of a bigger empty page.
function updateChrome() {
  const playing = document.body.classList.contains("playing");

  fullscreenToggle.hidden =
    !document.fullscreenEnabled || (!playing && !isFullscreen());
  // Not just unclickable but unreachable: a Tab into the editor mid-game would
  // put a capture in the way of the keys the game is reading. The routes
  // section below is deliberately left alone - it captures nothing, and a
  // recording in progress has to be stoppable from it while the game runs.
  controlsPanel.inert = playing;
  document.body.classList.toggle(
    "immersive",
    playing && (touchIsPrimary.matches || isFullscreen()),
  );
}

updateChrome();

// A laptop with a touchscreen can change its answer when a mouse is plugged in.
touchIsPrimary.addEventListener("change", () => {
  updateChrome();
  fitCanvas();
});

fullscreenToggle.addEventListener("click", async () => {
  if (isFullscreen()) {
    await document.exitFullscreen();
    return;
  }

  try {
    await document.documentElement.requestFullscreen({ navigationUI: "hide" });
  } catch {
    /* Refused: the game is just as playable in the page. */
    return;
  }

  // Only on a device that gets turned: locking a laptop's screen to landscape
  // would be asking the browser for something nobody wants.
  if (!touchIsPrimary.matches) return;

  try {
    await screen.orientation.lock("landscape");
  } catch {
    /* No lock available; the hint in shell.css asks instead. */
  }
});

// Leaving fullscreen with Esc or the browser's own control never reaches the
// handler above, so the icon follows the event rather than the click.
document.addEventListener("fullscreenchange", () => {
  document.body.classList.toggle("fullscreen", isFullscreen());
  updateChrome();
  fitCanvas();
});

// ---------------------------------------------------------------------------
// Keeping the screen on
//
// A gamepad is not an input the system's idle timer watches: a player who has
// both hands on a pad and never touches the keyboard or the mouse looks idle
// to it, and the screen blanks in the middle of a level. The wake lock is the
// web's answer to that, and it is best-effort throughout - a browser without
// the API, or one refusing the request on a low battery, just means the screen
// may blank, which is a nuisance rather than a bug.
//
// The lock is only held while a game is up, and the browser takes it away by
// itself whenever the page is hidden without ever handing it back, so coming
// back to a backgrounded tab has to ask again.
// ---------------------------------------------------------------------------
let wakeLock = null;

function acquireWakeLock() {
  if (!navigator.wakeLock || wakeLock) return;

  // What is stored is the request rather than the sentinel it resolves to, so
  // that a second call arriving before the first has been answered does not
  // ask for a second lock. Both handlers clear it only if it is still theirs,
  // since a release racing with the next game's request would otherwise throw
  // that request away.
  const request = navigator.wakeLock.request("screen").then(
    (lock) => {
      lock.addEventListener("release", () => {
        if (wakeLock === request) wakeLock = null;
      });
      return lock;
    },
    () => {
      if (wakeLock === request) wakeLock = null;
      return null;
    },
  );

  wakeLock = request;
}

async function releaseWakeLock() {
  const held = wakeLock;
  wakeLock = null;
  try {
    await (await held)?.release();
  } catch {
    /* Already gone, which is the state this was asking for. */
  }
}

// Asking again after a tab switch, a phone unlock, or anything else that hid
// the page long enough for the browser to drop the lock.
document.addEventListener("visibilitychange", () => {
  if (
    document.visibilityState === "visible" &&
    document.body.classList.contains("playing")
  ) {
    acquireWakeLock();
  }
});

// START on the gamepad submits the settings form, so the game can be started
// without touching the keyboard. Polling only happens while the menu is up,
// and only once there is a gamepad to poll: with none connected this waits on
// gamepadconnected instead of burning a callback every frame.
const GP_START = 9;
let gamepadStart = null;

function startPressed() {
  for (const gp of navigator.getGamepads()) {
    if (gp?.mapping !== "standard") continue;
    if (gp.buttons[GP_START]?.pressed) return true;
  }
  return false;
}

async function listenForGamepadStart(signal) {
  // The game itself exits on START, so it can still be held down when the menu
  // comes back. Only a fresh press counts.
  let wasPressed = true;

  while (!signal.aborted) {
    if (navigator.getGamepads().filter(Boolean).length === 0) {
      wasPressed = false;
      await new Promise((resolve) => {
        addEventListener("gamepadconnected", resolve, { once: true, signal });
        signal.addEventListener("abort", resolve, { once: true });
      });
      continue;
    }

    await new Promise((resolve) => requestAnimationFrame(resolve));
    if (signal.aborted) return;

    const pressed = startPressed();
    if (pressed && !wasPressed) {
      settings.requestSubmit();
      return;
    }
    wasPressed = pressed;
  }
}

function watchGamepadStart() {
  gamepadStart = new AbortController();
  listenForGamepadStart(gamepadStart.signal);
}

// The two platform features the port has no fallback for: JSPI, which is what
// lets the game's blocking main loop run in a browser at all, and the
// Uint8Array base64 methods that compat/tios.c saves through. Replacing the
// menu with the notice rather than hiding it takes the Start button - and the
// gamepad's START, which submits the same form - out of reach as well.
if (
  typeof WebAssembly.Suspending !== "function" ||
  typeof Uint8Array.fromBase64 !== "function" ||
  typeof Uint8Array.prototype.toBase64 !== "function"
) {
  settings.replaceWith(unsupported);
  unsupported.hidden = false;
} else {
  watchGamepadStart();
}

// ---------------------------------------------------------------------------
// The back gesture
//
// Starting a game pushes a history entry, so a phone's back swipe and the
// browser's back button come out of the game and land on the menu - the only
// thing back can sensibly mean on a page that never navigates.
//
// Going back reloads, because there is no way to stop the runtime from outside:
// main() blocks in Menus() and is suspended mid-frame by JSPI, with nothing to
// unwind it. Saves are in localStorage (see compat/tios.c), so a reload costs
// what abandoning a session costs anyway and no more.
// ---------------------------------------------------------------------------
let gameHistoryEntry = false;

function pushGameHistory() {
  history.pushState({ sm68k: "playing" }, "");
  gameHistoryEntry = true;
}

// Taken back out when the game exits on its own, so that back from the menu
// leaves the page rather than stepping through a menu that is already up.
function dropGameHistory() {
  if (!gameHistoryEntry) return;

  gameHistoryEntry = false;
  history.back();
}

addEventListener("popstate", () => {
  gameHistoryEntry = false;

  // dropGameHistory()'s own back() arrives here too, with the game already
  // gone and the menu already up: nothing left to leave.
  if (document.body.classList.contains("playing")) location.reload();
});

// Putting the menu back after the game exits. Resizing the canvas also clears
// the last frame the game left on it.
function showSettings() {
  document.body.classList.remove("playing");
  touchpad.hidden = true;
  releaseWakeLock();
  dropGameHistory();
  updateChrome();
  sizeCanvas(select.value);
  consoleBox.append(settings);
  watchGamepadStart();
}

function startGame() {
  gamepadStart?.abort();

  const calc = select.value;
  const lang = langSelect.value;
  try {
    localStorage.setItem(CALC_KEY, calc);
    localStorage.setItem(LANG_KEY, lang);
    localStorage.setItem(LATCH_KEY, String(latchOption.checked));
    localStorage.setItem(SPEEDRUN_KEY, String(speedrunOption.checked));
    localStorage.setItem(PRACTICE_KEY, String(practiceOption.checked));
    localStorage.setItem(
      KEYBOARD_INPUTS_KEY,
      String(keyboardInputsOption.checked),
    );
    localStorage.setItem(
      GAMEPAD_INPUTS_KEY,
      String(gamepadInputsOption.checked),
    );
  } catch {
    /* empty */
  }

  settings.remove();
  stopCapture();
  // The pads are laid out over the page and shell.css gives the canvas the rest
  // of the viewport; both only apply on a device whose primary pointer is a
  // finger, so a desktop sees neither.
  document.body.classList.add("playing");
  touchpad.hidden = false;
  acquireWakeLock();
  latchRun = latchOption.checked;
  pushGameHistory();
  updateChrome();
  fitCanvas();

  const keys = listenForGameKeys();

  // Absent unless there is a timer to report to, which is what src/speedrun.cpp
  // checks for before building an event at all.
  const timer = speedrunOption.checked ? speedrun : null;

  // The same, for the practice panel: absent unless it was asked for, which is
  // what src/practice.cpp checks for once and then never asks about again.
  const room = practiceOption.checked ? practice : null;

  // run() awaits main(), which JSPI makes asynchronous, so this promise
  // settling means the game itself has exited.
  createMario({
    canvas,
    ti89Mode: calc === "ti89",
    maTexts: maTexts[lang].texts,
    print: (t) => console.log(t),
    printErr: (t) => console.error(t),
    onRuntimeInitialized: () =>
      console.log("runtime ready, data mounted at /data"),
    onAbort: (w) => console.error("ABORT: " + w),
    gameActions,
    keyPressPromises,
    // Its answer goes back to src/speedrun.cpp, which throws the game out of
    // wherever it is when that answer is true. The timer only says so for a
    // recording that has just finished: the run is over, the panel is showing
    // the route it wrote, and there is no reason to keep the keyboard.
    //
    // Practice mode reports through the same hook, to the run history rather
    // than to a timer: it is kept, marked as practice, and never timed. Its
    // answer is never true - practice has no recording to finish.
    onSpeedrunEvent: timer
      ? (event) => timer.handle(event)
      : room && runHistory
        ? (event) => runHistory.practice(event)
        : undefined,
    // Polled from the world map, and answered with a warp, a powerup or an item
    // list when the player has asked the panel for one.
    onPracticeRequest: room ? (status) => room.handle(status) : undefined,
    // gray.c only resizes the canvas if the game asks for a screen other than
    // the one the menu sized it to, but if it ever does, the fitted display
    // size has to be recomputed for the new aspect.
    onCanvasResize: fitCanvas,
  })
    .catch((e) => console.error("exited with an error:", e))
    .finally(() => {
      // Whatever the game left open is over now, whether or not it said so:
      // a practice game from a save never goes back through a new game's
      // return to the menu.
      runHistory?.end();
      keys.abort();
      pressedCodes.clear();
      clearTouchActions();
      keyPressPromises.keydown = null;
      showSettings();
    });
}

settings.addEventListener("submit", (e) => {
  e.preventDefault();
  startGame();
});
