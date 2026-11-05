#ifndef GRAPHICS_SECOND_SCREEN_H
#define GRAPHICS_SECOND_SCREEN_H

#include "input/mouse.h"

/**
 * A second physical screen, such as the touch screen of the AYN Thor. While it is active,
 * it shows a panel with the sidebar controls and the main screen shows the city alone.
 * Dialogs such as the city messages also move to the second screen.
 * The second screen has its own size and coordinates, in game pixels.
 */

int second_screen_is_active(void);

int second_screen_width(void);

int second_screen_height(void);

/**
 * The number of screen pixels per game pixel of the content shown on the second screen.
 */
int second_screen_pixel_scale(void);

/**
 * Whether the game is drawing on the second screen or handling its input.
 * While it is, the screen size and the dialog offset are those of the second screen.
 */
int second_screen_is_current(void);

/**
 * Called by the platform with the size of the second screen in screen pixels, or 0 when it is gone.
 * The second screen is drawn at screen resolution, so images can be drawn at fractional game scales.
 */
void second_screen_set_size(int width, int height);

/**
 * Runs the draw function on the second screen, at the scale of its current content.
 * Returns 0 without drawing when the second screen is not active.
 */
int second_screen_draw(void (*draw)(void));

/**
 * Same as second_screen_draw, but clears the second screen first and sets the panel scale.
 * Use it for the drawing that gives the second screen its content.
 */
int second_screen_draw_background(void (*draw)(void));

/**
 * Same as second_screen_draw_background, at a scale where a 640x480 dialog fits,
 * over a marble background.
 */
int second_screen_draw_dialog_background(void (*draw)(void));

void second_screen_draw_marble_background(void);

/**
 * Called by the platform for each touch on the second screen, in its game pixels.
 */
void second_screen_handle_input(const mouse *m);

/**
 * Marks the second screen as empty until a window draws on it again.
 */
void second_screen_reset_content(void);

int second_screen_has_content(void);

#endif // GRAPHICS_SECOND_SCREEN_H
