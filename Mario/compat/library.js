addToLibrary({
  // Takes id, the enum for which set of strings, writes into arr
  get_menu_string(id, arr, len) {
    for (let i = 0; i < len; i++) {
      HEAPU32[arr / 4 + i] = 0;
    }

    len = Math.min(len, processedText[id].length);

    for (let i = 0; i < len; i++) {
      const str = processedText[id][i];
      const addr = _malloc(str.length + 1);
      const target = HEAPU8.subarray(addr);
      encodeTextInto(str, target);
      target[str.length] = 0;
      HEAPU32[arr / 4 + i] = addr;
    }
  },

  pageflip_sync: async (renderbuf) => {
    const LCD_SIZE = 3840;

    // light = HEAPU8.subarray(light, light + LCD_SIZE);
    // dark = HEAPU8.subarray(dark, dark + LCD_SIZE);

    // const out = new Uint8ClampedArray(LCD_SIZE * 8 * 4);

    // const lut = new Uint8Array([255, 192, 96, 0]);

    // for (let i = 0; i < LCD_SIZE; i++) {
    //   const l = light[i],
    //     d = dark[i];
    //   for (let j = 7; j >= 0; j--) {
    //     const id = ((l >> j) & 1) | (((d >> j) & 1) << 1);
    //     for (let k = 0; k < 3; k++) out[i * 8 * 4 + (8 - j) * 4 + k] = lut[id];
    //     out[i * 8 * 4 + (8 - j) * 4 + 3] = 255;
    //   }
    // }

    const view = new Uint8ClampedArray(
      HEAPU8.buffer,
      renderbuf,
      LCD_SIZE * 8 * 4,
    );

    const ctx = document
      .getElementById("canvas")
      .getContext("2d", { alpha: false });
    //ctx.fillRect(0, 0, 100, 100);
    const image = new ImageData(view, 240, 128);
    ctx.putImageData(image, 0, 0);

    // https://stackoverflow.com/a/19772220
    let now = performance.now();
    let elapsed;
    while (Math.round((elapsed = now - prevFrameTime)) < 50) {
      now = await new Promise((resolve) => requestAnimationFrame(resolve));
    }

    prevFrameTime = now - (elapsed % 50);
  },

  pageflip_sync__async: true,
});
