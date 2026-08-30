// Host-page entry module. mario.mjs is the Emscripten build compiled with
// -sEXPORT_ES6: its default export is a factory that takes the Module config
// and returns a promise for the running instance. The preloaded /data image is
// mounted into it before the runtime starts. All runtime output goes to the
// browser console.
import createMario from "./mario.mjs";

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

addEventListener("keydown", (e) => {
  if (!gameKeys.has(e.key)) return;

  e.preventDefault();
  pressedKeys.add(e.key);
  keyPressPromises.keydown?.();
  keyPressPromises.keydown = null;
});

addEventListener("keyup", (e) => {
  if (!gameKeys.has(e.key)) return;

  e.preventDefault();
  pressedKeys.delete(e.key);
});

// A window that loses focus never sees the keyup, which would otherwise
// leave the game running into a wall.
addEventListener("blur", () => {
  pressedKeys.clear();
});

createMario({
  canvas: document.getElementById("canvas"),
  frameMs: 1000 / CALC_FPS,
  print: (t) => console.log(t),
  printErr: (t) => console.error(t),
  onRuntimeInitialized: () =>
    console.log("runtime ready, data mounted at /data"),
  onAbort: (w) => console.error("ABORT: " + w),
  pressedKeys,
  pressedGpButtons,
  keyPressPromises,
}).catch((e) => console.error("failed to start:", e));
