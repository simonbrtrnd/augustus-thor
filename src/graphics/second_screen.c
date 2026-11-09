#include "second_screen.h"

#include "city/view.h"
#include "core/config.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/window.h"

#include <math.h>

#define PANEL_PIXEL_SCALE 4.0f
#define BUTTON_SCALE 1.5f
// On the AYN Thor, 1240x1080 pixels at 2.5 are exactly 31x27 blocks, so dialog borders meet the screen edges
#define FULL_SCREEN_DIALOG_PIXEL_SCALE 2.5f
#define FULL_DIALOG_WIDTH 640
#define FULL_DIALOG_HEIGHT 480
#define DIALOG_MARGIN 8
#define DIALOG_BACKGROUND_DARKNESS 8

static struct {
    int screen_width;
    int screen_height;
    float pixel_scale;
    int has_content;
    int has_dialog;
    window_id dialog_window;
    window_id content_window;
    int is_drawing;
    int is_handling_input;
} data = { .pixel_scale = PANEL_PIXEL_SCALE };

int second_screen_is_active(void)
{
    return data.screen_width > 0 && data.screen_height > 0 && config_get(CONFIG_UI_USE_SECOND_SCREEN);
}

int second_screen_width(void)
{
    return (int) (data.screen_width / data.pixel_scale + 0.5f);
}

int second_screen_height(void)
{
    return (int) (data.screen_height / data.pixel_scale + 0.5f);
}

int second_screen_is_drawing(void)
{
    return data.is_drawing;
}

int second_screen_is_current(void)
{
    return data.is_drawing || data.is_handling_input;
}

float second_screen_button_scale(void)
{
    return second_screen_is_current() ? BUTTON_SCALE : 0;
}

void second_screen_set_size(int width, int height)
{
    if (data.screen_width == width && data.screen_height == height) {
        return;
    }
    data.screen_width = width;
    data.screen_height = height;
    second_screen_update_layout();
}

void second_screen_update_layout(void)
{
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
    if (!renderer->begin_second_screen_drawing(data.screen_width, data.screen_height, pixel_scale)) {
        return 0;
    }
    data.pixel_scale = pixel_scale;
    data.is_drawing = 1;
    return 1;
}

static void end_drawing(void)
{
    data.is_drawing = 0;
    graphics_renderer()->end_second_screen_drawing();
}

static int begin_background(float pixel_scale)
{
    data.has_content = 0;
    data.has_dialog = 0;
    if (!begin_drawing(pixel_scale)) {
        return 0;
    }
    graphics_clear_screen();
    data.has_content = 1;
    data.content_window = window_get_id();
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

static int shows_dialogs(void)
{
    return second_screen_is_active() && config_get(CONFIG_UI_DIALOGS_ON_SECOND_SCREEN);
}

static int draw_dialog_background(float pixel_scale, int is_darkened, void (*draw)(void))
{
    if (!shows_dialogs() || !begin_background(pixel_scale)) {
        return 0;
    }
    second_screen_draw_marble_background();
    if (is_darkened) {
        graphics_shade_rect(0, 0, second_screen_width() + 1, second_screen_height() + 1, DIALOG_BACKGROUND_DARKNESS);
    }
    draw();
    end_drawing();
    data.has_dialog = 1;
    data.dialog_window = window_get_drawing_id();
    return 1;
}

static float get_full_dialog_scale(void)
{
    return fminf((float) data.screen_width / (FULL_DIALOG_WIDTH + 2 * DIALOG_MARGIN),
        (float) data.screen_height / (FULL_DIALOG_HEIGHT + 2 * DIALOG_MARGIN));
}

static int has_dialog_below(void)
{
    return data.has_content && data.has_dialog && data.content_window == window_get_id();
}

static int is_opened_from_bottom_screen(void)
{
    switch (window_get_id_behind()) {
        case WINDOW_CITY:
        case WINDOW_CITY_MILITARY:
        case WINDOW_TOP_MENU:
        case WINDOW_BUILD_MENU:
        case WINDOW_OVERLAY_MENU:
        case WINDOW_MILITARY_MENU:
            return 1;
        default:
            return 0;
    }
}

void second_screen_draw_full_screen_dialog(int min_height, void (*draw)(void))
{
    float pixel_scale = FULL_SCREEN_DIALOG_PIXEL_SCALE;
    if (min_height * pixel_scale > data.screen_height) {
        pixel_scale = (float) data.screen_height / min_height;
    }
    if (window_is_drawing_underlying_window()) {
        // A window opened over this dialog is drawn at its scale, so the dialog shrinks until a full dialog fits
        pixel_scale = fminf(pixel_scale, get_full_dialog_scale());
    }
    int opens_below = has_dialog_below() || is_opened_from_bottom_screen();
    if (!opens_below || !draw_dialog_background(pixel_scale, 0, draw)) {
        draw();
    }
}

int second_screen_has_dialog(void)
{
    return has_dialog_below() && data.dialog_window == window_get_id();
}

void second_screen_draw_dialog(void (*draw)(void))
{
    int is_drawn = 0;
    if (has_dialog_below()) {
        is_drawn = second_screen_draw(draw);
        data.dialog_window = window_get_drawing_id();
    } else if (is_opened_from_bottom_screen()) {
        is_drawn = draw_dialog_background(get_full_dialog_scale(), 1, draw);
    }
    if (!is_drawn) {
        draw();
    }
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
    if (!data.has_content || data.content_window != window_get_id()) {
        return;
    }
    data.is_handling_input = 1;
    window_handle_second_screen_input(m);
    data.is_handling_input = 0;
}

void second_screen_reset_content(void)
{
    data.has_content = 0;
}

int second_screen_has_content(void)
{
    return data.has_content;
}
