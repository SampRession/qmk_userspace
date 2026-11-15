// Left (slave) OLED, portrait: Lily, a little lily flower who reacts to the
// layer and to the typing speed, the WPM and its history; the Lily58 logo at
// boot. Needs SPLIT_LAYER_STATE_ENABLE and SPLIT_WPM_ENABLE.
// Mockup and reference: tools/oled-mockup (variant B, style "Fleur").
#include QMK_KEYBOARD_H

#include <string.h>

#include "layers.h"
#include "lily58_logo.h"
#include "lily_sprites.h"
#include "oled_gfx.h"
#include "oled_hud.h"
#include "oled_slave.h"

#define BOOT_LOGO_MS 2500
#define HIST_LEN 28
#define HIST_SAMPLE_MS 500
#define SLEEP_FRAMES 20 // frames at 0 WPM before Lily falls asleep (~5 s)

typedef enum { ARM_DOWN, ARM_UP, ARM_OUT } arm_t;
typedef enum { EYES_OPEN, EYES_CLOSED, EYES_HAPPY, EYES_ANGRY, EYES_FOCUS } eyes_t;
typedef enum { MOUTH_SMALL, MOUTH_SMILE, MOUTH_OPEN } mouth_t;

enum {
    FX_ZZZ     = 1 << 0,
    FX_SPARKLE = 1 << 1,
    FX_BANG    = 1 << 2,
    FX_MIRROR  = 1 << 3,
    FX_SWEAT   = 1 << 4,
};

typedef struct {
    int8_t  squash, stretch, lift; // body height and hop
    int8_t  bx, hx;                // shake of the whole body, sway of the head
    arm_t   arms[2];
    eyes_t  eyes;
    int8_t  look_x, look_y;
    mouth_t mouth;
    uint8_t head; // lily_head pose
    bool    pad;  // holds a gamepad instead of waving her leaves
    uint8_t fx;
} pose_t;

static pose_t pose_for(uint8_t layer, uint16_t t, uint8_t wpm, bool sleeping) {
    pose_t p = {.arms = {ARM_DOWN, ARM_DOWN}, .eyes = EYES_OPEN, .mouth = MOUTH_SMALL, .head = 2};
    if (sleeping) {
        p.eyes = EYES_CLOSED;
        p.head = 5;
        p.fx   = FX_ZZZ;
        return p;
    }

    bool mirror = layer == _MIRROR || layer == _LMIRROR;
    bool gaming = layer == _GAMING1 || layer == _LGAMNG1;
    bool low    = layer == _LOWER || layer == _LMIRROR || layer == _LGAMNG1;
    bool spin   = false;

    if (low) { // crouch
        p.squash = 4;
        p.look_y = 1;
        p.arms[0] = p.arms[1] = ARM_OUT;
    }
    switch (layer) {
        case _QWERTY: { // sways, blinks now and then
            static const int8_t sway[8] = {0, 0, 1, 1, 0, 0, -1, -1};
            p.hx = sway[t % 8];
            if (t % 24 < 2) p.eyes = EYES_CLOSED;
            p.mouth = MOUTH_SMILE;
            break;
        }
        case _LOWER:
            p.squash = 4 + (t % 4 < 2 ? 0 : 1);
            break;
        case _RAISE: { // jumps, arms up
            static const int8_t hop[4] = {0, 2, 4, 2};
            p.arms[0] = p.arms[1] = ARM_UP;
            p.stretch = 2;
            p.lift    = hop[t % 4];
            p.mouth   = MOUTH_OPEN;
            p.look_y  = -1;
            break;
        }
        case _ADJUST: // happy, petals twirl
            p.eyes    = EYES_HAPPY;
            p.mouth   = MOUTH_SMILE;
            p.arms[0] = p.arms[1] = ARM_OUT;
            spin      = t > 0;
            p.fx |= FX_SPARKLE;
            break;
        case _HARDER: // angry
            p.eyes    = EYES_ANGRY;
            p.mouth   = MOUTH_OPEN;
            p.arms[0] = p.arms[1] = ARM_UP;
            p.bx      = t % 2 ? 1 : -1;
            p.fx |= FX_BANG;
            break;
    }
    if (mirror) { // waves one side, then the other
        bool w = t % 8 < 4;
        if (low) {
            p.arms[0] = w ? ARM_UP : ARM_OUT;
            p.arms[1] = w ? ARM_OUT : ARM_UP;
        } else {
            p.arms[0] = w ? ARM_UP : ARM_DOWN;
            p.arms[1] = w ? ARM_DOWN : ARM_UP;
        }
        p.look_x = w ? -1 : 1;
        p.look_y = low ? 1 : 0;
        p.fx |= FX_MIRROR;
    }
    if (gaming) { // plays
        p.pad    = true;
        p.look_x = 0;
        p.look_y = 1;
        p.eyes   = EYES_FOCUS;
        p.mouth  = MOUTH_SMALL;
        if (!low) p.lift = t % 4 == 0 ? 1 : 0;
    }
    if (wpm > 80 && layer != _HARDER) {
        p.mouth = MOUTH_OPEN;
        p.fx |= FX_SWEAT;
    }

    static const uint8_t twirl[4] = {0, 2, 4, 2};
    p.head = spin ? twirl[t % 4] : (uint8_t)(2 + p.hx);
    return p;
}

