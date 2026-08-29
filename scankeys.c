#include "scankeys.h"
#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <stddef.h>
#include <stdint.h>

struct keystate Keystate, Previous_keystate;

EM_ASYNC_JS(void, browser_wait_no_keys_held, (void), {
  // clang-format off
  while (Module.pressedKeys.size !== 0) {
    await new Promise((resolve) => { Module.keyPressPromises.keyup = resolve; });
  }
  // clang-format on
});

EM_ASYNC_JS(void, browser_keydown_event, (void), {
  // clang-format off
  await new Promise((resolve) => { Module.keyPressPromises.keydown = resolve; });
  // clang-format on
});

void WaitKeyReleased() { browser_wait_no_keys_held(); }

void WaitKeyPress() {
  browser_wait_no_keys_held();
  browser_keydown_event();
};

void ScanKeys(void) {
  EM_ASM(
      {
        // clang-format off
        const enter = $1;
        const esc = $2;
        const jump = $3;
        const up = $4;
        const down = $5;
        const left = $6;
        const right = $7;
        const run = $8;

        const keystate = new Uint8Array(HEAPU8.buffer, $0);
        const pressedKeys = Module.pressedKeys;

        keystate[enter] = pressedKeys.has("Enter");
        keystate[esc] = pressedKeys.has("Escape");
        keystate[jump] = pressedKeys.has(" ");
        keystate[up] = pressedKeys.has("ArrowUp");
        keystate[down] = pressedKeys.has("ArrowDown");
        keystate[left] = pressedKeys.has("ArrowLeft");
        keystate[right] = pressedKeys.has("ArrowRight");
        keystate[run] = pressedKeys.has("Shift");
        // clang-format on
      },
      &Keystate, offsetof(struct keystate, enter),
      offsetof(struct keystate, esc), offsetof(struct keystate, jump),
      offsetof(struct keystate, up), offsetof(struct keystate, down),
      offsetof(struct keystate, left), offsetof(struct keystate, right),
      offsetof(struct keystate, run));

  bool MaskLeft = false;
  if (Keystate.left && Keystate.right && Previous_keystate.right) {
    MaskLeft = true;
  }
  if (Keystate.right && Keystate.left && Previous_keystate.left) {
    Keystate.right = 0;
  }
  if (MaskLeft) {
    Keystate.left = 0;
  }
}