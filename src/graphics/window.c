#include "window.h"

#include "game/system.h"
#include "graphics/graphics.h"
#include "graphics/second_screen.h"
#include "graphics/warning.h"
#include "input/cursor.h"
#include "input/hotkey.h"
#include "input/joystick.h"
#include "input/scroll.h"
#include "input/touch.h"
#include "window/city.h"

#define MAX_QUEUE 5

static struct {
    window_type window_queue[MAX_QUEUE];
    int queue_index;
    window_type *current_window;
    int refresh_immediate;
    int refresh_on_draw;
    int underlying_windows_redrawing;
} data;

static void noop(void)
{}

static void noop_input(const mouse *m, const hotkeys *h)
{}

static void noop_return(window_id from)
{}

static void increase_queue_index(void)
{
    data.queue_index++;
    if (data.queue_index >= MAX_QUEUE) {
        data.queue_index = 0;
    }
}

static void decrease_queue_index(void)
{
    data.queue_index--;
    if (data.queue_index < 0) {
        data.queue_index = MAX_QUEUE - 1;
    }
}

static void reset_input(void)
{
    mouse_reset_button_state();
    reset_touches(1);
    scroll_stop();
}

void window_invalidate(void)
{
    data.refresh_immediate = 1;
    data.refresh_on_draw = 1;
}

int window_is_invalid(void)
{
    return data.refresh_immediate;
}

void window_request_refresh(void)
{
    data.refresh_on_draw = 1;
}

int window_is(window_id id)
{
    return data.current_window->id == id;
}

window_id window_get_id(void)
{
    return data.current_window->id;
}

window_id window_get_id_behind(void)
{
    int index = data.queue_index > 0 ? data.queue_index - 1 : MAX_QUEUE - 1;
    return data.window_queue[index].id;
}

void window_show(const window_type *window)
{
    reset_input();
    increase_queue_index();
    data.window_queue[data.queue_index] = *window;
    data.current_window = &data.window_queue[data.queue_index];
    if (!data.current_window->draw_background) {
        data.current_window->draw_background = noop;
    }
    if (!data.current_window->draw_foreground) {
        data.current_window->draw_foreground = noop;
    }
    if (!data.current_window->handle_input) {
        data.current_window->handle_input = noop_input;
    }
    if (!data.current_window->on_return) {
        data.current_window->on_return = noop_return;
    }
    window_invalidate();
}

void window_go_back(void)
{
    reset_input();
    window_id from_id = window_get_id();
    decrease_queue_index();
    data.current_window = &data.window_queue[data.queue_index];
    data.current_window->on_return(from_id);
    window_invalidate();
}

static void update_input_before(void)
{
    int handled = touch_to_mouse();
    handled |= joystick_to_mouse_and_keyboard();
    if (!handled) {
        mouse_determine_button_state();  // touch and joystick override mouse
    }
    hotkey_handle_global_keys();
}

static mouse get_go_back_mouse(void)
{
    const mouse *m = mouse_get();
    return (mouse) { .x = -1, .y = -1, .right = m->right, .is_touch = m->is_touch };
}

static void pass_scroll_to_second_screen_dialog(void)
{
    const mouse *m = mouse_get();
    if (m->scrolled != SCROLL_NONE) {
        mouse scroll = { .x = -1, .y = -1, .scrolled = m->scrolled };
        second_screen_handle_input(&scroll);
    }
}

static void update_input_after(void)
{
    reset_touches(0);
    mouse_reset_scroll();
    input_cursor_update(data.current_window->id);
    hotkey_reset_state();
}

void window_draw(int force)
{
    update_input_before();
    window_type *w = data.current_window;
    if (force || data.refresh_on_draw) {
        data.refresh_on_draw = 0;
        data.refresh_immediate = 0;
        graphics_clear_screen();
        second_screen_reset_content();
        w->draw_background();
    }
    w->draw_foreground();

    const hotkeys *h = hotkey_state();
    if (second_screen_has_dialog()) {
        mouse go_back = get_go_back_mouse();
        w->handle_input(&go_back, h);
        pass_scroll_to_second_screen_dialog();
    } else {
        const mouse *m = mouse_get();
        w->handle_input(m, h);
        tooltip_handle(m, w->get_tooltip);
    }
    warning_draw();
    update_input_after();
}

void window_handle_second_screen_input(const mouse *m)
{
    static const hotkeys no_hotkeys;
    if (data.current_window->handle_second_screen_input) {
        data.current_window->handle_second_screen_input(m);
    } else if (second_screen_has_dialog()) {
        data.current_window->handle_input(m, &no_hotkeys);
    }
}

int window_is_drawing_underlying_window(void)
{
    return data.underlying_windows_redrawing > 0;
}

void window_draw_underlying_window(void)
{
    if (data.underlying_windows_redrawing < MAX_QUEUE) {
        ++data.underlying_windows_redrawing;
        decrease_queue_index();
        window_type *window_behind = &data.window_queue[data.queue_index];
        window_behind->draw_background();
        window_behind->draw_foreground();
        increase_queue_index();
        --data.underlying_windows_redrawing;
    }
}
