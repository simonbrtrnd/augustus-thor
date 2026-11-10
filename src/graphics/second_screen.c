#include "second_screen.h"

#include "city/view.h"
#include "core/config.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "core/calc.h"
#include "core/time.h"
#include "game/system.h"

#include <math.h>
#include <stdlib.h>

#define PANEL_PIXEL_SCALE 4.0f
#define BUTTON_SCALE 1.5f
// On the AYN Thor, 1240x1080 pixels at 2.5 are exactly 31x27 blocks, so dialog borders meet the screen edges
#define FULL_SCREEN_DIALOG_PIXEL_SCALE 2.5f
#define FULL_DIALOG_WIDTH 640
#define FULL_DIALOG_HEIGHT 480
#define DIALOG_MARGIN 8
#define DIALOG_BACKGROUND_DARKNESS 8
#define MAX_LISTED_BUTTONS 64
#define BUTTON_LISTING_PROBE (-10000)
#define CURSOR_JUMP_MILLIS 120
#define FOCUS_BORDER_COLOR 0xff8c2c24
#define FOCUS_BORDER_SCREEN_PIXELS 6
#define SAME_ROW_DISTANCE 10

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

typedef struct {
    int x;
    int y;
    int width;
    int height;
} button_area;

static struct {
    int is_active;
    int is_visible;
    int x;
    int y;
    int main_screen_x;
    int main_screen_y;
    int has_focus;
    button_area focus;
    time_millis last_jump;
    int is_drawn;
    int is_clicking;
} cursor;

