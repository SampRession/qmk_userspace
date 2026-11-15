// Right (master) OLED, portrait: layer name, an animated icon per layer, the
// layer stack of the current family, and a WPM gauge. The animation speeds up
// with the typing speed. Mockup and reference: tools/oled-mockup (variant A).
#include QMK_KEYBOARD_H

#include "layers.h"
#include "oled_gfx.h"
#include "oled_master.h"

#define NONE 0xFF

static const char *layer_name(uint8_t layer) {
    switch (layer) {
        case _QWERTY:  return "BASE";
        case _LOWER:   return "LOWER";
        case _RAISE:   return "RAISE";
        case _ADJUST:  return "ADJUST";
        case _HARDER:  return "HARDER";
        case _MIRROR:  return "MIRROR";
        case _LMIRROR: return "LMIRROR";
        case _GAMING1: return "GAMING";
        case _LGAMNG1: return "LGAMING";
        default:       return "?";
    }
}

static uint8_t tri6(uint16_t t) {
    static const uint8_t wave[6] = {0, 1, 2, 3, 2, 1};
    return wave[t % 6];
}

// ---- header and footer ----

static void draw_hud(uint8_t layer) {
    gfx_text(layer_name(layer), -1, 3);
    for (int16_t x = 2; x < 30; x += 2) {
        gfx_px(x, 11, true);
    }
}

// Three slabs: the floors of the current family (raise / base / lower).
static void draw_stack(uint8_t layer, uint16_t t) {
    static const uint8_t main_rows[3]   = {_RAISE, _QWERTY, _LOWER};
    static const uint8_t mirror_rows[3] = {NONE, _MIRROR, _LMIRROR};
    static const uint8_t gaming_rows[3] = {NONE, _GAMING1, _LGAMNG1};

    const uint8_t *rows = main_rows;
    if (layer == _MIRROR || layer == _LMIRROR) {
        rows = mirror_rows;
    } else if (layer == _GAMING1 || layer == _LGAMNG1) {
        rows = gaming_rows;
    }

    for (uint8_t i = 0; i < 3; i++) {
        int16_t y = 66 + i * 16;
        if (rows[i] == NONE) { // this family has no upper floor
            for (int16_t x = 4; x < 28; x += 2) {
                gfx_px(x, y, true);
                gfx_px(x, y + 9, true);
            }
            continue;
        }
        gfx_rect(4, y, 24, 10);
        bool active;
        if (layer == _ADJUST) {
            active = rows[i] == _RAISE || rows[i] == _LOWER;
        } else if (layer == _HARDER) {
            active = t % 4 < 2; // the whole stack blinks
        } else {
            active = rows[i] == layer;
        }
        if (active) {
            gfx_frect(4, y, 24, 10, true);
            gfx_frect(4 + (t * 2) % 24, y + 1, 2, 8, false); // moving glint
        }
    }
}

static void draw_gauge(uint8_t wpm) {
    uint8_t w = wpm > 120 ? 120 : wpm;
    gfx_rect(2, 118, 28, 6);
    gfx_frect(4, 120, (24 * w + 60) / 120, 2, true);
}

// ---- icons (zone y 14..60, centre y 37) ----

static void varrow(int16_t c, int16_t top, bool up) {
    for (int16_t i = 0; i < 6; i++) {
        int16_t y = up ? top + i : top + 11 - i;
        for (int16_t k = -i; k <= i; k++) {
            gfx_px(c + k, y, true);
        }
    }
    gfx_frect(c - 1, up ? top + 6 : top, 3, 6, true);
}

// dir -1 points left, +1 points right
static void harrow(int16_t tip, int16_t cy, int8_t dir) {
    for (int16_t i = 0; i < 5; i++) {
        for (int16_t k = -i; k <= i; k++) {
            gfx_px(tip - dir * i, cy + k, true);
        }
    }
    gfx_frect(dir < 0 ? tip + 5 : tip - 9, cy - 1, 5, 3, true);
}

// Warning sign. Coordinates are doubled so the apex sits between columns 15 and 16.
static void warning(bool filled) {
    for (int16_t y = 17; y <= 56; y++) {
        int16_t outer = 25 * (y - 17) / 39;
        int16_t inner = (y >= 23 && y <= 53) ? 17 * (y - 23) / 30 : -2;
        for (int16_t x = 2; x < 30; x++) {
            int16_t d = 2 * x - 31;
            if (d < 0) d = -d;
            if (d <= outer + 1) {
                gfx_px(x, y, filled || d > inner + 1);
            }
        }
    }
    gfx_frect(15, 30, 2, 13, !filled);
    gfx_frect(15, 46, 2, 3, !filled);
}

