#include "bottom_screen.h"

#ifdef __ANDROID__

#include "graphics/color.h"
#include "graphics/renderer.h"
#include "graphics/second_screen.h"
#include "graphics/window.h"
#include "input/mouse.h"

#include <android/native_window_jni.h>
#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PIXEL_SCALE 4
#define MAX_PENDING_TOUCHES 32
#define WINDOW_BLACK 0xff000000

enum {
    MOTION_ACTION_DOWN = 0,
    MOTION_ACTION_UP = 1,
    MOTION_ACTION_MOVE = 2,
    MOTION_ACTION_CANCEL = 3
};

typedef enum {
    SHOWING_NOTHING,
    SHOWING_BLACK,
    SHOWING_CANVAS
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
    touch_event pending[MAX_PENDING_TOUCHES];
    int pending_count;
    color_t *canvas;
    color_t *shown_canvas;
    int canvas_size;
    int *source_columns;
    int source_columns_size;
    window_content shown;
} data = { .lock = PTHREAD_MUTEX_INITIALIZER };

static uint32_t to_window_pixel(color_t color)
{
    return (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color >> 16) & 0xff);
}

static int reserve_canvas(int size)
{
    if (size <= data.canvas_size) {
        return 1;
    }
    color_t *canvas = realloc(data.canvas, size * sizeof(color_t));
    if (canvas) {
        data.canvas = canvas;
    }
    color_t *shown_canvas = realloc(data.shown_canvas, size * sizeof(color_t));
    if (shown_canvas) {
        data.shown_canvas = shown_canvas;
    }
    if (!canvas || !shown_canvas) {
        return 0;
    }
    data.canvas_size = size;
    return 1;
}

static int read_canvas(void)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    return second_screen_has_content() && renderer->save_second_screen_buffer &&
        reserve_canvas(data.width * data.height) &&
        renderer->save_second_screen_buffer(data.canvas, 0, 0, data.width, data.height, data.width);
}

static int is_canvas_shown(void)
{
    return data.shown == SHOWING_CANVAS &&
        memcmp(data.canvas, data.shown_canvas, data.width * data.height * sizeof(color_t)) == 0;
}

static const int *get_source_columns(int window_width)
{
    if (window_width > data.source_columns_size) {
        int *columns = realloc(data.source_columns, window_width * sizeof(int));
        if (!columns) {
            return 0;
        }
        data.source_columns = columns;
        data.source_columns_size = window_width;
    }
    for (int x = 0; x < window_width; x++) {
        data.source_columns[x] = x * data.width / window_width;
    }
    return data.source_columns;
}

static void scale_canvas(const ANativeWindow_Buffer *buffer)
{
    const int *columns = get_source_columns(buffer->width);
    if (!columns) {
        return;
    }
    uint32_t *bits = buffer->bits;
    for (int y = 0; y < buffer->height; y++) {
        uint32_t *row = &bits[y * buffer->stride];
        const color_t *source = &data.canvas[y * data.height / buffer->height * data.width];
        for (int x = 0; x < buffer->width; x++) {
            row[x] = to_window_pixel(source[columns[x]]);
        }
    }
}

static void fill_black(const ANativeWindow_Buffer *buffer)
{
    uint32_t *bits = buffer->bits;
    for (int i = 0; i < buffer->stride * buffer->height; i++) {
        bits[i] = WINDOW_BLACK;
    }
}

static void draw_window(void)
{
    int has_canvas = read_canvas();
    if (has_canvas ? is_canvas_shown() : data.shown == SHOWING_BLACK) {
        return;
    }
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(data.window, &buffer, 0) != 0) {
        return;
    }
    if (has_canvas) {
        scale_canvas(&buffer);
        memcpy(data.shown_canvas, data.canvas, data.width * data.height * sizeof(color_t));
        data.shown = SHOWING_CANVAS;
    } else {
        fill_black(&buffer);
        data.shown = SHOWING_BLACK;
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
    mouse m = { 0 };
    m.x = (int) (event->x * width);
    m.y = (int) (event->y * height);
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

    second_screen_set_size(has_window ? width : 0, has_window ? height : 0);
    for (int i = 0; i < count; i++) {
        mouse m = to_screen_mouse(&events[i], width, height);
        window_handle_second_screen_input(&m);
    }
}

JNIEXPORT void JNICALL Java_com_github_Keriew_augustus_BottomScreen_nativeSetSurface(
    JNIEnv *env, jclass cls, jobject surface, jint surface_width, jint surface_height)
{
    ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : 0;
    if (window) {
        ANativeWindow_setBuffersGeometry(window, surface_width, surface_height, WINDOW_FORMAT_RGBA_8888);
    }

    pthread_mutex_lock(&data.lock);
    if (data.window) {
        ANativeWindow_release(data.window);
    }
    data.window = window;
    data.width = (int) (surface_width / PIXEL_SCALE);
    data.height = (int) (surface_height / PIXEL_SCALE);
    data.pending_count = 0;
    data.shown = SHOWING_NOTHING;
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
