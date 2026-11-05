#include "second_screen.h"

#include "city/view.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/window.h"

#include <math.h>

#define PANEL_PIXEL_SCALE 4.0f
#define DIALOG_MARGIN 8

static struct {
    int screen_width;
    int screen_height;
    float pixel_scale;
    int has_content;
    int is_current;
} data = { .pixel_scale = PANEL_PIXEL_SCALE };

int second_screen_is_active(void)
{
    return data.screen_width > 0 && data.screen_height > 0;
}

int second_screen_width(void)
{
    return (int) (data.screen_width / data.pixel_scale);
}

int second_screen_height(void)
{
    return (int) (data.screen_height / data.pixel_scale);
}

int second_screen_is_current(void)
{
    return data.is_current;
}

void second_screen_set_size(int width, int height)
{
    if (data.screen_width == width && data.screen_height == height) {
        return;
    }
    data.screen_width = width;
    data.screen_height = height;
    data.has_content = 0;
    city_view_set_viewport(screen_width(), screen_height());
    window_invalidate();
}

static int begin_drawing(float pixel_scale)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    if (!second_screen_is_active() || !renderer->begin_second_screen_drawing) {
        return 0;
    }
    data.pixel_scale = pixel_scale;
    if (!renderer->begin_second_screen_drawing(data.screen_width, data.screen_height, pixel_scale)) {
        return 0;
    }
    data.is_current = 1;
    return 1;
}

static void end_drawing(void)
{
    data.is_current = 0;
    graphics_renderer()->end_second_screen_drawing();
}

static int begin_background(float pixel_scale)
{
    data.has_content = 0;
    if (!begin_drawing(pixel_scale)) {
        return 0;
    }
    graphics_clear_screen();
    data.has_content = 1;
    return 1;
}

int second_screen_draw(void (*draw)(void))
{
    if (!begin_drawing(data.pixel_scale)) {
        return 0;
    }
    draw();
    end_drawing();
    return 1;
}

int second_screen_draw_background(void (*draw)(void))
{
    if (!begin_background(PANEL_PIXEL_SCALE)) {
        return 0;
    }
    draw();
    end_drawing();
    return 1;
}

static float get_scale_to_fit(int width, int height)
{
    return fminf((float) data.screen_width / (width + 2 * DIALOG_MARGIN),
        (float) data.screen_height / (height + 2 * DIALOG_MARGIN));
}

int second_screen_draw_dialog_background(int width, int height, void (*draw)(void))
{
    if (!begin_background(get_scale_to_fit(width, height))) {
        return 0;
    }
    second_screen_draw_marble_background();
    draw();
    end_drawing();
    return 1;
}

void second_screen_draw_marble_background(void)
{
    int width_blocks = (second_screen_width() + BLOCK_SIZE - 1) / BLOCK_SIZE + 2;
    int height_blocks = (second_screen_height() + BLOCK_SIZE - 1) / BLOCK_SIZE + 2;
    // The outer blocks hold the panel border, so they are placed outside the screen to leave only marble
    outer_panel_draw(-BLOCK_SIZE, -BLOCK_SIZE, width_blocks, height_blocks);
}

void second_screen_handle_input(const mouse *m)
{
    data.is_current = 1;
    window_handle_second_screen_input(m);
    data.is_current = 0;
}

void second_screen_reset_content(void)
{
    data.has_content = 0;
}

int second_screen_has_content(void)
{
    return data.has_content;
}
