#include "platform/android/android.h"

#include "platform/android/jni.h"

#include "SDL.h"

JNIEnv *jni_get_env(void)
{
    return SDL_AndroidGetJNIEnv();
}

jobject jni_get_activity(void)
{
    return SDL_AndroidGetActivity();
}

void platform_show_virtual_keyboard(void)
{
    // SDL starts with text input active but its keyboard hidden, so the keyboard's own state decides
    if (!SDL_IsScreenKeyboardShown(SDL_GetKeyboardFocus())) {
        SDL_StartTextInput();
    }
}

void platform_hide_virtual_keyboard(void)
{
    if (SDL_IsTextInputActive()) {
        SDL_StopTextInput();
    }
}
