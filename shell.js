// Host-page entry module. mario.mjs is the Emscripten build compiled with
// -sEXPORT_ES6: its default export is a factory that takes the Module config
// and returns a promise for the running instance. The preloaded /data image is
// mounted into it before the runtime starts. All runtime output goes to the
// browser console.
import createMario from "./mario.js";
import maTexts from "./ma_texts.json" with { type: "json" };

// pageflip_wait() in compat/gray.c reads Module.frameMs before every frame and
// paces on requestAnimationFrame. CALC_FPS is what the calculator ran at; on a
// display refreshing slower than that the game just tracks the refresh rate.
const CALC_FPS = 30;

const gameKeys = new Set([
  "Enter",
  "Escape",
  " ",
  "ArrowUp",
  "ArrowDown",
  "ArrowLeft",
  "ArrowRight",
  "Shift",
]);
const gameGpButtons = new Set([0, 1, 2, 3, 8, 9, 12, 13, 14, 15]);
const pressedKeys = new Set();

function pressedGpButtons() {
  const ret = new Set();

  for (const gp of navigator.getGamepads()) {
    if (gp?.mapping !== "standard") continue;

    for (const button of gameGpButtons) {
      if (gp.buttons[button].pressed) {
        ret.add(button);
      }
      for (let i = 0; i < 4; i++) {
        if (Math.round(gp.axes[i]) !== 0) {
          ret.add(i + 0x100);
        }
      }
    }
  }

  return ret;
}

// Passed by the resolve function by code in scankeys.c
const keyPressPromises = { keydown: null };

// The one way a key is pressed, whichever input pressed it: the keyboard
// listeners below and the touch pads further down both go through here, so
// scankeys.c only ever sees the one set of key names. Waking a waiting
// WaitKeyPress() on every press rather than only on a new one matches what the
// keyboard's auto-repeat used to do.
function pressKey(key) {
  pressedKeys.add(key);
  keyPressPromises.keydown?.();
  keyPressPromises.keydown = null;
}

function releaseKey(key) {
  pressedKeys.delete(key);
}

// The game's key handling is only bound while it runs: outside of that the
// settings menu needs Enter and the arrow keys for itself. The controller is
// what unbinds all three listeners again when main() returns.
function listenForGameKeys() {
  const keys = new AbortController();
  const opts = { signal: keys.signal };

  addEventListener(
    "keydown",
    (e) => {
      if (!gameKeys.has(e.key)) return;

      e.preventDefault();
      pressKey(e.key);
    },
    opts,
  );

  addEventListener(
    "keyup",
    (e) => {
      if (!gameKeys.has(e.key)) return;

      e.preventDefault();
      releaseKey(e.key);
    },
    opts,
  );

  // A window that loses focus never sees the keyup, which would otherwise
  // leave the game running into a wall. A backgrounded phone browser stops
  // delivering pointer events just the same, so the pads are let go too.
  addEventListener(
    "blur",
    () => {
      pressedKeys.clear();
      clearTouchKeys();
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
const consoleBox = document.querySelector(".console");
const settings = document.getElementById("settings");
const select = settings.elements.calc;
const langSelect = settings.elements.lang;
const latchOption = settings.elements.latch;
const canvas = document.getElementById("canvas");
const unsupported = document.getElementById("unsupported");

// The screens the two builds draw to, from init_calc_screen_constants() in
// render.c, and the largest scale they are displayed at. Sizing the canvas from
// here shows the difference between the versions while choosing, and leaves the
// canvas at the size the game wants, so gray.c never resizes it.
const SCALE = 3;
const CALCS = {
  ti92: { width: 240, height: 128 },
  ti89: { width: 160, height: 100 },
};

// How much of the viewport height the menu leaves the canvas. The rest is for
// the title above it and, on a narrow screen where the menu no longer fits over
// the canvas, the settings below it.
const MENU_HEIGHT_SHARE = 0.6;

// A phone screen is nowhere near 720px wide, so the canvas is displayed at
// whatever scale up to 3x the space actually allows. The pixel size is left
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

  const availWidth = document.body.clientWidth - chrome;
  const availHeight = immersive ? innerHeight : innerHeight * MENU_HEIGHT_SHARE;
  // 3x is as large as the calculator's screen wants to be in the middle of a
  // page, but a screen given over to the game entirely should fill it.
  const cap = isFullscreen() ? Infinity : SCALE;
  const scale = Math.max(
    Math.min(cap, availWidth / width, availHeight / height),
    1,
  );

  canvas.style.width = Math.round(width * scale) + "px";
  canvas.style.height = Math.round(height * scale) + "px";
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
} catch {
  /* empty */
}

sizeCanvas(select.value);
select.addEventListener("change", () => sizeCanvas(select.value));

// ---------------------------------------------------------------------------
// On-screen controls
//
// The pads in index.html name game keys with tokens rather than the key names
// themselves, because one of those names is a space and data-keys is read as a
// space-separated token list - the corner pads of the direction pad name two
// directions each, which is how a diagonal is held.
// ---------------------------------------------------------------------------
const TOUCH_KEYS = {
  up: "ArrowUp",
  down: "ArrowDown",
  left: "ArrowLeft",
  right: "ArrowRight",
  jump: " ",
  run: "Shift",
  enter: "Enter",
  esc: "Escape",
};

const touchpad = document.getElementById("touchpad");
const touchIsPrimary = matchMedia("(pointer: coarse)");

// B is a switch by default rather than a key to be held: running in this game
// means holding Run for as long as you want to move, which is a lot to ask of a
// thumb that is also aiming at A. The pad marked data-latch is the one this
// applies to, and the menu option turns it back into a held key.
let latchRun = true;
const latchedKeys = new Set();

function isLatching(pad) {
  return latchRun && pad.hasAttribute("data-latch");
}

// Which pad each finger currently rests on, and the keys those pads are holding
// down. The keys are tracked separately from pressedKeys so that letting go of
// a pad only releases what touch pressed: a player with both a keyboard and a
// touchscreen can hold Shift and tap Jump without the tap cancelling the Shift.
const padUnderPointer = new Map();
const touchHeld = new Set();

// Hit testing on every move rather than per-element enter/leave listeners, so
// that sliding a thumb from left to right across the pad - or from Run onto
// Jump - hands over cleanly, the way a physical d-pad does.
function padAt(x, y) {
  return document.elementFromPoint(x, y)?.closest("[data-keys]") ?? null;
}

function syncTouchKeys() {
  // A latched key is wanted until it is tapped off again, whether or not a
  // finger is still on the pad, so it joins what the pointers are asking for
  // rather than being tracked apart from it.
  const wanted = new Set(latchedKeys);

  for (const pad of padUnderPointer.values()) {
    for (const token of pad.dataset.keys.split(" ")) wanted.add(TOUCH_KEYS[token]);
  }

  for (const key of touchHeld) {
    if (wanted.has(key)) continue;

    touchHeld.delete(key);
    releaseKey(key);
  }

  for (const key of wanted) {
    if (touchHeld.has(key)) continue;

    touchHeld.add(key);
    pressKey(key);
  }

  const held = new Set(padUnderPointer.values());
  for (const pad of touchpad.querySelectorAll("[data-keys]")) {
    const latched =
      isLatching(pad) && latchedKeys.has(TOUCH_KEYS[pad.dataset.keys]);

    pad.classList.toggle("held", held.has(pad) || latched);
  }
}

function clearTouchKeys() {
  padUnderPointer.clear();
  latchedKeys.clear();
  syncTouchKeys();
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
    const key = TOUCH_KEYS[pad.dataset.keys];

    if (!latchedKeys.delete(key)) latchedKeys.add(key);
    syncTouchKeys();
    return;
  }

  try {
    touchpad.setPointerCapture(e.pointerId);
  } catch {
    /* A pointer that is no longer active cannot be captured; the pad still
       presses, and the release below still frees it. */
  }
  padUnderPointer.set(e.pointerId, pad);
  syncTouchKeys();
});