// ---- Lily's parts ----

static int16_t leaf_short(int16_t v) { // 70% of v, rounded away from zero
    int16_t a = v < 0 ? -v : v;
    a         = (a * 7 + 5) / 10;
    return v < 0 ? -a : a;
}

static void leaf(int16_t x0, int16_t y0, int16_t dx, int16_t dy) {
    gfx_line(x0, y0, x0 + dx, y0 + dy, true);
    gfx_line(x0, y0 + 1, x0 + leaf_short(dx), y0 + 1 + leaf_short(dy), true);
}

// A tiny gamepad held in front of the stem; a button blinks as it is pressed.
static void held_pad(int16_t cx, int16_t y, uint16_t t) {
    gfx_frect(cx - 7, y - 1, 14, 7, false);
    gfx_frect(cx - 5, y, 10, 1, true);
    gfx_frect(cx - 6, y + 1, 12, 3, true);
    gfx_frect(cx - 6, y + 4, 3, 1, true);
    gfx_frect(cx + 3, y + 4, 3, 1, true);
    gfx_frect(cx - 5, y + 2, 3, 1, false); // d-pad
    gfx_px(cx - 4, y + 1, false);
    gfx_px(cx - 4, y + 3, false);
    gfx_px(cx + 2, y + 2, t % 2); // buttons
    gfx_px(cx + 3, y + 1, false);
    gfx_px(cx + 4, y + 2, false);
    gfx_px(cx + 3, y + 3, !(t % 2));
    gfx_px(cx - 7, y + 2, true); // leaf-hands
    gfx_px(cx + 6, y + 2, true);
}

static void draw_body(int16_t cx, int16_t from, int16_t gy, const pose_t *p, int16_t stem, uint16_t t) {
    static const int8_t arm_end[3][2] = {{4, 3}, {6, -4}, {5, -1}}; // down, up, out
    gfx_frect(cx - 1, from, 2, gy - from, true);                    // stem
    gfx_frect(cx - 3, gy, 2, 1, true);                              // feet
    gfx_frect(cx + 1, gy, 2, 1, true);
    int16_t ay = from + 1 + stem / 3;
    if (p->pad) {
        held_pad(cx, ay - 1, t);
        return;
    }
    leaf(cx - 2, ay, -arm_end[p->arms[0]][0], arm_end[p->arms[0]][1]);
    leaf(cx + 1, ay, arm_end[p->arms[1]][0], arm_end[p->arms[1]][1]);
}

