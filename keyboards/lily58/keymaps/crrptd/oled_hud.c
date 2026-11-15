#include "oled_hud.h"

#include "layers.h"
#include "oled_gfx.h"

const char *oled_layer_name(uint8_t layer) {
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

void oled_draw_hud(uint8_t layer) {
    gfx_text(oled_layer_name(layer), -1, 3);
    for (int16_t x = 2; x < 30; x += 2) {
        gfx_px(x, 11, true);
    }
}

uint16_t oled_frame_interval(uint8_t wpm) {
    uint16_t speedup = (uint16_t)wpm * 17 / 10;
    return wpm == 0 ? 260 : (speedup >= 205 ? 35 : 240 - speedup);
}
