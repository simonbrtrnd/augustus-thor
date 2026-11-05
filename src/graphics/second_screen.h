#ifndef GRAPHICS_SECOND_SCREEN_H
#define GRAPHICS_SECOND_SCREEN_H

#include "input/mouse.h"

/**
 * A second physical screen, such as the touch screen of the AYN Thor.
 * It has its own size and coordinates, in game pixels.
 */

int second_screen_is_active(void);

int second_screen_width(void);

int second_screen_height(void);

/**
 * Whether the game is drawing on the second screen.
 * While it is, the screen size is that of the second screen.
 */
int second_screen_is_drawing(void);

/**
 * Whether the game is drawing on the second screen or handling a touch on it.
 * While it is, the dialog offset is that of the second screen.
 */
int second_screen_is_current(void);

/**
 * Called by the platform with the size of the second screen in screen pixels, or 0 when it is gone.
 * The second screen is drawn at screen resolution, so images can be drawn at fractional game scales.
 */
void second_screen_set_size(int width, int height);

/**
 * Runs the draw function on the second screen, at the scale of its current content.
 * Returns 0 without drawing when the second screen cannot be drawn on.
 */
int second_screen_draw(void (*draw)(void));

/**
 * Same as second_screen_draw, but clears the second screen first and sets the panel scale.
 * Use it for the drawing that gives the second screen its content.
 */
int second_screen_draw_background(void (*draw)(void));

/**
 * Same as second_screen_draw_background, over a darkened marble background, at the scale where
 * a dialog of the given size in game pixels fills the second screen.
 * Returns 0 without drawing when the player keeps the messages on the main screen.
 */
int second_screen_draw_dialog_background(int width, int height, void (*draw)(void));

void second_screen_draw_marble_background(void);

/**
 * Called by the platform for each touch on the second screen, in its game pixels.
 * The touch goes to the current window, once that window has drawn the second screen.
 */
void second_screen_handle_input(const mouse *m);

/**
 * Marks the second screen as empty until a window draws on it again.
 */
void second_screen_reset_content(void);

int second_screen_has_content(void);

#endif // GRAPHICS_SECOND_SCREEN_H
