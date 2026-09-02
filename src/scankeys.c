#include "scankeys.h"
#include <emscripten/em_asm.h>
#include <emscripten/em_js.h>
#include <stddef.h>
#include <stdint.h>

struct keystate Keystate, Previous_keystate;

// What a key or a controller button means is the shell's business: it is what
// holds the bindings, and what the player edits them through. Module.gameActions
// hands over the eight actions this game knows about, already resolved from
// whatever they are bound to - a key, a pad button, a stick, an on-screen pad -
// so nothing below has to know a key name.
EM_ASYNC_JS(void, browser_wait_no_actions_held, (void), {
	// clang-format off
  while (Object.values(Module.gameActions()).some(Boolean)) {
    await new Promise((resolve) => { requestAnimationFrame(resolve) });
  }
	// clang-format on
});

// Waits for an action to be pressed. The keyboard and the on-screen pads resolve
// the shell's promise when they press one; a controller has nothing to fire an
// event, so it is polled - but only once there is one connected, rather than
// burning a callback every frame when there is not.
EM_ASYNC_JS(void, browser_action_pressed, (void), {
	// clang-format off
  const abortController = new AbortController();
  const signal = abortController.signal;

  const pressed = new Promise((resolve, reject) => {
    Module.keyPressPromises.keydown = resolve;
    signal.addEventListener("abort", reject, {once: true});
  });
  const gpPressed = (async () => {
    if (navigator.getGamepads().filter(Boolean).length === 0) {
      await new Promise((resolve, reject) => {
        addEventListener("gamepadconnected", resolve, {once: true, signal});
        signal.addEventListener("abort", reject, {once: true});
      });
    }

    // The caller waits for everything to be let go of first, so anything
    // active by now is a fresh press and there is no baseline to compare to.
    while (!Object.values(Module.gameActions()).some(Boolean)) {
      await new Promise((resolve, reject) => {
        if (signal.aborted) reject();
        requestAnimationFrame(resolve);
      });
    }
  })();

  await Promise.any([pressed, gpPressed]);
  Module.keyPressPromises.keydown = null;
  abortController.abort();
	// clang-format on
});

void WaitKeyReleased()
{
	browser_wait_no_actions_held();
}

void WaitKeyPress()
{
	browser_wait_no_actions_held();
	browser_action_pressed();
};

void ScanKeys(void)
{
	EM_ASM(
		{
			// clang-format off
        const actions = Module.gameActions();
        const keystate = new Uint8Array(HEAPU8.buffer, $0);

        keystate[$1] = actions.enter;
        keystate[$2] = actions.esc;
        keystate[$3] = actions.jump;
        keystate[$4] = actions.up;
        keystate[$5] = actions.down;
        keystate[$6] = actions.left;
        keystate[$7] = actions.right;
        keystate[$8] = actions.run;
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