static const char *const gamepad_rows[] = {
    "...####............####...",
    "..######################..",
    ".#......................#.",
    "#........................#",
    "#........................#",
    "#........................#",
    "#........................#",
    "#........................#",
    "#.......##########.......#",
    "#......#..........#......#",
    "#.....#............#.....#",
    ".#...#..............#...#.",
    "..###................###..",
};

// k = which d-pad arm and face button are pressed this frame
static void gamepad(int16_t y0, uint8_t k) {
    static const int8_t dirs[4][2] = {{0, -2}, {2, 0}, {0, 2}, {-2, 0}};
    const int16_t x0 = 3;
    gfx_sprite(gamepad_rows, sizeof(gamepad_rows) / sizeof(gamepad_rows[0]), x0, y0);

    // d-pad: filled plus, the pressed arm pulls in
    int16_t dx = x0 + 6, dy = y0 + 5;
    gfx_frect(dx - 2, dy, 5, 1, true);
    gfx_frect(dx, dy - 2, 1, 5, true);
    gfx_px(dx + dirs[k][0], dy + dirs[k][1], false);

    // face buttons: diamond of dots, the pressed one flashes as a small plus
    int16_t bx = x0 + 20, by = y0 + 5;
    for (uint8_t i = 0; i < 4; i++) {
        int16_t x = bx + dirs[i][0], y = by + dirs[i][1];
        gfx_px(x, y, true);
        if (i == (k + 1) % 4) {
            gfx_px(x - 1, y, true);
            gfx_px(x + 1, y, true);
            gfx_px(x, y - 1, true);
            gfx_px(x, y + 1, true);
        }
    }
    gfx_frect(x0 + 10, y0 + 5, 2, 1, true); // select
    gfx_frect(x0 + 14, y0 + 5, 2, 1, true); // start
}

static void draw_icon(uint8_t layer, uint16_t t) {
    gfx_clip(14, 60);
    int16_t o = t % 16;
    switch (layer) {
        case _QWERTY: { // pulsing diamond
            int16_t r = (t % 12) - 6;
            if (r < 0) r = -r;
            for (int16_t dx = -r; dx <= r; dx++) {
                int16_t dy = r - (dx < 0 ? -dx : dx);
                gfx_px(16 + dx, 37 + dy, true);
                gfx_px(16 + dx, 37 - dy, true);
            }
            break;
        }
        case _RAISE:
            for (int16_t k = -1; k < 4; k++) varrow(16, 14 + k * 16 - o, true);
            break;
        case _LOWER:
            for (int16_t k = -1; k < 4; k++) varrow(16, 14 + k * 16 + o - 16, false);
            break;
        case _ADJUST:
            for (int16_t k = -1; k < 4; k++) {
                varrow(9, 14 + k * 16 - o, true);
                varrow(22, 14 + k * 16 + o - 16, false);
            }
            break;
        case _HARDER:
            warning(t % 4 < 2);
            break;
        case _MIRROR:
        case _LMIRROR: {
            int16_t s  = 3 - tri6(t);
            int16_t cy = layer == _MIRROR ? 37 : 31;
            for (int16_t y = 14; y < 60; y += 4) gfx_frect(15, y, 2, 2, true);
            harrow(2 + s, cy, -1);
            harrow(29 - s, cy, 1);
            if (layer == _LMIRROR) varrow(16, 41 + (t % 4), false);
            break;
        }
        case _GAMING1:
        case _LGAMNG1:
            gamepad(layer == _GAMING1 ? 30 : 26, t % 4);
            if (layer == _LGAMNG1) varrow(16, 45 + (t % 4), false);
            break;
    }
    gfx_noclip();
}

// ---- entry point, called from oled_task_user() on the master ----

void oled_master_render(void) {
    static uint32_t timer      = 0;
    static uint16_t t          = 0;
    static uint8_t  last_layer = NONE;
    static uint8_t  last_wpm   = NONE;

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    uint8_t wpm   = get_current_wpm();

    // Frame duration: 260 ms idle, faster as you type, 35 ms at most.
    uint16_t speedup  = (uint16_t)wpm * 17 / 10;
    uint16_t interval = wpm == 0 ? 260 : (speedup >= 205 ? 35 : 240 - speedup);

    bool redraw = false;
    if (layer != last_layer) {
        last_layer = layer;
        t          = 0;
        timer      = timer_read32();
        redraw     = true;
    } else if (timer_elapsed32(timer) >= interval) {
        timer  = timer_read32();
        t++;
        redraw = true;
    }
    if (wpm != last_wpm) {
        last_wpm = wpm;
        redraw   = true;
    }
    if (!redraw) {
        return;
    }

    oled_clear();
    draw_hud(layer);
    draw_icon(layer, t);
    draw_stack(layer, t);
    draw_gauge(wpm);
}
