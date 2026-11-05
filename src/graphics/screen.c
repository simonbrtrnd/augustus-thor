#include "screen.h"

#include "city/view.h"
#include "city/warning.h"
#include "graphics/renderer.h"
#include "graphics/second_screen.h"
#include "graphics/window.h"

typedef struct {
    int x;
    int y;
} dialog_offset;

static struct {
    int width;
    int height;
    dialog_offset dialog_offset;
    dialog_offset second_screen_dialog_offset;
} data;

static dialog_offset *get_dialog_offset(void)
{
    return second_screen_is_current() ? &data.second_screen_dialog_offset : &data.dialog_offset;
}

void screen_set_dialog_offset(int width, int height)
{
    dialog_offset *offset = get_dialog_offset();
    offset->x = (screen_width() - width) / 2;
    offset->y = (screen_height() - height) / 2;
}

void screen_set_resolution(int width, int height)
{
    data.width = width;
    data.height = height;
    screen_set_dialog_offset(640, 480);

    graphics_renderer()->clear_screen();
    graphics_renderer()->set_clip_rectangle(0, 0, width, height);

    city_view_set_viewport(width, height);
    city_warning_clear_all();
    window_invalidate();
}

int screen_width(void)
{
    return second_screen_is_drawing() ? second_screen_width() : data.width;
}

int screen_height(void)
{
    return second_screen_is_drawing() ? second_screen_height() : data.height;
}

int screen_dialog_offset_x(void)
{
    return get_dialog_offset()->x;
}

int screen_dialog_offset_y(void)
{
    return get_dialog_offset()->y;
}
