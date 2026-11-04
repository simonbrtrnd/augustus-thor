#ifndef GRAPHICS_SECOND_SCREEN_H
#define GRAPHICS_SECOND_SCREEN_H

/**
 * A second physical screen, such as the touch screen of the AYN Thor. While it is active,
 * it shows a panel with the sidebar controls and the main screen shows the city alone.
 * The second screen has its own size and coordinates, in game pixels.
 */

int second_screen_is_active(void);

int second_screen_width(void);

int second_screen_height(void);

/**
 * Called by the platform with the size of the second screen in game pixels, or 0 when it is gone,
 * and the number of screen pixels per game pixel.
 * The second screen is drawn at screen resolution, so images can be drawn at fractional game scales.
 */
void second_screen_set_size(int width, int height, int pixel_scale);

/**
 * Runs the draw function on the second screen.
 * Returns 0 without drawing when the second screen is not active.
 */
int second_screen_draw(void (*draw)(void));

/**
 * Same as second_screen_draw, but clears the second screen first.
 * Use it for the drawing that gives the second screen its content.
 */
int second_screen_draw_background(void (*draw)(void));

/**
 * Marks the second screen as empty until a window draws on it again.
 */
void second_screen_reset_content(void);

int second_screen_has_content(void);

#endif // GRAPHICS_SECOND_SCREEN_H