static struct {
    int is_listing;
    int count;
    button_area buttons[MAX_LISTED_BUTTONS];
} listed;

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
        case WINDOW_BUILDING_INFO:
        case WINDOW_MESSAGE_LIST:
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
    if (m->left.went_down && !cursor.is_clicking) {
        cursor.is_visible = 0;
        cursor.has_focus = 0;
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

int second_screen_cursor_is_shown(void)
{
    return cursor.is_active;
}

static void activate_cursor(void)
{
    const mouse *m = mouse_get();
    cursor.is_active = 1;
    cursor.is_visible = 0;
    cursor.has_focus = 0;
    cursor.main_screen_x = m->x;
    cursor.main_screen_y = m->y;
    cursor.x = second_screen_width() / 2;
    cursor.y = second_screen_height() / 2;
    system_hide_cursor();
}

static void deactivate_cursor(void)
{
    cursor.is_active = 0;
    int x = cursor.main_screen_x;
    int y = cursor.main_screen_y;
    system_set_mouse_position(&x, &y);
    mouse_set_position(x, y);
    system_show_cursor();
    window_request_refresh();
}

static void update_cursor_activity(void)
{
    int is_wanted = second_screen_has_dialog() && !mouse_get()->is_touch;
    if (is_wanted && !cursor.is_active) {
        activate_cursor();
    } else if (!is_wanted && cursor.is_active) {
        deactivate_cursor();
    }
}

int second_screen_cursor_move(int delta_x, int delta_y)
{
    update_cursor_activity();
    if (!cursor.is_active) {
        return 0;
    }
    cursor.x = calc_bound(cursor.x + delta_x, 0, second_screen_width() - 1);
    cursor.y = calc_bound(cursor.y + delta_y, 0, second_screen_height() - 1);
    cursor.is_visible = 1;
    cursor.has_focus = 0;
    return 1;
}

int second_screen_cursor_click(int is_right_button, int is_down)
{
    update_cursor_activity();
    if (!cursor.is_active) {
        return 0;
    }
    if (!is_right_button && !cursor.is_visible && !cursor.has_focus) {
        return 1;
    }
    mouse m = { 0 };
    m.x = cursor.x;
    m.y = cursor.y;
    m.is_inside_window = 1;
    m.window_has_focus = 1;
    mouse_button *button = is_right_button ? &m.right : &m.left;
    button->is_down = is_down;
    button->went_down = is_down;
    button->went_up = !is_down;
    cursor.is_clicking = 1;
    second_screen_handle_input(&m);
    cursor.is_clicking = 0;
    return 1;
}

static int contains_center_of(const button_area *outer, const button_area *inner)
{
    int center_x = inner->x + inner->width / 2;
    int center_y = inner->y + inner->height / 2;
    return center_x >= outer->x && center_x < outer->x + outer->width &&
        center_y >= outer->y && center_y < outer->y + outer->height;
}

void second_screen_list_button(const mouse *m, int x, int y, int width, int height)
{
    if (!listed.is_listing || listed.count >= MAX_LISTED_BUTTONS || width <= 0 || height <= 0) {
        return;
    }
    button_area button = { x + BUTTON_LISTING_PROBE - m->x, y + BUTTON_LISTING_PROBE - m->y, width, height };
    for (int i = 0; i < listed.count; i++) {
        button_area *other = &listed.buttons[i];
        if (contains_center_of(other, &button) || contains_center_of(&button, other)) {
            if (button.width * button.height > other->width * other->height) {
                *other = button;
            }
            return;
        }
    }
    listed.buttons[listed.count++] = button;
}

static void list_dialog_buttons(void)
{
    mouse probe = { 0 };
    probe.x = BUTTON_LISTING_PROBE;
    probe.y = BUTTON_LISTING_PROBE;
    probe.is_inside_window = 1;
    probe.window_has_focus = 1;
    listed.count = 0;
    listed.is_listing = 1;
    second_screen_handle_input(&probe);
    listed.is_listing = 0;
}

static const button_area *find_button_in_direction(int direction_x, int direction_y)
{
    const button_area *best = 0;
    int best_distance = 0;
    for (int i = 0; i < listed.count; i++) {
        const button_area *button = &listed.buttons[i];
        int x = button->x + button->width / 2 - cursor.x;
        int y = button->y + button->height / 2 - cursor.y;
        int along = x * direction_x + y * direction_y;
        int across = abs(x * direction_y - y * direction_x);
        if (along <= 0 || across > along) {
            continue;
        }
        int distance = along + 2 * across;
        if (!best || distance < best_distance) {
            best = button;
            best_distance = distance;
        }
    }
    return best;
}

int second_screen_cursor_jump(int direction_x, int direction_y)
{
    update_cursor_activity();
    if (!cursor.is_active) {
        return 0;
    }
    if (time_get_millis() - cursor.last_jump < CURSOR_JUMP_MILLIS) {
        return 1;
    }
    cursor.is_visible = 0;
    list_dialog_buttons();
    const button_area *button = find_button_in_direction(direction_x, direction_y);
    if (!button) {
        return 0;
    }
    cursor.focus = *button;
    cursor.has_focus = 1;
    cursor.is_visible = 0;
    cursor.x = button->x + button->width / 2;
    cursor.y = button->y + button->height / 2;
    cursor.last_jump = time_get_millis();
    return 1;
}

void second_screen_cursor_scroll(int direction_y)
{
    if (!cursor.is_active || !direction_y) {
        return;
    }
    mouse m = { 0 };
    m.x = cursor.x;
    m.y = cursor.y;
    m.is_inside_window = 1;
    m.window_has_focus = 1;
    m.scrolled = direction_y > 0 ? SCROLL_DOWN : SCROLL_UP;
    cursor.is_clicking = 1;
    second_screen_handle_input(&m);
    cursor.is_clicking = 0;
}

static int is_focus_listed(void)
{
    for (int i = 0; i < listed.count; i++) {
        const button_area *button = &listed.buttons[i];
        if (button->x == cursor.focus.x && button->y == cursor.focus.y &&
            button->width == cursor.focus.width && button->height == cursor.focus.height) {
            return 1;
        }
    }
    return 0;
}

static const button_area *find_first_button(void)
{
    const button_area *first = 0;
    for (int i = 0; i < listed.count; i++) {
        const button_area *button = &listed.buttons[i];
        if (!first || button->y + button->height / 2 < first->y + first->height / 2) {
            first = button;
        }
    }
    if (!first) {
        return 0;
    }
    int top_row_y = first->y + first->height / 2;
    for (int i = 0; i < listed.count; i++) {
        const button_area *button = &listed.buttons[i];
        if (button->y + button->height / 2 - top_row_y <= SAME_ROW_DISTANCE && button->x < first->x) {
            first = button;
        }
    }
    return first;
}

static void focus_first_button_after_page_change(void)
{
    list_dialog_buttons();
    if (is_focus_listed()) {
        return;
    }
    const button_area *button = find_first_button();
    if (!button) {
        cursor.has_focus = 0;
        return;
    }
    cursor.focus = *button;
    cursor.x = button->x + button->width / 2;
    cursor.y = button->y + button->height / 2;
}

static void draw_cursor(void)
{
    const graphics_renderer_interface *renderer = graphics_renderer();
    if (cursor.has_focus && renderer->draw_frame_in_screen_pixels) {
        renderer->draw_frame_in_screen_pixels(cursor.focus.x, cursor.focus.y, cursor.focus.width,
            cursor.focus.height, FOCUS_BORDER_SCREEN_PIXELS, FOCUS_BORDER_COLOR);
    }
    if (cursor.is_visible && renderer->draw_mouse_cursor) {
        renderer->draw_mouse_cursor(cursor.x, cursor.y);
    }
}

void second_screen_erase_cursor(void)
{
    if (cursor.is_drawn) {
        graphics_renderer()->restore_second_screen();
        cursor.is_drawn = 0;
    }
}

void second_screen_draw_cursor(int is_window_redrawn)
{
    update_cursor_activity();
    if (cursor.is_active && cursor.has_focus && is_window_redrawn) {
        focus_first_button_after_page_change();
    }
    const graphics_renderer_interface *renderer = graphics_renderer();
    if (!cursor.is_active || (!cursor.is_visible && !cursor.has_focus) ||
        !renderer->save_second_screen || !renderer->save_second_screen()) {
        return;
    }
    cursor.is_drawn = second_screen_draw(draw_cursor);
}
