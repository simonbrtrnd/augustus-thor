#include "zoom.h"

#include "city/view.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/speed.h"
#include "core/time.h"
#include "input/hotkey.h"
#include "input/joystick.h"

#include <math.h>

#define ZOOM_STEP 2

static const int ZOOM_LEVELS[] = { 50, 75, 100, 150, 200, 250, 300, 400, 500 };

#define NUM_ZOOM_LEVELS (sizeof(ZOOM_LEVELS) / sizeof(ZOOM_LEVELS[0]))

static const int ZOOM_MAGNETS[] = { 100, 200 };

#define NUM_ZOOM_MAGNETS (sizeof(ZOOM_MAGNETS) / sizeof(ZOOM_MAGNETS[0]))

#define MIN_ZOOM 50
#define HELD_ZOOM_DOUBLING_MILLIS 700
#define HELD_ZOOM_RAMP_MILLIS 250
#define HELD_ZOOM_START_SPEED 0.3f
#define MAGNET_HOLD_MILLIS 400
#define MAX_HELD_ZOOM_FRAME_MILLIS 100

static struct {
    int delta;
    int restore;
    pixel_offset input_offset;
    speed_type step;
    struct {
        int active;
        int start_zoom;
        int current_zoom;
    } touch;
    struct {
        int direction;
        float zoom;
        time_millis started;
        time_millis last_update;
        time_millis on_magnet_since;
    } held;
} data;

static void start_touch(const touch *first, const touch *last, int scale)
{
    data.restore = 0;
    data.touch.active = 1;
    data.input_offset.x = first->current_point.x;
    data.input_offset.y = first->current_point.y;
    data.touch.start_zoom = scale;
    data.touch.current_zoom = scale;
}

void zoom_update_touch(const touch *first, const touch *last, int scale)
{
    if (!data.touch.active) {
        start_touch(first, last, scale);
        return;
    }
    int original_distance, current_distance;
    pixel_offset temp;
    temp.x = first->start_point.x - last->start_point.x;
    temp.y = first->start_point.y - last->start_point.y;
    original_distance = (int) sqrt(temp.x * temp.x + temp.y * temp.y);
    temp.x = first->current_point.x - last->current_point.x;
    temp.y = first->current_point.y - last->current_point.y;
    current_distance = (int) sqrt(temp.x * temp.x + temp.y * temp.y);

    if (!original_distance || !current_distance) {
        data.touch.active = 0;
        return;
    }

    int finger_distance_percentage = calc_percentage(current_distance, original_distance);
    data.touch.current_zoom = calc_percentage(data.touch.start_zoom, finger_distance_percentage);
}

void zoom_end_touch(void)
{
    data.touch.active = 0;
}

static int get_y_in_viewport(int y)
{
    int x_offset, y_offset, width, height;
    city_view_get_viewport(&x_offset, &y_offset, &width, &height);
    return y - y_offset;
}

static int get_next_zoom_level(int zoom, int is_zoom_out)
{
    int max = city_view_get_max_scale();
    if (zoom > max) {
        zoom = max;
    }
    if (is_zoom_out) {
        for (unsigned int i = 0; i < NUM_ZOOM_LEVELS; i++) {
            if (ZOOM_LEVELS[i] > zoom) {
                return ZOOM_LEVELS[i] < max ? ZOOM_LEVELS[i] : max;
            }
        }
        return max;
    }
    for (int i = NUM_ZOOM_LEVELS - 1; i >= 0; i--) {
        if (ZOOM_LEVELS[i] < zoom) {
            return ZOOM_LEVELS[i];
        }
    }
    return zoom;
}

void zoom_map(const mouse *m, const hotkeys *h, int current_zoom)
{
    if (data.touch.active || m->is_touch) {
        return;
    }
    if (h->reset_zoom) {
        data.restore = 1;
        speed_clear(&data.step);
        data.input_offset.x = m->x;
        data.input_offset.y = get_y_in_viewport(m->y);
        return;
    }
    if (h->zoom_in || h->zoom_out) {
        int target = data.restore ? 100 : current_zoom + data.delta;
        data.restore = 0;
        data.delta = get_next_zoom_level(target, h->zoom_out) - current_zoom;
        if (config_get(CONFIG_UI_SMOOTH_SCROLLING)) {
            speed_clear(&data.step);
            speed_set_target(&data.step, ZOOM_STEP, SPEED_CHANGE_IMMEDIATE, 1);
        }
        data.input_offset.x = m->x;
        data.input_offset.y = get_y_in_viewport(m->y);
    }
}

static int is_magnet(int zoom)
{
    for (unsigned int i = 0; i < NUM_ZOOM_MAGNETS; i++) {
        if (ZOOM_MAGNETS[i] == zoom) {
            return 1;
        }
    }
    return 0;
}

