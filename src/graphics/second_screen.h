#ifndef GRAPHICS_SECOND_SCREEN_H
#define GRAPHICS_SECOND_SCREEN_H

/**
 * A second physical screen, such as the touch screen of the AYN Thor. While it is active,
 * the sidebar and the build menu are drawn for it, and the main screen shows the city alone.
 */

int second_screen_is_active(void);

/**
 * Called by the platform when the second screen appears or disappears.
 */
void second_screen_set_active(int active);

/**
 * Runs the draw function on the second screen when it is active, on the main screen otherwise.
 */
void second_screen_draw(void (*draw)(void));

/**
 * Same as second_screen_draw, but clears the second screen first.
 * Use it for the drawing that gives the second screen its content.
 */
void second_screen_draw_background(void (*draw)(void));

/**
 * Marks the second screen as empty until a window draws on it again.
 */
void second_screen_reset_content(void);

int second_screen_has_content(void);

#endif // GRAPHICS_SECOND_SCREEN_H
