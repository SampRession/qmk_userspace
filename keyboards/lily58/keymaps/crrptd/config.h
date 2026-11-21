#pragma once


#define MASTER_RIGHT

// OLED
#define OLED_FONT_H "lib/glcdfont_lily.c"

// The left screen (Lily) needs the layer and the WPM from the master, and its
// activity so it sleeps and wakes with the right half.
#define SPLIT_LAYER_STATE_ENABLE
#define SPLIT_WPM_ENABLE
#define SPLIT_ACTIVITY_ENABLE
// The slave screen follows the master's on/off state: it turns off when the
// host suspends and on the master's timeout, instead of only on its own keys.
#define SPLIT_OLED_ENABLE

// Caps Word
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

// Tri layers
#define TRI_LAYER_LOWER_LAYER 1
#define TRI_LAYER_UPPER_LAYER 2
#define TRI_LAYER_ADJUST_LAYER 3

// Tap-Hold
#define TAPPING_TERM 200
#define QUICK_TAP_TERM 0
#define PERMISSIVE_HOLD

// Leader key
#define LEADER_TIMEOUT 250
#define LEADER_PER_KEY_TIMING

// Compile key
#define ENABLE_COMPILE_KEYCODE
