// Shared by both portrait screens: layer names, header, frame timing.
#pragma once

#include <stdint.h>

const char *oled_layer_name(uint8_t layer);

// Layer name at the top, dotted rule under it.
void oled_draw_hud(uint8_t layer);

// Frame duration: 260 ms idle, faster as you type, 35 ms at most.
uint16_t oled_frame_interval(uint8_t wpm);
