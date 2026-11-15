#pragma once

// Draws the left (slave) screen: the boot logo, then Lily. Call it from
// oled_task_user() on the slave, with the display in portrait (32x128).
void oled_slave_render(void);
