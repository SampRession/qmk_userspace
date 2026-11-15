// Small 1-bit drawing helpers for the portrait OLED (32x128).
// Ported from tools/oled-mockup/core.js.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GFX_W 32
#define GFX_H 128

// Draw a frame between gfx_begin() (blank frame) and gfx_end() (sends the
// changes to the OLED driver).
void gfx_begin(void);
void gfx_end(void);

// Pixels outside [top, bot) are dropped, so animations can scroll through a zone.
void gfx_clip(int16_t top, int16_t bot);
void gfx_noclip(void);

void gfx_px(int16_t x, int16_t y, bool on);
void gfx_frect(int16_t x, int16_t y, int16_t w, int16_t h, bool on);
void gfx_rect(int16_t x, int16_t y, int16_t w, int16_t h);

void gfx_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on);

// Draws `rows` strings at (x, y): '#' = on, 'o' = off, anything else is left untouched.
void gfx_sprite(const char *const *rows, uint8_t n, int16_t x, int16_t y);

// 3x5 font, upper case letters, digits, '!' and space. Centred when x < 0.
void gfx_text(const char *s, int16_t x, int16_t y);
// Same, each font pixel drawn as a scale x scale square.
void gfx_text_scaled(const char *s, int16_t x, int16_t y, uint8_t scale);
// Left edge that centres `len` characters at this scale.
int16_t gfx_text_x(uint8_t len, uint8_t scale);
