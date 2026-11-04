#include "second_screen.h"

#include "city/view.h"
#include "graphics/graphics.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/window.h"

static struct {
    int width;
    int height;
    int pixel_scale;
    int has_content;
} data;

int second_screen_is_active(void)
{
    return data.width > 0 && data.height > 0;
}

int second_screen_width(void)
{
    return data.width;
}

int second_screen_height(void)
{
    return data.height;
}

void second_screen_set_size(int width, int height, int pixel_scale)
{
    if (data.width == width && data.height == height && data.pixel_scale == pixel_scale) {
        return;
    }
    data.width = width;
    data.height = height;
    data.pixel_scale = pixel_scale;
    data.has_content = 0;
    city_view_set_viewport(screen_width(), screen_height());
    window_invalidate();
}

static int begin_drawing(void)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    return second_screen_is_active() && renderer->begin_second_screen_drawing &&
        renderer->begin_second_screen_drawing(data.width, data.height, data.pixel_scale);
}

int second_screen_draw(void (*draw)(void))
{
    if (!begin_drawing()) {
        return 0;
    }
    draw();
    graphics_renderer()->end_second_screen_drawing();
    return 1;
}

int second_screen_draw_background(void (*draw)(void))
{
    if (!begin_drawing()) {
        return 0;
    }
    graphics_clear_screen();
    draw();
    graphics_renderer()->end_second_screen_drawing();
    data.has_content = 1;
    return 1;
}

void second_screen_reset_content(void)
{
    data.has_content = 0;
}

int second_screen_has_content(void)
{
    return data.has_content;
}
