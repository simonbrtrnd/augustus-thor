#include "bottom_screen.h"

#ifdef __ANDROID__

#include "graphics/renderer.h"
#include "graphics/second_screen.h"
#include "input/mouse.h"

#include <android/input.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PENDING_TOUCHES 32
#define WINDOW_BLACK 0xff000000
#define DRAG_SCROLL_STEP 100

typedef enum {
    SHOWING_NOTHING,
    SHOWING_BLACK,
    SHOWING_PIXELS
} window_content;

typedef struct {
    int action;
    float x;
    float y;
} touch_event;

static struct {
    pthread_mutex_t lock;
    ANativeWindow *window;
    int width;
    int height;
    int has_new_window;
    touch_event pending[MAX_PENDING_TOUCHES];
    int pending_count;
} shared = { .lock = PTHREAD_MUTEX_INITIALIZER };

static struct {
    int width;
    int height;
    uint32_t *pixels;
    uint32_t *shown_pixels;
    window_content shown;
} frame;

static struct {
    float anchor_y;
    int has_scrolled;
} drag;

static void free_pixels(void)
{
    free(frame.pixels);
    free(frame.shown_pixels);
    frame.pixels = 0;
    frame.shown_pixels = 0;
    frame.shown = SHOWING_NOTHING;
}

static size_t get_frame_bytes(void)
{
    return (size_t) frame.width * frame.height * sizeof(uint32_t);
}

static int allocate_pixels(void)
{
    if (frame.pixels) {
        return 1;
    }
    frame.pixels = malloc(get_frame_bytes());
    frame.shown_pixels = malloc(get_frame_bytes());
    if (!frame.pixels || !frame.shown_pixels) {
        free_pixels();
        return 0;
    }
    return 1;
}

static int read_pixels(void)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    return second_screen_has_content() && renderer->read_second_screen_pixels && allocate_pixels() &&
        renderer->read_second_screen_pixels(frame.pixels, frame.width, frame.height);
}

static int is_shown(int has_pixels)
{
    if (!has_pixels) {
        return frame.shown == SHOWING_BLACK;
    }
    return frame.shown == SHOWING_PIXELS &&
        memcmp(frame.pixels, frame.shown_pixels, get_frame_bytes()) == 0;
}

static void copy_pixels(const ANativeWindow_Buffer *buffer)
{
    uint32_t *rows = buffer->bits;
    for (int y = 0; y < frame.height; y++) {
        memcpy(&rows[y * buffer->stride], &frame.pixels[y * frame.width], frame.width * sizeof(uint32_t));
    }
    uint32_t *shown_pixels = frame.shown_pixels;
    frame.shown_pixels = frame.pixels;
    frame.pixels = shown_pixels;
}

static void fill_black(const ANativeWindow_Buffer *buffer)
{
    uint32_t *rows = buffer->bits;
    for (int i = 0; i < buffer->stride * buffer->height; i++) {
        rows[i] = WINDOW_BLACK;
    }
}

static void show(ANativeWindow *window, int has_pixels)
{
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, 0) != 0) {
        return;
    }
    if (buffer.width != frame.width || buffer.height != frame.height) {
        fill_black(&buffer);
        frame.shown = SHOWING_NOTHING;
    } else if (has_pixels) {
        copy_pixels(&buffer);
        frame.shown = SHOWING_PIXELS;
    } else {
        fill_black(&buffer);
        frame.shown = SHOWING_BLACK;
    }
    ANativeWindow_unlockAndPost(window);
}

static ANativeWindow *acquire_window(int *is_new)
{
    pthread_mutex_lock(&shared.lock);
    ANativeWindow *window = shared.window;
    if (window) {
        ANativeWindow_acquire(window);
    }
    *is_new = shared.has_new_window;
    shared.has_new_window = 0;
    pthread_mutex_unlock(&shared.lock);
    return window;
}

void android_bottom_screen_present(void)
{
    if (!frame.width || !frame.height) {
        return;
    }
    int has_pixels = read_pixels();
    int is_new_window;
    ANativeWindow *window = acquire_window(&is_new_window);
    if (!window) {
        return;
    }
    if (is_new_window || !is_shown(has_pixels)) {
        show(window, has_pixels);
    }
    ANativeWindow_release(window);
}