touchpad.addEventListener("pointermove", (e) => {
  if (!padUnderPointer.has(e.pointerId)) return;

  const pad = padAt(e.clientX, e.clientY);
  if (pad) {
    padUnderPointer.set(e.pointerId, pad);
  } else {
    padUnderPointer.delete(e.pointerId);
  }
  syncTouchKeys();
});

for (const type of ["pointerup", "pointercancel"]) {
  touchpad.addEventListener(type, (e) => {
    if (padUnderPointer.delete(e.pointerId)) syncTouchKeys();
  });
}

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
// player: without it, going fullscreen would only put the same 720px canvas in
// the middle of a bigger empty page.
function updateChrome() {
  const playing = document.body.classList.contains("playing");

  fullscreenToggle.hidden =
    !document.fullscreenEnabled || (!playing && !isFullscreen());
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

// Putting the menu back after the game exits. Resizing the canvas also clears
// the last frame the game left on it.
function showSettings() {
  document.body.classList.remove("playing");
  touchpad.hidden = true;
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
  } catch {
    /* empty */
  }

  settings.remove();
  // The pads are laid out over the page and shell.css gives the canvas the rest
  // of the viewport; both only apply on a device whose primary pointer is a
  // finger, so a desktop sees neither.
  document.body.classList.add("playing");
  touchpad.hidden = false;
  latchRun = latchOption.checked;
  updateChrome();
  fitCanvas();

  const keys = listenForGameKeys();

  // run() awaits main(), which JSPI makes asynchronous, so this promise
  // settling means the game itself has exited.
  createMario({
    canvas,
    ti89Mode: calc === "ti89",
    maTexts: maTexts[lang].texts,
    frameMs: 1000 / CALC_FPS,
    print: (t) => console.log(t),
    printErr: (t) => console.error(t),
    onRuntimeInitialized: () =>
      console.log("runtime ready, data mounted at /data"),
    onAbort: (w) => console.error("ABORT: " + w),
    pressedKeys,
    pressedGpButtons,
    keyPressPromises,
    // gray.c only resizes the canvas if the game asks for a screen other than
    // the one the menu sized it to, but if it ever does, the fitted display
    // size has to be recomputed for the new aspect.
    onCanvasResize: fitCanvas,
  })
    .catch((e) => console.error("exited with an error:", e))
    .finally(() => {
      keys.abort();
      pressedKeys.clear();
      clearTouchKeys();
      keyPressPromises.keydown = null;
      showSettings();
    });
}

settings.addEventListener("submit", (e) => {
  e.preventDefault();
  startGame();
});
