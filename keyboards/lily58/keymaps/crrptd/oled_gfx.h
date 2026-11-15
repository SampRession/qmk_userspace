// Small 1-bit drawing helpers for the portrait OLED (32x128).
// Ported from tools/oled-mockup/core.js.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GFX_W 32
#define GFX_H 128

// Pixels outside [top, bot) are dropped, so animations can scroll through a zone.
void gfx_clip(int16_t top, int16_t bot);
void gfx_noclip(void);

void gfx_px(int16_t x, int16_t y, bool on);
void gfx_frect(int16_t x, int16_t y, int16_t w, int16_t h, bool on);
void gfx_rect(int16_t x, int16_t y, int16_t w, int16_t h);

// Draws `rows` strings of '#' (on) and '.' (left untouched) at (x, y).
void gfx_sprite(const char *const *rows, uint8_t n, int16_t x, int16_t y);

// 3x5 font, upper case letters, digits, '!' and space. Centred when x < 0.
void gfx_text(const char *s, int16_t x, int16_t y);
