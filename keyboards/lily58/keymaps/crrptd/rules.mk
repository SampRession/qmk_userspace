LTO_ENABLE = yes                     # Link Time Optimization enabled
BOOTMAGIC_ENABLE = no                # Enable Bootmagic Lite
MOUSEKEY_ENABLE = yes                # Mouse keys
EXTRAKEY_ENABLE = yes                # Audio control and System control
LEADER_ENABLE = yes                  # Enable Leader key
DYNAMIC_TAPPING_TERM_ENABLE = yes    # Enable dynamic change of tap-hold delay
TRI_LAYER_ENABLE = yes				 # Enable Tri-Layer mode
CONSOLE_ENABLE = no                  # Console for debug
COMMAND_ENABLE = no                  # Commands for debug and configuration
# SEND_STRING_ENABLE = no             # Enable the "send string" feature
CAPS_WORD_ENABLE = yes               # Enable a modern Caps Lock alternative
NKRO_ENABLE = yes                    # N-Key Rollover
BACKLIGHT_ENABLE = no                # Enable keyboard backlight functionality
AUDIO_ENABLE = no                    # Audio output
RGBLIGHT_ENABLE = no                 # Enable WS2812 RGB underlight.
SWAP_HANDS_ENABLE = no               # Enable one-hand typing
OLED_ENABLE = yes                    # OLED display
WPM_ENABLE = yes                     # Words per minute, drives the OLED animations

# Portrait OLED screens
SRC += oled_gfx.c \
       oled_hud.c \
       oled_master.c \
       oled_slave.c

# Custom rules
CONVERT_TO = rp2040_ce
