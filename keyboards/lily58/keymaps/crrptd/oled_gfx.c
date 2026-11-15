#include "oled_gfx.h"

#include <string.h>
#include "oled_driver.h"

static int16_t clip_top = 0;
static int16_t clip_bot = GFX_H;

void gfx_clip(int16_t top, int16_t bot) {
    clip_top = top;
    clip_bot = bot;
}

void gfx_noclip(void) {
    gfx_clip(0, GFX_H);
}

void gfx_px(int16_t x, int16_t y, bool on) {
    if (x < 0 || x >= GFX_W || y < clip_top || y >= clip_bot) {
        return;
    }
    oled_write_pixel((uint8_t)x, (uint8_t)y, on);
}

void gfx_frect(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
    for (int16_t i = 0; i < w; i++) {
        for (int16_t j = 0; j < h; j++) {
            gfx_px(x + i, y + j, on);
        }
    }
}

void gfx_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t i = 0; i < w; i++) {
        gfx_px(x + i, y, true);
        gfx_px(x + i, y + h - 1, true);
    }
    for (int16_t j = 0; j < h; j++) {
        gfx_px(x, y + j, true);
        gfx_px(x + w - 1, y + j, true);
    }
}

void gfx_sprite(const char *const *rows, uint8_t n, int16_t x, int16_t y) {
    for (uint8_t j = 0; j < n; j++) {
        for (int16_t i = 0; rows[j][i] != '\0'; i++) {
            if (rows[j][i] == '#') {
                gfx_px(x + i, y + j, true);
            }
        }
    }
}

// One row = 3 bits, MSB on the left.
typedef struct {
    char    c;
    uint8_t rows[5];
} glyph_t;

static const glyph_t font[] = {
    {'A', {2, 5, 7, 5, 5}}, {'B', {6, 5, 6, 5, 6}}, {'C', {3, 4, 4, 4, 3}}, {'D', {6, 5, 5, 5, 6}},
    {'E', {7, 4, 6, 4, 7}}, {'F', {7, 4, 6, 4, 4}}, {'G', {3, 4, 5, 5, 3}}, {'H', {5, 5, 7, 5, 5}},
    {'I', {7, 2, 2, 2, 7}}, {'J', {1, 1, 1, 5, 2}}, {'K', {5, 5, 6, 5, 5}}, {'L', {4, 4, 4, 4, 7}},
    {'M', {5, 7, 7, 5, 5}}, {'N', {6, 5, 5, 5, 5}}, {'O', {2, 5, 5, 5, 2}}, {'P', {6, 5, 6, 4, 4}},
    {'Q', {2, 5, 5, 6, 3}}, {'R', {6, 5, 6, 5, 5}}, {'S', {3, 4, 2, 1, 6}}, {'T', {7, 2, 2, 2, 2}},
    {'U', {5, 5, 5, 5, 7}}, {'V', {5, 5, 5, 5, 2}}, {'W', {5, 5, 5, 7, 5}}, {'X', {5, 5, 2, 5, 5}},
    {'Y', {5, 5, 2, 2, 2}}, {'Z', {7, 1, 2, 4, 7}}, {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}},
    {'2', {6, 1, 2, 4, 7}}, {'3', {6, 1, 2, 1, 6}}, {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 6, 1, 6}},
    {'6', {3, 4, 6, 5, 2}}, {'7', {7, 1, 2, 2, 2}}, {'8', {2, 5, 2, 5, 2}}, {'9', {2, 5, 3, 1, 6}},
    {'!', {2, 2, 2, 0, 2}},
};

static const glyph_t *find_glyph(char c) {
    for (uint8_t i = 0; i < sizeof(font) / sizeof(font[0]); i++) {
        if (font[i].c == c) {
            return &font[i];
        }
    }
    return NULL; // space and unknown characters draw nothing
}

void gfx_text(const char *s, int16_t x, int16_t y) {
    if (x < 0) {
        x = (GFX_W - ((int16_t)strlen(s) * 4 - 1)) / 2;
    }
    for (; *s != '\0'; s++, x += 4) {
        const glyph_t *g = find_glyph(*s);
        if (g == NULL) {
            continue;
        }
        for (int16_t r = 0; r < 5; r++) {
            for (int16_t b = 0; b < 3; b++) {
                if ((g->rows[r] >> (2 - b)) & 1) {
                    gfx_px(x + b, y + r, true);
                }
            }
        }
    }
}
