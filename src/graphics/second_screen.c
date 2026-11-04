#include "second_screen.h"

#include "city/view.h"
#include "graphics/graphics.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/window.h"

static struct {
    int active;
    int has_content;
} data;

int second_screen_is_active(void)
{
    return data.active;
}

void second_screen_set_active(int active)
{
    if (data.active == active) {
        return;
    }
    data.active = active;
    data.has_content = 0;
    city_view_set_viewport(screen_width(), screen_height());
    window_invalidate();
}

static int begin_drawing(void)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    return data.active && renderer->begin_second_screen_drawing && renderer->begin_second_screen_drawing();
}

void second_screen_draw(void (*draw)(void))
{
    if (!begin_drawing()) {
        draw();
        return;
    }
    draw();
    graphics_renderer()->end_second_screen_drawing();
}

void second_screen_draw_background(void (*draw)(void))
{
    if (!begin_drawing()) {
        draw();
        return;
    }
    graphics_clear_screen();
    draw();
    graphics_renderer()->end_second_screen_drawing();
    data.has_content = 1;
}

void second_screen_reset_content(void)
{
    data.has_content = 0;
}

int second_screen_has_content(void)
{
    return data.has_content;
}
