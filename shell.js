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

createMario({
  canvas: document.getElementById("canvas"),
  frameMs: 1000 / CALC_FPS,
  print: (t) => console.log(t),
  printErr: (t) => console.error(t),
  onRuntimeInitialized: () => console.log("runtime ready, data mounted at /data"),
  onAbort: (w) => console.error("ABORT: " + w),
}).catch((e) => console.error("failed to start:", e));
