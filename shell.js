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
      pressedKeys.add(e.key);
      keyPressPromises.keydown?.();
      keyPressPromises.keydown = null;
    },
    opts,
  );

  addEventListener(
    "keyup",
    (e) => {
      if (!gameKeys.has(e.key)) return;

      e.preventDefault();
      pressedKeys.delete(e.key);
    },
    opts,
  );

  // A window that loses focus never sees the keyup, which would otherwise
  // leave the game running into a wall.
  addEventListener("blur", () => pressedKeys.clear(), opts);

  return keys;
}

// main() reads Module.ti89Mode once at startup and the texts are handed over
// with it, so both the calculator and the language have to be picked before the
// runtime is created. The choices are remembered for next time; storage can be
// unavailable (private mode, blocked cookies), in which case the menu just falls
// back to its defaults.
const CALC_KEY = "sm68k.calc";
const LANG_KEY = "sm68k.lang";
const consoleBox = document.querySelector(".console");
const settings = document.getElementById("settings");
const select = settings.elements.calc;
const langSelect = settings.elements.lang;
const canvas = document.getElementById("canvas");
const unsupported = document.getElementById("unsupported");

// The screens the two builds draw to, from init_calc_screen_constants() in
// render.c, and the scale compat/gray.c displays them at. Sizing the canvas
// from here shows the difference between the versions while choosing, and
// leaves the canvas at the size the game wants, so gray.c never resizes it.
const SCALE = 3;
const CALCS = {
  ti92: { width: 240, height: 128 },
  ti89: { width: 160, height: 100 },
};

function sizeCanvas(calc) {
  const { width, height } = CALCS[calc];

  canvas.width = width;
  canvas.height = height;
  canvas.style.width = width * SCALE + "px";
  canvas.style.height = height * SCALE + "px";
}

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
} catch {
  /* empty */
}

sizeCanvas(select.value);
select.addEventListener("change", () => sizeCanvas(select.value));

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
  sizeCanvas(select.value);
  consoleBox.append(settings);
  watchGamepadStart();
}

settings.addEventListener("submit", (e) => {
  e.preventDefault();
  gamepadStart.abort();

  const calc = select.value;
  const lang = langSelect.value;
  try {
    localStorage.setItem(CALC_KEY, calc);
    localStorage.setItem(LANG_KEY, lang);
  } catch {
    /* empty */
  }

  settings.remove();
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
  })
    .catch((e) => console.error("exited with an error:", e))
    .finally(() => {
      keys.abort();
      pressedKeys.clear();
      keyPressPromises.keydown = null;
      showSettings();
    });
});
