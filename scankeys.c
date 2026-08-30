#include "scankeys.h"
#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <stddef.h>
#include <stdint.h>

struct keystate Keystate, Previous_keystate;

EM_ASYNC_JS(void, browser_wait_no_keys_held, (void), {
	// clang-format off
  while (Module.pressedKeys.size !== 0 || Module.pressedGpButtons().size !== 0) {
    await new Promise((resolve) => { requestAnimationFrame(resolve) });
  }
	// clang-format on
});

EM_ASYNC_JS(void, browser_keydown_event, (void), {
	// clang-format off
  const abortController = new AbortController();
  const signal = abortController.signal;

  const keydown = new Promise((resolve, reject) => {
    Module.keyPressPromises.keydown = resolve;
    signal.addEventListener("abort", reject, {once: true});
  });
  const gpPress = (async () => {
    let originalButtons;

    // Don't busy-poll if there isn't a gamepad connected
    if (navigator.getGamepads().filter(Boolean).length === 0) {
      await new Promise((resolve, reject) => {
        addEventListener("gamepadconnected", resolve, {once: true, signal});
        signal.addEventListener("abort", reject, {once: true});
      });
      originalButtons = new Set();
    } else {
      originalButtons = Module.pressedGpButtons();
    }

    do {
      await new Promise((resolve, reject) => {
        if (signal.aborted) reject();
        requestAnimationFrame(resolve);
      });
    } while (Module.pressedGpButtons().difference(originalButtons).size === 0);
  })();
  await Promise.any([keydown, gpPress]);
  Module.keyPressPromises.keydown = null;
  abortController.abort();
	// clang-format on
});

void WaitKeyReleased()
{
	browser_wait_no_keys_held();
}
// TODO: support analog sticks in the menus
void WaitKeyPress()
{
	browser_wait_no_keys_held();
	browser_keydown_event();
};

void ScanKeys(void)
{
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

        // Handle gamepads. Only consider `standard' gamepads.
        for (const gp of navigator.getGamepads()) {
          if (gp?.mapping !== "standard") continue;

          keystate[enter] |= gp.buttons[8]?.pressed;
          keystate[esc] |= gp.buttons[9]?.pressed;
          keystate[jump] |= gp.buttons[0]?.pressed | gp.buttons[1]?.pressed;
          keystate[up] |= gp.buttons[12]?.pressed;
          keystate[down] |= gp.buttons[13]?.pressed;
          keystate[left] |= gp.buttons[14]?.pressed;
          keystate[right] |= gp.buttons[15]?.pressed;
          keystate[run] |= gp.buttons[2]?.pressed | gp.buttons[3]?.pressed;

          const ud1 = Math.round(gp.axes[1]);
          const ud2 = Math.round(gp.axes[3]);
          keystate[up] |= ud1 === -1 | ud2 === -1;
          keystate[down] |= ud1 === 1 | ud2 === 1;
          const lr1 = Math.round(gp.axes[0]);
          const lr2 = Math.round(gp.axes[2]);
          keystate[left] |= lr1 === -1 | lr2 === -1;
          keystate[right] |= lr1 === 1 | lr2 === 1;
        }
			// clang-format on
		},
		&Keystate, offsetof(struct keystate, enter),
		offsetof(struct keystate, esc), offsetof(struct keystate, jump),
		offsetof(struct keystate, up), offsetof(struct keystate, down),
		offsetof(struct keystate, left),
		offsetof(struct keystate, right),
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