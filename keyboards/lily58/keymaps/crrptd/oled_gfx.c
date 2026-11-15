#include "oled_gfx.h"

#include <string.h>
#include "oled_driver.h"

static int16_t clip_top = 0;
static int16_t clip_bot = GFX_H;

// Frames are drawn here, then only the bytes that changed go to the OLED
// buffer, so the driver re-sends a few blocks instead of the whole screen.
// Same layout as the driver's buffer in portrait: 32 columns x 16 pages.
static uint8_t frame[GFX_W * GFX_H / 8];

void gfx_begin(void) {
    memset(frame, 0, sizeof(frame));
    gfx_noclip();
}

void gfx_end(void) {
    for (uint16_t i = 0; i < sizeof(frame); i++) {
        oled_write_raw_byte((char)frame[i], i);
    }
}

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
    uint8_t *b = &frame[x + (y / 8) * GFX_W];
    if (on) {
        *b |= 1 << (y % 8);
    } else {
        *b &= ~(1 << (y % 8));
    }
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
            } else if (rows[j][i] == 'o') {
                gfx_px(x + i, y + j, false);
            }
        }
    }
}

void gfx_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool on) {
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int16_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int16_t e  = dx + dy;
    for (;;) {
        gfx_px(x0, y0, on);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int16_t e2 = 2 * e;
        if (e2 >= dy) {
            e += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            e += dx;
            y0 += sy;
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

int16_t gfx_text_x(uint8_t len, uint8_t scale) {
    return (GFX_W - ((int16_t)len * 4 - 1) * scale) / 2;
}

void gfx_text_scaled(const char *s, int16_t x, int16_t y, uint8_t scale) {
    if (x < 0) {
        x = gfx_text_x(strlen(s), scale);
    }
    for (; *s != '\0'; s++, x += 4 * scale) {
        const glyph_t *g = find_glyph(*s);
        if (g == NULL) {
            continue;
        }
        for (int16_t r = 0; r < 5; r++) {
            for (int16_t b = 0; b < 3; b++) {
                if ((g->rows[r] >> (2 - b)) & 1) {
                    gfx_frect(x + b * scale, y + r * scale, scale, scale, true);
                }
            }
        }
    }
}

void gfx_text(const char *s, int16_t x, int16_t y) {
    gfx_text_scaled(s, x, y, 1);
}
