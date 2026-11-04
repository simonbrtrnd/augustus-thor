#include "bottom_screen.h"

#ifdef __ANDROID__

#include "graphics/color.h"
#include "graphics/menu.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/second_screen.h"
#include "graphics/window.h"
#include "input/mouse.h"

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
    color_t *pixels;
    int pixels_size;
} data = { .lock = PTHREAD_MUTEX_INITIALIZER };

static int min(int a, int b)
{
    return a < b ? a : b;
}

static area get_screen_area(int width, int height)
{
    int area_width = min(width, screen_width());
    int area_height = min(height, screen_height() - TOP_MENU_HEIGHT);
    area screen_area = { screen_width() - area_width, TOP_MENU_HEIGHT, area_width, area_height };
    return screen_area;
}

static area get_window_area(int width, int height)
{
    area screen_area = get_screen_area(width, height);
    area window_area = {
        width - screen_area.width, (height - screen_area.height) / 2, screen_area.width, screen_area.height
    };
    return window_area;
}

static uint32_t to_window_pixel(color_t color)
{
    return (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color >> 16) & 0xff);
}

static int read_second_screen(area screen_area)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    if (!second_screen_has_content() || !renderer->save_second_screen_buffer) {
        return 0;
    }
    int size = screen_area.width * screen_area.height;
    if (size > data.pixels_size) {
        color_t *pixels = realloc(data.pixels, size * sizeof(color_t));
        if (!pixels) {
            return 0;
        }
        data.pixels = pixels;
        data.pixels_size = size;
    }
    return renderer->save_second_screen_buffer(data.pixels, screen_area.x, screen_area.y,
        screen_area.width, screen_area.height, screen_area.width);
}

static void draw_window(void)
{
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(data.window, &buffer, 0) != 0) {
        return;
    }
    uint32_t *bits = buffer.bits;
    for (int i = 0; i < buffer.stride * buffer.height; i++) {
        bits[i] = WINDOW_BLACK;
    }
    area screen_area = get_screen_area(buffer.width, buffer.height);
    if (read_second_screen(screen_area)) {
        area window_area = get_window_area(buffer.width, buffer.height);
        for (int y = 0; y < window_area.height; y++) {
            uint32_t *row = &bits[(window_area.y + y) * buffer.stride + window_area.x];
            const color_t *pixels = &data.pixels[y * window_area.width];
            for (int x = 0; x < window_area.width; x++) {
                row[x] = to_window_pixel(pixels[x]);
            }
        }
    }
    ANativeWindow_unlockAndPost(data.window);
}

void android_bottom_screen_present(void)
{
    pthread_mutex_lock(&data.lock);
    if (data.window) {
        draw_window();
    }
    pthread_mutex_unlock(&data.lock);
}

static mouse to_screen_mouse(const touch_event *event, int width, int height)
{
    area screen_area = get_screen_area(width, height);
    area window_area = get_window_area(width, height);
    mouse m = { 0 };
    m.x = screen_area.x + (int) (event->x * width) - window_area.x;
    m.y = screen_area.y + (int) (event->y * height) - window_area.y;
    m.is_inside_window = 1;
    m.window_has_focus = 1;
    m.is_touch = 1;
    switch (event->action) {
        case MOTION_ACTION_DOWN:
            m.left.went_down = 1;
            m.left.is_down = 1;
            break;
        case MOTION_ACTION_MOVE:
            m.left.is_down = 1;
            break;
        case MOTION_ACTION_UP:
            m.left.went_up = 1;
            break;
        default:
            break;
    }
    return m;
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
    int has_window = data.window != 0;
    pthread_mutex_unlock(&data.lock);

    second_screen_set_active(has_window);
    for (int i = 0; i < count; i++) {
        mouse m = to_screen_mouse(&events[i], width, height);
        window_handle_second_screen_input(&m);
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