static int get_magnet_passed(float from, float to)
{
    for (unsigned int i = 0; i < NUM_ZOOM_MAGNETS; i++) {
        float magnet = (float) ZOOM_MAGNETS[i];
        if ((from < magnet && magnet <= to) || (to <= magnet && magnet < from)) {
            return ZOOM_MAGNETS[i];
        }
    }
    return 0;
}

static void start_held_zoom(int direction, int zoom, time_millis now)
{
    data.held.direction = direction;
    data.held.zoom = (float) zoom;
    data.held.started = now;
    data.held.last_update = now;
    data.held.on_magnet_since = is_magnet(zoom) ? now : 0;
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    data.input_offset.x = width / 2;
    data.input_offset.y = height / 2;
}

static float get_held_zoom_speed(time_millis now)
{
    float ramp = (float) (now - data.held.started) / HELD_ZOOM_RAMP_MILLIS;
    if (ramp > 1.0f) {
        ramp = 1.0f;
    }
    return HELD_ZOOM_START_SPEED + (1.0f - HELD_ZOOM_START_SPEED) * ramp;
}

static int get_held_zoom_step(int zoom, int max, int *step)
{
    int direction = joystick_get_held_zoom();
    time_millis now = time_get_millis();
    if (!direction) {
        data.held.direction = 0;
        return 0;
    }
    if (direction != data.held.direction || fabsf(data.held.zoom - zoom) > 1.0f) {
        start_held_zoom(direction, zoom, now);
        *step = 0;
        return 1;
    }
    time_millis elapsed = now - data.held.last_update;
    if (elapsed > MAX_HELD_ZOOM_FRAME_MILLIS) {
        elapsed = MAX_HELD_ZOOM_FRAME_MILLIS;
    }
    data.held.last_update = now;
    *step = 0;
    if (data.held.on_magnet_since) {
        if (now - data.held.on_magnet_since < MAGNET_HOLD_MILLIS) {
            return 1;
        }
        data.held.on_magnet_since = 0;
        data.held.started = now;
    }
    float doublings = direction * get_held_zoom_speed(now) * elapsed / HELD_ZOOM_DOUBLING_MILLIS;
    float next = data.held.zoom * powf(2.0f, doublings);
    int magnet = get_magnet_passed(data.held.zoom, next);
    if (magnet) {
        next = (float) magnet;
        data.held.on_magnet_since = now;
    }
    if (next < MIN_ZOOM) {
        next = MIN_ZOOM;
    } else if (next > max) {
        next = (float) max;
    }
    data.held.zoom = next;
    *step = (int) (next + 0.5f) - zoom;
    return 1;
}

int zoom_update_value(int *zoom, int max, pixel_offset *camera_position)
{
    int step;
    if (get_held_zoom_step(*zoom, max, &step)) {
        speed_clear(&data.step);
        data.delta = 0;
        data.restore = 0;
    } else if (!data.touch.active) {
        if (data.restore) {
            data.delta = 100 - *zoom;
            if (config_get(CONFIG_UI_SMOOTH_SCROLLING)) {
                speed_set_target(&data.step, ZOOM_STEP, SPEED_CHANGE_IMMEDIATE, 1);
            }
            data.restore = 0;
        }
        if (data.delta == 0) {
            return 0;
        }
        if (config_get(CONFIG_UI_SMOOTH_SCROLLING)) {
            step = speed_get_delta(&data.step);
            step *= (*zoom / 100) + 1;
            if (!step) {
                return 1;
            }
        } else {
            step = data.delta;
        }

        data.delta = calc_absolute_decrement(data.delta, &step);

        if (data.delta == 0) {
            speed_clear(&data.step);
        }
    } else {
        speed_clear(&data.step);
        data.restore = 0;
        int current_zoom = data.touch.current_zoom;
        if (current_zoom > 90 && current_zoom < 110) {
            current_zoom = 100;
        }
        step = current_zoom - *zoom;
    }

    int result = calc_bound(*zoom + step, MIN_ZOOM, max);
    if (*zoom == result) {
        speed_clear(&data.step);
        data.delta = 0;
        return 0;
    }
    pixel_offset old_offset, new_offset;
    old_offset.x = calc_adjust_with_percentage(data.input_offset.x, *zoom);
    old_offset.y = calc_adjust_with_percentage(data.input_offset.y, *zoom);

    new_offset.x = calc_adjust_with_percentage(data.input_offset.x, result);
    new_offset.y = calc_adjust_with_percentage(data.input_offset.y, result);

    camera_position->x -= new_offset.x - old_offset.x;
    camera_position->y -= new_offset.y - old_offset.y;

    if (!config_get(CONFIG_UI_SMOOTH_SCROLLING) && !data.touch.active && !data.held.direction) {
        int remaining_x = camera_position->x & 60;
        int remaining_y = camera_position->y & 15;
        if (remaining_x >= 30) {
            remaining_x -= 60;
        }
        if (remaining_y >= 8) {
            remaining_y -= 15;
        }
        camera_position->x -= remaining_x;
        camera_position->y -= remaining_y;
    }
    *zoom = result;
    return 1;
}
