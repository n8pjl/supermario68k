// Host-page glue for the Emscripten build. Loaded before mario.js, which picks
// up the global Module this sets up and mounts the preloaded /data image into
// it once the runtime is ready. All runtime output goes to the browser console.

// pageflip_wait() in compat/gray.c reads Module.frameMs before every frame and
// paces on requestAnimationFrame. CALC_FPS is what the calculator ran at; on a
// display refreshing slower than that the game just tracks the refresh rate.
const CALC_FPS = 30;

window.Module = {
  canvas: document.getElementById("canvas"),
  frameMs: 1000 / CALC_FPS,
  print: (t) => console.log(t),
  printErr: (t) => console.error(t),
  onRuntimeInitialized: () => console.log("runtime ready, data mounted at /data"),
  onAbort: (w) => console.error("ABORT: " + w),
};