// Black features on the white face, centred between columns hcx - 1 and hcx.
static void draw_face(int16_t hcx, int16_t fy, const pose_t *p) {
    int16_t l = hcx - 4 + p->look_x, r = hcx + 2 + p->look_x, ey = fy - 2 + p->look_y;
    switch (p->eyes) {
        case EYES_OPEN:
            gfx_frect(l, ey, 2, 2, false);
            gfx_frect(r, ey, 2, 2, false);
            break;
        case EYES_CLOSED:
            gfx_frect(l - 1, ey + 1, 3, 1, false);
            gfx_frect(r, ey + 1, 3, 1, false);
            break;
        case EYES_HAPPY:
            gfx_px(l - 1, ey + 2, false);
            gfx_px(l, ey + 1, false);
            gfx_px(l + 1, ey + 2, false);
            gfx_px(r, ey + 2, false);
            gfx_px(r + 1, ey + 1, false);
            gfx_px(r + 2, ey + 2, false);
            break;
        case EYES_ANGRY:
            gfx_px(l - 2, ey - 2, false);
            gfx_px(l - 1, ey - 1, false);
            gfx_px(l, ey, false);
            gfx_px(r + 3, ey - 2, false);
            gfx_px(r + 2, ey - 1, false);
            gfx_px(r + 1, ey, false);
            gfx_frect(l, ey + 1, 2, 2, false);
            gfx_frect(r, ey + 1, 2, 2, false);
            break;
        case EYES_FOCUS:
            gfx_frect(l - 1, ey - 1, 3, 1, false);
            gfx_frect(r, ey - 1, 3, 1, false);
            gfx_frect(l, ey, 2, 2, false);
            gfx_frect(r, ey, 2, 2, false);
            break;
    }
    int16_t my = fy + 2;
    switch (p->mouth) {
        case MOUTH_SMALL:
            gfx_frect(hcx - 1, my, 2, 1, false);
            break;
        case MOUTH_SMILE:
            gfx_px(hcx - 2, my - 1, false);
            gfx_frect(hcx - 1, my, 2, 1, false);
            gfx_px(hcx + 1, my - 1, false);
            break;
        case MOUTH_OPEN:
            gfx_frect(hcx - 1, my, 2, 2, false);
            break;
    }
}

static void plus(int16_t x, int16_t y) {
    gfx_px(x, y, true);
    gfx_px(x - 1, y, true);
    gfx_px(x + 1, y, true);
    gfx_px(x, y - 1, true);
    gfx_px(x, y + 1, true);
}

static void draw_lily(uint8_t layer, uint16_t t, uint8_t wpm, bool sleeping, int16_t cx, int16_t ground, int16_t top) {
    pose_t p = pose_for(layer, t, wpm, sleeping);
    if (p.fx & FX_MIRROR) {
        for (int16_t y = top; y < ground; y += 4) gfx_frect(15, y, 2, 2, true);
    }

    const int16_t R    = 6;
    int16_t       stem = 6 - p.squash + p.stretch;
    if (stem < 2) stem = 2;
    int16_t gy = ground - p.lift, bx = cx + p.bx, hcx = bx + p.hx;
    int16_t hcy = gy - stem - R - 1;

    gfx_sprite(lily_head[p.head], LILY_HEAD_H, hcx + LILY_HEAD_DX, hcy + LILY_HEAD_DY);
    draw_body(bx, hcy + R + 1, gy, &p, stem, t);
    draw_face(hcx, hcy + 1, &p);

    int16_t head_top = hcy - 16;
    if ((p.fx & FX_BANG) && t % 2 == 0) {
        gfx_frect(hcx - 1, head_top - 9, 2, 6, true);
        gfx_frect(hcx - 1, head_top - 2, 2, 2, true);
    }
    if (p.fx & FX_SPARKLE) {
        static const int8_t pts[4][2] = {{-11, -6}, {10, -2}, {-10, 4}, {11, 7}};
        for (uint8_t i = 0; i < 4; i++) {
            if ((t + i) % 3 == 0) plus(hcx + pts[i][0], hcy + pts[i][1]);
        }
    }
    if (p.fx & FX_SWEAT) {
        int16_t y = hcy - 3 + t % 4;
        gfx_px(hcx + 8, y, true);
        gfx_px(hcx + 8, y + 1, true);
        gfx_px(hcx + 9, y + 1, true);
    }
    if (p.fx & FX_ZZZ) {
        int16_t k = t % 12;
        gfx_text("Z", hcx + 5, hcy - 12 - (k >> 1));
        if (k > 5) gfx_text("Z", hcx + 9, hcy - 18 - ((k - 6) >> 1));
    }
}

