#ifndef PLATFORM_ANDROID_BOTTOM_SCREEN_H
#define PLATFORM_ANDROID_BOTTOM_SCREEN_H

#ifdef __ANDROID__

/**
 * Turns the second screen on or off for the game, and passes the touches received on it
 * since the last frame to the current window.
 */
void android_bottom_screen_handle_input(void);

/**
 * Shows what the game drew for the second screen.
 */
void android_bottom_screen_present(void);

#endif // __ANDROID__
#endif // PLATFORM_ANDROID_BOTTOM_SCREEN_H
