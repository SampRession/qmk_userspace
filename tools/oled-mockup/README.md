# OLED mockups

Animated mockups of the Lily58 OLEDs (portrait, 32x128): the layer stack on the
right (master) half, the mascot Lily on the left half, and the boot logo.

- `core.js`: the drawing engine (1-bit framebuffer, shapes, 3x5 font, layers,
  Lily). Written to be ported almost line by line to C with
  `oled_write_pixel()`.
- `template.html`: the page; `build.py` inlines `core.js` and the logo from
  `keyboards/lily58/keymaps/crrptd/lily58_logo.h` into `lily58-oled.html`.
- `preview_ascii.js`: renders frames as ASCII in the terminal, e.g.
  `node preview_ascii.js ./core.js A 0 128 1 n BASE,GAMING`
  (variant A/B/C, rows 0-128, frame 1, `s` = sleeping, layers).

Rebuild with `python3 build.py`, then open `lily58-oled.html` in a browser.