// ---- screen ----

static void draw_wpm(uint8_t wpm, const uint8_t *hist) {
    char buf[8] = "WPM ";
    char digits[4];
    uint8_t n = 0;
    do {
        digits[n++] = '0' + wpm % 10;
        wpm /= 10;
    } while (wpm > 0);
    for (uint8_t i = 0; i < n; i++) buf[4 + i] = digits[n - 1 - i];
    buf[4 + n] = '\0';
    gfx_text(buf, -1, 89);

    for (uint8_t i = 0; i < HIST_LEN; i++) {
        uint8_t v   = hist[i] > 150 ? 150 : hist[i];
        int16_t hgt = (26 * v + 75) / 150;
        if (hgt < 1) hgt = 1;
        gfx_frect(2 + i, 125 - hgt, 1, hgt, true);
    }
    for (int16_t x = 2; x < 30; x += 2) gfx_px(x, 126, true);
}

static void draw_screen(uint8_t layer, uint16_t t, uint8_t wpm, bool sleeping, const uint8_t *hist) {
    oled_draw_hud(layer);
    gfx_clip(14, 86);
    draw_lily(layer, t, wpm, sleeping, 16, 80, 14);
    for (int16_t x = 1; x < 31; x += 2) gfx_px(x, 82, true);
    gfx_noclip();
    draw_wpm(wpm, hist);
}

// The logo, turned a quarter so it reads along the portrait screen.
static void draw_logo(void) {
    for (int16_t lx = 0; lx < 126; lx++) {
        for (int16_t ly = 0; ly < 24; ly++) {
            if ((pgm_read_byte(&lily58_logo[(ly >> 3) * 128 + lx]) >> (ly & 7)) & 1) {
                gfx_px(4 + ly, 126 - lx, true);
            }
        }
    }
}

// ---- entry point, called from oled_task_user() on the slave ----

void oled_slave_render(void) {
    static bool     logo_done   = false;
    static uint32_t frame_timer = 0;
    static uint32_t hist_timer  = 0;
    static uint16_t t           = 0;
    static uint16_t idle        = 0;
    static uint8_t  last_layer  = 0xFF;
    static uint8_t  last_wpm    = 0xFF;
    static uint8_t  hist[HIST_LEN];

    if (timer_read32() < BOOT_LOGO_MS) {
        if (!logo_done) {
            gfx_begin();
            draw_logo();
            gfx_end();
            logo_done = true;
        }
        return;
    }

    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    uint8_t wpm   = get_current_wpm();

    if (timer_elapsed32(hist_timer) >= HIST_SAMPLE_MS) {
        hist_timer = timer_read32();
        memmove(hist, hist + 1, HIST_LEN - 1);
        hist[HIST_LEN - 1] = wpm;
    }

    bool redraw = false;
    if (layer != last_layer) {
        last_layer  = layer;
        t           = 0;
        frame_timer = timer_read32();
        redraw      = true;
    } else if (timer_elapsed32(frame_timer) >= oled_frame_interval(wpm)) {
        frame_timer = timer_read32();
        t++;
        idle   = wpm == 0 ? idle + 1 : 0;
        redraw = true;
    }
    if (wpm != last_wpm) {
        last_wpm = wpm;
        redraw   = true;
    }
    if (!redraw) {
        return;
    }

    gfx_begin();
    draw_screen(layer, t, wpm, idle > SLEEP_FRAMES, hist);
    gfx_end();
}
