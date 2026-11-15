#pragma once

// Draws the right (master) screen. Call it from oled_task_user() on the master,
// with the display in OLED_ROTATION_270 (portrait, 32x128).
void oled_master_render(void);