static mouse to_screen_mouse(const touch_event *event)
{
    mouse m = { 0 };
    m.x = (int) (event->x * second_screen_width());
    m.y = (int) (event->y * second_screen_height());
    m.is_inside_window = 1;
    m.window_has_focus = 1;
    switch (event->action) {
        case AMOTION_EVENT_ACTION_DOWN:
            m.left.went_down = 1;
            m.left.is_down = 1;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            m.left.is_down = 1;
            break;
        case AMOTION_EVENT_ACTION_UP:
            m.left.went_up = 1;
            break;
        default:
            break;
    }
    return m;
}

static void scroll_with_drag(const touch_event *event, const mouse *m)
{
    float y = event->y * frame.height;
    while (fabsf(y - drag.anchor_y) >= DRAG_SCROLL_STEP) {
        int direction = y < drag.anchor_y ? -1 : 1;
        mouse scroll = *m;
        scroll.scrolled = direction < 0 ? SCROLL_DOWN : SCROLL_UP;
        second_screen_handle_input(&scroll);
        drag.anchor_y += direction * DRAG_SCROLL_STEP;
        drag.has_scrolled = 1;
    }
}

static void handle_touch(const touch_event *event)
{
    mouse m = to_screen_mouse(event);
    switch (event->action) {
        case AMOTION_EVENT_ACTION_DOWN:
            drag.anchor_y = event->y * frame.height;
            drag.has_scrolled = 0;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            scroll_with_drag(event, &m);
            break;
        case AMOTION_EVENT_ACTION_UP:
            if (drag.has_scrolled) {
                m.left.went_up = 0;
            }
            break;
        default:
            break;
    }
    second_screen_handle_input(&m);
}

void android_bottom_screen_handle_input(void)
{
    touch_event events[MAX_PENDING_TOUCHES];

    pthread_mutex_lock(&shared.lock);
    int count = shared.pending_count;
    memcpy(events, shared.pending, count * sizeof(touch_event));
    shared.pending_count = 0;
    int width = shared.window ? shared.width : 0;
    int height = shared.window ? shared.height : 0;
    pthread_mutex_unlock(&shared.lock);

    if (width != frame.width || height != frame.height) {
        free_pixels();
        frame.width = width;
        frame.height = height;
    }
    second_screen_set_size(width, height);
    for (int i = 0; i < count; i++) {
        handle_touch(&events[i]);
    }
}

JNIEXPORT void JNICALL Java_com_github_Keriew_augustus_BottomScreen_nativeSetSurface(
    JNIEnv *env, jclass cls, jobject surface, jint surface_width, jint surface_height)
{
    ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : 0;
    if (window) {
        ANativeWindow_setBuffersGeometry(window, surface_width, surface_height, WINDOW_FORMAT_RGBA_8888);
    }

    pthread_mutex_lock(&shared.lock);
    if (shared.window) {
        ANativeWindow_release(shared.window);
    }
    shared.window = window;
    shared.width = surface_width;
    shared.height = surface_height;
    shared.has_new_window = 1;
    shared.pending_count = 0;
    pthread_mutex_unlock(&shared.lock);
}

JNIEXPORT void JNICALL Java_com_github_Keriew_augustus_BottomScreen_nativeTouch(
    JNIEnv *env, jclass cls, jint action, jfloat x, jfloat y)
{
    pthread_mutex_lock(&shared.lock);
    touch_event *last = shared.pending_count ? &shared.pending[shared.pending_count - 1] : 0;
    if (last && last->action == AMOTION_EVENT_ACTION_MOVE && action == AMOTION_EVENT_ACTION_MOVE) {
        last->x = x;
        last->y = y;
    } else if (shared.pending_count < MAX_PENDING_TOUCHES) {
        touch_event *event = &shared.pending[shared.pending_count++];
        event->action = action;
        event->x = x;
        event->y = y;
    }
    pthread_mutex_unlock(&shared.lock);
}

#endif // __ANDROID__
