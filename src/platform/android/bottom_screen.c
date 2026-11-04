#include "bottom_screen.h"

#ifdef __ANDROID__

#include "graphics/color.h"
#include "graphics/menu.h"
#include "graphics/renderer.h"
#include "input/mouse.h"
#include "widget/sidebar/common.h"

#include <android/native_window_jni.h>
#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PIXEL_SCALE 2
#define MAX_PENDING_TOUCHES 32
#define WINDOW_BLACK 0xff000000

enum {
    MOTION_ACTION_DOWN = 0,
    MOTION_ACTION_UP = 1,
    MOTION_ACTION_MOVE = 2,
    MOTION_ACTION_CANCEL = 3
};

typedef struct {
    int action;
    float x;
    float y;
} touch_event;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} area;

static struct {
    pthread_mutex_t lock;
    ANativeWindow *window;
    int width;
    int height;
    touch_event pending[MAX_PENDING_TOUCHES];
    int pending_count;
    int is_pressing;
    color_t *pixels;
    int pixels_size;
} data = { .lock = PTHREAD_MUTEX_INITIALIZER };

static area get_sidebar_area(void)
{
    area sidebar = {
        sidebar_common_get_x_offset_expanded(), TOP_MENU_HEIGHT,
        SIDEBAR_EXPANDED_WIDTH, sidebar_common_get_height()
    };
    return sidebar;
}

static area get_panel_area(int width, int height)
{
    area sidebar = get_sidebar_area();
    area panel = { (width - sidebar.width) / 2, (height - sidebar.height) / 2, sidebar.width, sidebar.height };
    return panel;
}

static uint32_t to_window_pixel(color_t color)
{
    return (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color >> 16) & 0xff);
}

static int read_sidebar(area sidebar)
{
    int size = sidebar.width * sidebar.height;
    if (size > data.pixels_size) {
        color_t *pixels = realloc(data.pixels, size * sizeof(color_t));
        if (!pixels) {
            return 0;
        }
        data.pixels = pixels;
        data.pixels_size = size;
    }
    return graphics_renderer()->save_screen_buffer(data.pixels, sidebar.x, sidebar.y,
        sidebar.width, sidebar.height, sidebar.width);
}

static void draw_sidebar(void)
{
    area sidebar = get_sidebar_area();
    if (!read_sidebar(sidebar)) {
        return;
    }
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(data.window, &buffer, 0) != 0) {
        return;
    }
    uint32_t *bits = buffer.bits;
    for (int i = 0; i < buffer.stride * buffer.height; i++) {
        bits[i] = WINDOW_BLACK;
    }
    area panel = get_panel_area(buffer.width, buffer.height);
    for (int y = 0; y < panel.height; y++) {
        int window_y = panel.y + y;
        if (window_y < 0 || window_y >= buffer.height) {
            continue;
        }
        for (int x = 0; x < panel.width; x++) {
            int window_x = panel.x + x;
            if (window_x >= 0 && window_x < buffer.width) {
                bits[window_y * buffer.stride + window_x] = to_window_pixel(data.pixels[y * panel.width + x]);
            }
        }
    }
    ANativeWindow_unlockAndPost(data.window);
}

void android_bottom_screen_present(void)
{
    pthread_mutex_lock(&data.lock);
    if (data.window) {
        draw_sidebar();
    }
    pthread_mutex_unlock(&data.lock);
}

static int clamp(int value, int min, int max)
{
    return value < min ? min : (value > max ? max : value);
}

static void handle_touch(const touch_event *event, int width, int height)
{
    area sidebar = get_sidebar_area();
    area panel = get_panel_area(width, height);
    int x = (int) (event->x * width) - panel.x;
    int y = (int) (event->y * height) - panel.y;
    int is_inside = x >= 0 && x < panel.width && y >= 0 && y < panel.height;
    int screen_x = sidebar.x + clamp(x, 0, panel.width - 1);
    int screen_y = sidebar.y + clamp(y, 0, panel.height - 1);

    switch (event->action) {
        case MOTION_ACTION_DOWN:
            if (is_inside) {
                data.is_pressing = 1;
                mouse_set_position(screen_x, screen_y);
                mouse_set_left_down(1);
            }
            break;
        case MOTION_ACTION_MOVE:
            if (data.is_pressing) {
                mouse_set_position(screen_x, screen_y);
            }
            break;
        case MOTION_ACTION_UP:
        case MOTION_ACTION_CANCEL:
            if (data.is_pressing) {
                data.is_pressing = 0;
                mouse_set_position(screen_x, screen_y);
                mouse_set_left_down(0);
            }
            break;
        default:
            break;
    }
}

void android_bottom_screen_handle_input(void)
{
    touch_event events[MAX_PENDING_TOUCHES];

    pthread_mutex_lock(&data.lock);
    int count = data.pending_count;
    memcpy(events, data.pending, count * sizeof(touch_event));
    data.pending_count = 0;
    int width = data.width;
    int height = data.height;
    pthread_mutex_unlock(&data.lock);

    for (int i = 0; i < count; i++) {
        handle_touch(&events[i], width, height);
    }
}

JNIEXPORT void JNICALL Java_com_github_Keriew_augustus_BottomScreen_nativeSetSurface(
    JNIEnv *env, jclass cls, jobject surface, jint surface_width, jint surface_height)
{
    ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : 0;
    int width = surface_width / PIXEL_SCALE;
    int height = surface_height / PIXEL_SCALE;
    if (window) {
        ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    }

    pthread_mutex_lock(&data.lock);
    if (data.window) {
        ANativeWindow_release(data.window);
    }
    data.window = window;
    data.width = width;
    data.height = height;
    data.pending_count = 0;
    pthread_mutex_unlock(&data.lock);
}

JNIEXPORT void JNICALL Java_com_github_Keriew_augustus_BottomScreen_nativeTouch(
    JNIEnv *env, jclass cls, jint action, jfloat x, jfloat y)
{
    pthread_mutex_lock(&data.lock);
    touch_event *last = data.pending_count ? &data.pending[data.pending_count - 1] : 0;
    if (last && last->action == MOTION_ACTION_MOVE && action == MOTION_ACTION_MOVE) {
        last->x = x;
        last->y = y;
    } else if (data.pending_count < MAX_PENDING_TOUCHES) {
        touch_event *event = &data.pending[data.pending_count++];
        event->action = action;
        event->x = x;
        event->y = y;
    }
    pthread_mutex_unlock(&data.lock);
}

#endif // __ANDROID__
