#ifndef PLATFORM_ANDROID_BOTTOM_SCREEN_H
#define PLATFORM_ANDROID_BOTTOM_SCREEN_H

#ifdef __ANDROID__

/**
 * Applies the touches received on the second screen since the last frame as mouse input.
 */
void android_bottom_screen_handle_input(void);

/**
 * Copies the sidebar from the frame being drawn to the second screen.
 * Must be called while the frame's render target is still active.
 */
void android_bottom_screen_present(void);

#endif // __ANDROID__
#endif // PLATFORM_ANDROID_BOTTOM_SCREEN_H
