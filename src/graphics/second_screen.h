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
 * Scale of the buttons of a dialog, larger on the second screen so they are easier to touch.
 * Returns 0, which means unscaled, when the second screen is not current.
 */
float second_screen_button_scale(void);

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
 * Runs the draw function of a dialog on the second screen when dialogs go there, or on the main screen.
 * The first call of a redraw clears the second screen at the scale where a full 640x480 dialog fits.
 * Later calls, and those of a window opened over a dialog of the second screen, draw over it.
 */
void second_screen_draw_dialog(void (*draw)(void));

/**
 * Same as second_screen_draw_background, for a dialog that fills the second screen with whole blocks.
 * The scale is 2.5, or smaller when the dialog needs more than the screen height in game pixels.
 * The dialog sizes itself from screen_width() and screen_height(), over a marble background
 * that shows only where whole blocks do not meet the screen edges.
 * Runs the draw function on the main screen instead when dialogs do not go on the second screen,
 * or when the second screen cannot be drawn on.
 */
void second_screen_draw_full_screen_dialog(int min_height, void (*draw)(void));

/**
 * Whether the second screen shows the dialog of the current window, or the dialog it was opened over.
 * Input on the main screen then can only close the window.
 */
int second_screen_has_dialog(void);

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
