#include "city.h"

#include "assets/assets.h"
#include "building/menu.h"
#include "city/finance.h"
#include "city/message.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/config.h"
#include "core/direction.h"
#include "core/locale.h"
#include "core/string.h"
#include "core/time.h"
#include "game/campaign.h"
#include "game/orientation.h"
#include "game/state.h"
#include "game/time.h"
#include "game/undo.h"
#include "graphics/font.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/second_screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "map/orientation.h"
#include "scenario/property.h"
#include "translation/translation.h"
#include "widget/city/city.h"
#include "widget/minimap.h"
#include "widget/sidebar/common.h"
#include "widget/sidebar/extra.h"
#include "widget/sidebar/slide.h"
#include "widget/top_menu.h"
#include "window/advisors.h"
#include "window/build_menu.h"
#include "window/city.h"
#include "window/empire.h"
#include "window/message_dialog.h"
#include "window/message_list.h"
#include "window/mission_briefing.h"
#include "window/overlay_menu.h"

#include <stdlib.h>

#define MINIMAP_Y_OFFSET 59
#define PANEL_LIST_MARGIN 6
#define PANEL_LIST_ROW_HEIGHT 24
#define PANEL_LIST_TOP (2 * PANEL_LIST_MARGIN + PANEL_LIST_ROW_HEIGHT)
#define PANEL_LIST_ROW_GAP 4
#define PANEL_LIST_COLUMN_GAP 6
#define PANEL_LIST_SCROLL_STEP_MILLIS 120
#define PANEL_TEXT_MARGIN 6
#define PANEL_FONT FONT_SMALL_PLAIN
#define PANEL_FONT_CAP_HEIGHT 9
#define PANEL_TEXT_COLOR 0xff160901
#define PANEL_TEXT_CARVING_COLOR 0xffcbbe99
#define PANEL_TEXT_CARVING_OFFSET 1
#define PANEL_TEXT_HEIGHT (PANEL_FONT_CAP_HEIGHT + PANEL_TEXT_CARVING_OFFSET)
#define PANEL_RATINGS_LINE_HEIGHT 13
#define PANEL_PRESSED_MILLIS 150
#define PANEL_BOX_COLOR 0xff313131
#define PANEL_BOX_BORDER_COLOR 0xff121212
#define PANEL_ASSETS "Thor_Panel"
#define MINIMAP_GLIDE_PERCENT 35
#define PANEL_IMAGE_SCALE 4.0f
#define PANEL_MESSAGE_COUNT_FONT FONT_NORMAL_PLAIN
#define PANEL_MESSAGE_COUNT_CAP_HEIGHT 11
#define PANEL_MESSAGE_COUNT_LARGE_MAX 99
#define PANEL_MESSAGE_COUNT_CENTER_X 38
#define PANEL_MESSAGE_COUNT_CENTER_Y 15

typedef struct {
    int x;
    int y;
    int width;
    int height;
} panel_area;

static void button_overlay(int param1, int param2);
static void button_collapse_expand(int param1, int param2);
static void button_build(int submenu, int param2);
static void button_undo(int param1, int param2);
static void button_messages(int param1, int param2);
static void button_help(int param1, int param2);
static void button_go_to_problem(int param1, int param2);
static void button_advisors(int param1, int param2);
static void button_empire(int param1, int param2);
static void button_toggle_grid(int param1, int param2);
static void button_rotate_north(int param1, int param2);
static void button_rotate(int clockwise, int param2);
static void button_toggle_pause(int param1, int param2);
static void button_panel_overlay(const generic_button *button);
static void button_panel_menu(const generic_button *button);
static void button_panel_advisor(const generic_button *button);
static void button_panel_advisors(int param1, int param2);

static image_button buttons_overlays_collapse_sidebar[] = {
    {127, 5, 31, 20, IB_NORMAL, 90, 0, button_collapse_expand, button_none, 0, 0, 1},
    {4, 3, 117, 31, IB_NORMAL, 93, 0, button_overlay, button_help, 0, MESSAGE_DIALOG_OVERLAYS, 1}
};

static image_button button_expand_sidebar[] = {
    {6, 4, 31, 20, IB_NORMAL, 90, 4, button_collapse_expand, button_none, 0, 0, 1}
};

static image_button buttons_build_collapsed[] = {
    {2, 32, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 0, button_build, button_none, BUILD_MENU_VACANT_HOUSE, 0, 1},
    {2, 67, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 8, button_build, button_none, BUILD_MENU_CLEAR_LAND, 0, 1},
    {2, 102, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 12, button_build, button_none, BUILD_MENU_ROAD, 0, 1},
    {2, 137, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 4, button_build, button_none, BUILD_MENU_WATER, 0, 1},
    {2, 172, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 40, button_build, button_none, BUILD_MENU_HEALTH, 0, 1, },
    {2, 207, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 28, button_build, button_none, BUILD_MENU_TEMPLES, 0, 1},
    {2, 242, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 24, button_build, button_none, BUILD_MENU_EDUCATION, 0, 1},
    {2, 277, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 20, button_build, button_none, BUILD_MENU_ENTERTAINMENT, 0, 1},
    {2, 312, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 16, button_build, button_none, BUILD_MENU_ADMINISTRATION, 0, 1},
    {2, 347, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 44, button_build, button_none, BUILD_MENU_ENGINEERING, 0, 1},
    {2, 382, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 36, button_build, button_none, BUILD_MENU_SECURITY, 0, 1},
    {2, 417, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 32, button_build, button_none, BUILD_MENU_INDUSTRY, 0, 1},
};

static image_button buttons_build_expanded[] = {
    {13, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 0, button_build, button_none, BUILD_MENU_VACANT_HOUSE, 0, 1},
    {63, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 8, button_build, button_none, BUILD_MENU_CLEAR_LAND, 0, 1},
    {113, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 12, button_build, button_none, BUILD_MENU_ROAD, 0, 1},
    {13, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 4, button_build, button_none, BUILD_MENU_WATER, 0, 1},
    {63, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 40, button_build, button_none, BUILD_MENU_HEALTH, 0, 1,},
    {113, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 28, button_build, button_none, BUILD_MENU_TEMPLES, 0, 1},
    {13, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 24, button_build, button_none, BUILD_MENU_EDUCATION, 0, 1},
    {63, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 20, button_build, button_none, BUILD_MENU_ENTERTAINMENT, 0, 1},
    {113, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 16, button_build, button_none, BUILD_MENU_ADMINISTRATION, 0, 1},
    {13, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 44, button_build, button_none, BUILD_MENU_ENGINEERING, 0, 1},
    {63, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 36, button_build, button_none, BUILD_MENU_SECURITY, 0, 1},
    {113, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 32, button_build, button_none, BUILD_MENU_INDUSTRY, 0, 1},
    {13, 421, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 48, button_undo, button_none, 0, 0, 1},
    {63, 421, 39, 26, IB_NORMAL, GROUP_MESSAGE_ICON, 18, button_messages, button_help, 0, MESSAGE_DIALOG_MESSAGES, 1},
    {113, 421, 39, 26, IB_BUILD, GROUP_MESSAGE_ICON, 22, button_go_to_problem, button_none, 0, 0, 1},
};

static image_button buttons_top_expanded[] = {
    {7, 155, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 0, button_advisors, button_none, 0, 0, 1},
    {84, 155, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 3, button_empire, button_help, 0, MESSAGE_DIALOG_EMPIRE_MAP, 1},
    {7, 184, 33, 22, IB_NORMAL, 0, 0, button_toggle_grid, button_none, 0, 0, 1, "UI", "Toggle Grid Button" },
    {46, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 3, button_rotate_north, button_none, 0, 0, 1},
    {84, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 6, button_rotate, button_none, 0, 0, 1},
    {123, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 9, button_rotate, button_none, 1, 0, 1},
};

static const panel_area panel_date = {140, 4, 166, 31};
static const panel_area panel_minimap = {4, 136, 134, 97};

enum {
    PANEL_BUTTON_FUNDS,
    PANEL_BUTTON_POPULATION,
    PANEL_BUTTON_RATINGS,
    PANEL_BUTTON_OVERLAY,
    PANEL_BUTTON_MENU,
    PANEL_TEXT_BUTTONS
};

static generic_button panel_text_buttons[PANEL_TEXT_BUTTONS] = {
    [PANEL_BUTTON_FUNDS] = {140, 37, 82, 31, button_panel_advisor, 0, ADVISOR_FINANCIAL},
    [PANEL_BUTTON_POPULATION] = {224, 37, 82, 31, button_panel_advisor, 0, ADVISOR_POPULATION},
    [PANEL_BUTTON_RATINGS] = {4, 37, 134, 64, button_panel_advisor, 0, ADVISOR_RATINGS},
    [PANEL_BUTTON_OVERLAY] = {4, 103, 134, 31, button_panel_overlay},
    [PANEL_BUTTON_MENU] = {37, 4, 101, 31, button_panel_menu},
};

static image_button panel_button_pause[] = {
    {4, 4, 31, 31, IB_NORMAL, 0, 0, button_toggle_pause, button_none, 0, 0, 1, PANEL_ASSETS, "pause"},
};

static image_button panel_buttons_advisors_empire[] = {
    {140, 70, 82, 31, IB_NORMAL, 0, 0, button_panel_advisors, button_none, 0, 0, 1, PANEL_ASSETS, "advisors"},
    {224, 70, 82, 31, IB_NORMAL, 0, 0, button_empire, button_none, 0, 0, 1, PANEL_ASSETS, "empire"},
};

static image_button panel_buttons_build[] = {
    {140, 103, 54, 31, IB_NORMAL, 0, 0, button_build, button_none, BUILD_MENU_VACANT_HOUSE, 0, 1, PANEL_ASSETS, "housing"},
    {196, 103, 54, 31, IB_NORMAL, 0, 0, button_build, button_none, BUILD_MENU_CLEAR_LAND, 0, 1, PANEL_ASSETS, "clear-land"},
    {252, 103, 54, 31, IB_NORMAL, 0, 0, button_build, button_none, BUILD_MENU_ROAD, 0, 1, PANEL_ASSETS, "roads"},
    {140, 136, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_WATER, 0, 1, PANEL_ASSETS, "water"},
    {196, 136, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_HEALTH, 0, 1, PANEL_ASSETS, "health"},
    {252, 136, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_TEMPLES, 0, 1, PANEL_ASSETS, "temples"},
    {140, 169, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_EDUCATION, 0, 1, PANEL_ASSETS, "education"},
    {196, 169, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_ENTERTAINMENT, 0, 1, PANEL_ASSETS, "entertainment"},
    {252, 169, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_ADMINISTRATION, 0, 1, PANEL_ASSETS, "administration"},
    {140, 202, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_ENGINEERING, 0, 1, PANEL_ASSETS, "engineering"},
    {196, 202, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_SECURITY, 0, 1, PANEL_ASSETS, "security"},
    {252, 202, 54, 31, IB_BUILD, 0, 0, button_build, button_none, BUILD_MENU_INDUSTRY, 0, 1, PANEL_ASSETS, "industry"},
};

static image_button panel_buttons_tools[] = {
    {140, 235, 54, 31, IB_NORMAL, 0, 0, button_undo, button_none, 0, 0, 1, PANEL_ASSETS, "undo"},
    {196, 235, 54, 31, IB_NORMAL, 0, 0, button_messages, button_none, 0, 0, 1, PANEL_ASSETS, "messages"},
    {252, 235, 54, 31, IB_BUILD, 0, 0, button_go_to_problem, button_none, 0, 0, 1, PANEL_ASSETS, "go-to-problem"},
};

static image_button panel_buttons_views[] = {
    {4, 235, 32, 31, IB_NORMAL, 0, 0, button_toggle_grid, button_none, 0, 0, 1, PANEL_ASSETS, "grid"},
    {38, 235, 32, 31, IB_NORMAL, 0, 0, button_rotate_north, button_none, 0, 0, 1, PANEL_ASSETS, "rotate-north"},
    {72, 235, 32, 31, IB_NORMAL, 0, 0, button_rotate, button_none, 0, 0, 1, PANEL_ASSETS, "rotate-left"},
    {106, 235, 32, 31, IB_NORMAL, 0, 0, button_rotate, button_none, 1, 0, 1, PANEL_ASSETS, "rotate-right"},
};

#define MAX_PANEL_TARGETS 32

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const image_button *image;
    const generic_button *text;
} panel_target;

static struct {
    int focus_button_for_tooltip;
    int is_dragging_minimap;
    struct {
        int is_active;
        const void *button;
    } panel_focus;
    struct {
        int is_active;
        int x;
        int y;
    } glide_target;
} data;

static void draw_overlay_text(int x_offset)
{
    if (game_state_overlay()) {
        const uint8_t *text = get_current_overlay_text();
        text_draw_centered(text, x_offset, 32, 117, FONT_NORMAL_GREEN, 0);
    } else {
        lang_text_draw_centered(6, 4, x_offset, 32, 117, FONT_NORMAL_GREEN);
    }
}

static void draw_sidebar_remainder(int x_offset, int is_collapsed)
{
    int width = SIDEBAR_EXPANDED_WIDTH;
    if (is_collapsed) {
        width = SIDEBAR_COLLAPSED_WIDTH;
    }
    int available_height = sidebar_common_get_height() - SIDEBAR_MAIN_SECTION_HEIGHT;
    int extra_height = sidebar_extra_draw_background(x_offset, SIDEBAR_FILLER_Y_OFFSET,
        width, available_height, is_collapsed, SIDEBAR_EXTRA_DISPLAY_ALL);
    sidebar_extra_draw_foreground();
    int relief_y_offset = SIDEBAR_FILLER_Y_OFFSET + extra_height;
    sidebar_common_draw_relief(x_offset, relief_y_offset, GROUP_SIDE_PANEL, is_collapsed);
}

static int has_messages_to_show(void)
{
    return game_campaign_is_original() || city_message_count() > 0 || scenario_intro_message();
}

static void draw_message_count(int right_x, int y)
{
    int messages = city_message_count();
    int x = right_x - text_get_number_width(messages, '@', "", FONT_SMALL_PLAIN);
    text_draw_number(messages, '@', "", x, y - 1, FONT_SMALL_PLAIN, COLOR_BLACK); //top
    text_draw_number(messages, '@', "", x, y + 1, FONT_SMALL_PLAIN, COLOR_BLACK); //bottom
    text_draw_number(messages, '@', "", x - 1, y, FONT_SMALL_PLAIN, COLOR_BLACK); //left
    text_draw_number(messages, '@', "", x + 1, y, FONT_SMALL_PLAIN, COLOR_BLACK); //right
    text_draw_number(messages, '@', "", x, y, FONT_SMALL_PLAIN, COLOR_WHITE);
}

static void draw_number_of_messages(int x_offset)
{
    int show_messages = has_messages_to_show();
    buttons_build_expanded[13].enabled = show_messages;
    buttons_build_expanded[14].enabled = city_message_problem_area_count();
    if (show_messages) {
        draw_message_count(x_offset + 100, 453);
    }
}

static void draw_buttons_collapsed(int x_offset)
{
    int asclepius = config_get(CONFIG_UI_DRAW_ASCLEPIUS);
    buttons_build_collapsed[4].assetlist_name = asclepius ? "UI" : 0;
    buttons_build_collapsed[4].image_name = asclepius ? "Asclepius Button" : 0;
    image_buttons_draw(x_offset, 24, button_expand_sidebar, 1);
    image_buttons_draw(x_offset, 24, buttons_build_collapsed, 12);
}

static void draw_buttons_expanded(int x_offset)
{
    int asclepius = config_get(CONFIG_UI_DRAW_ASCLEPIUS);
    buttons_build_expanded[4].assetlist_name = asclepius ? "UI" : 0;
    buttons_build_expanded[4].image_name = asclepius ? "Asclepius Button" : 0;
    buttons_build_expanded[12].enabled = game_can_undo();
    image_buttons_draw(x_offset, 24, buttons_overlays_collapse_sidebar, 2);
    image_buttons_draw(x_offset, 24, buttons_build_expanded, 15);
    image_buttons_draw(x_offset, 24, buttons_top_expanded, 6);
}

static void draw_collapsed_background(void)
{
    int x_offset = sidebar_common_get_x_offset_collapsed();
    image_draw(image_group(GROUP_SIDE_PANEL), x_offset, 24, COLOR_MASK_NONE, SCALE_NONE);
    draw_buttons_collapsed(x_offset);
    draw_sidebar_remainder(x_offset, 1);
}

static void draw_expanded_background(int x_offset)
{
    image_draw(image_group(GROUP_SIDE_PANEL) + 1, x_offset, 24, COLOR_MASK_NONE, SCALE_NONE);
    draw_buttons_expanded(x_offset);
    draw_overlay_text(x_offset + 4);
    draw_number_of_messages(x_offset);
    image_draw(window_build_menu_image(), x_offset + 6, 239, COLOR_MASK_NONE, SCALE_NONE);
    widget_minimap_update(0);
    widget_minimap_draw_decorated(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT);

    draw_sidebar_remainder(x_offset, 0);
}

void widget_sidebar_city_draw_background(void)
{
    if (city_view_is_sidebar_collapsed()) {
        draw_collapsed_background();
    } else {
        draw_expanded_background(sidebar_common_get_x_offset_expanded());
    }
}

static void enable_building_buttons(void)
{
    for (int i = 0; i < 12; i++) {
        buttons_build_expanded[i].enabled = 1;
        if (building_menu_count_items(buttons_build_expanded[i].parameter1) <= 0) {
            buttons_build_expanded[i].enabled = 0;
        }

        buttons_build_collapsed[i].enabled = 1;
        if (building_menu_count_items(buttons_build_collapsed[i].parameter1) <= 0) {
            buttons_build_collapsed[i].enabled = 0;
        }

        panel_buttons_build[i].enabled = building_menu_count_items(panel_buttons_build[i].parameter1) > 0;
    }
}

void widget_sidebar_city_draw_foreground(void)
{
    if (building_menu_has_changed()) {
        enable_building_buttons();
    }
    if (city_view_is_sidebar_collapsed()) {
        int x_offset = sidebar_common_get_x_offset_collapsed();
        draw_buttons_collapsed(x_offset);
    } else {
        int x_offset = sidebar_common_get_x_offset_expanded();
        draw_buttons_expanded(x_offset);
        draw_overlay_text(x_offset + 4);
        widget_minimap_draw_decorated(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT);
        draw_number_of_messages(x_offset);
    }
    sidebar_extra_draw_foreground();
}

int widget_sidebar_city_handle_mouse(const mouse *m)
{
    if (widget_city_has_input()) {
        return 0;
    }
    int handled = 0;
    unsigned int button_id;
    data.focus_button_for_tooltip = 0;
    if (city_view_is_sidebar_collapsed()) {
        int x_offset = sidebar_common_get_x_offset_collapsed();
        handled |= image_buttons_handle_mouse(m, x_offset, 24, button_expand_sidebar, 1, &button_id);
        if (button_id) {
            data.focus_button_for_tooltip = 12;
        }
        handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_build_collapsed, 12, &button_id);
        if (button_id) {
            data.focus_button_for_tooltip = button_id + 19;
        }
    } else {
        if (widget_minimap_handle_mouse(m)) {
            return 1;
        }
        int x_offset = sidebar_common_get_x_offset_expanded();
        handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_overlays_collapse_sidebar, 2, &button_id);
        if (button_id) {
            data.focus_button_for_tooltip = button_id + 9;
        }
        handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_build_expanded, 15, &button_id);
        if (button_id) {
            data.focus_button_for_tooltip = button_id + 19;
        }
        handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_top_expanded, 6, &button_id);
        if (button_id) {
            data.focus_button_for_tooltip = button_id + 39;
        }
        handled |= sidebar_extra_handle_mouse(m);
    }
    return handled;
}

static int get_button_image_id(const image_button *button)
{
    if (button->assetlist_name) {
        return assets_get_image_id(button->assetlist_name, button->image_name);
    }
    return button->image_collection ? image_group(button->image_collection) + button->image_offset :
        button->image_offset;
}

static int is_panel_button_pressed(const image_button *button)
{
    return button->pressed && time_get_millis() - button->pressed_since < PANEL_PRESSED_MILLIS;
}

static int get_button_state_image_id(const image_button *button)
{
    int image_id = get_button_image_id(button);
    if (!button->enabled) {
        return image_id + 3;
    }
    if (is_panel_button_pressed(button)) {
        return image_id + 2;
    }
    return image_id;
}

static void draw_panel_image(int image_id, int x, int y)
{
    image_draw(image_id, (int) (x * PANEL_IMAGE_SCALE), (int) (y * PANEL_IMAGE_SCALE), COLOR_MASK_NONE,
        PANEL_IMAGE_SCALE);
}

static void draw_panel_buttons(const image_button *buttons, int count)
{
    for (int i = 0; i < count; i++) {
        draw_panel_image(get_button_state_image_id(&buttons[i]), buttons[i].x_offset, buttons[i].y_offset);
    }
}

static void draw_panel_stone(int x, int y, int width, int height)
{
    int image_id = assets_get_image_id(PANEL_ASSETS, "gray-stone");
    int tile_size = (int) (image_get(image_id)->width / PANEL_IMAGE_SCALE);
    graphics_set_clip_rectangle(x, y, width, height);
    for (int tile_y = y; tile_y < y + height; tile_y += tile_size) {
        for (int tile_x = x; tile_x < x + width; tile_x += tile_size) {
            draw_panel_image(image_id, tile_x, tile_y);
        }
    }
    graphics_reset_clip_rectangle();
}

static void draw_panel_plate(int x, int y, int width, int height, int is_highlighted)
{
    draw_panel_stone(x, y, width, height);
    large_label_draw_border(x, y, width, height);
    if (is_highlighted) {
        graphics_light_up_rect(x, y, width, height, 3);
    }
}

static void draw_panel_plate_button(const image_button *button)
{
    draw_panel_plate(button->x_offset, button->y_offset, button->width, button->height,
        is_panel_button_pressed(button));
    draw_panel_image(get_button_image_id(button), button->x_offset, button->y_offset);
}

static int get_centered_y(int y, int height, int content_height)
{
    return y + (height - content_height) / 2;
}

static int draw_panel_list_arrow(int x, int y, int width, int height)
{
    int image_id = assets_get_image_id("UI", "Expand Menu Icon");
    const image *img = image_get(image_id);
    int arrow_x = x + width - PANEL_TEXT_MARGIN - img->width;
    int arrow_y = get_centered_y(y, height, img->height);
    image_draw(image_id, arrow_x - img->x_offset, arrow_y - img->y_offset, COLOR_MASK_NONE, SCALE_NONE);
    return img->width + PANEL_TEXT_MARGIN;
}

static void draw_panel_item_text(const generic_button *button, const panel_list_item *item, int label_width,
    int offset, color_t color)
{
    int x = button->x + offset;
    int text_y = get_centered_y(button->y, button->height, PANEL_TEXT_HEIGHT) + offset;
    if (item->value) {
        int value_width = text_get_width(item->value, PANEL_FONT);
        text_draw(item->value, x + button->width - PANEL_TEXT_MARGIN - value_width, text_y, PANEL_FONT, color);
    }
    if (item->is_label_centered) {
        text_draw_centered_ellipsized(item->label, x + PANEL_TEXT_MARGIN, text_y, label_width, PANEL_FONT, color);
    } else {
        text_draw_ellipsized(item->label, x + PANEL_TEXT_MARGIN, text_y, label_width, PANEL_FONT, color);
    }
}

static void draw_panel_item(const generic_button *button, const panel_list_item *item)
{
    draw_panel_plate(button->x, button->y, button->width, button->height, item->is_highlighted);
    int label_width = button->width - 2 * PANEL_TEXT_MARGIN;
    if (item->value) {
        label_width -= text_get_width(item->value, PANEL_FONT) + PANEL_TEXT_MARGIN;
    } else if (item->has_submenu) {
        label_width -= draw_panel_list_arrow(button->x, button->y, button->width, button->height);
    }
    draw_panel_item_text(button, item, label_width, PANEL_TEXT_CARVING_OFFSET, PANEL_TEXT_CARVING_COLOR);
    draw_panel_item_text(button, item, label_width, 0, PANEL_TEXT_COLOR);
}

static void draw_panel_text_button(const generic_button *button, const uint8_t *text)
{
    const panel_list_item item = { .label = text, .is_label_centered = 1 };
    draw_panel_item(button, &item);
}

static void draw_panel_ratings_text(const generic_button *button, int offset, color_t color)
{
    const int ratings[] = {
        city_rating_culture(), city_rating_prosperity(), city_rating_peace(), city_rating_favor()
    };
    int x = button->x + offset;
    int text_height = 3 * PANEL_RATINGS_LINE_HEIGHT + PANEL_TEXT_HEIGHT;
    int first_line_y = get_centered_y(button->y, button->height, text_height) + offset;
    for (int i = 0; i < 4; i++) {
        int text_y = first_line_y + PANEL_RATINGS_LINE_HEIGHT * i;
        lang_text_draw_colored(53, i + 1, x + PANEL_TEXT_MARGIN, text_y, PANEL_FONT, color);
        int number_width = text_get_number_width(ratings[i], '@', "", PANEL_FONT);
        text_draw_number(ratings[i], '@', "", x + button->width - PANEL_TEXT_MARGIN - number_width, text_y,
            PANEL_FONT, color);
    }
}

static void draw_panel_ratings(const generic_button *button)
{
    draw_panel_plate(button->x, button->y, button->width, button->height, 0);
    draw_panel_ratings_text(button, PANEL_TEXT_CARVING_OFFSET, PANEL_TEXT_CARVING_COLOR);
    draw_panel_ratings_text(button, 0, PANEL_TEXT_COLOR);
}

static void draw_panel_number_box(int x, int y, int width, int height)
{
    graphics_fill_rect(x + 1, y + 1, width - 2, height - 2, PANEL_BOX_COLOR);
    graphics_draw_line(x + 1, x + width - 2, y, y, PANEL_BOX_BORDER_COLOR);
    graphics_draw_line(x + 1, x + width - 2, y + height - 1, y + height - 1, PANEL_BOX_BORDER_COLOR);
    graphics_draw_line(x, x, y + 1, y + height - 2, PANEL_BOX_BORDER_COLOR);
    graphics_draw_line(x + width - 1, x + width - 1, y + 1, y + height - 2, PANEL_BOX_BORDER_COLOR);
}

static void draw_panel_number(const generic_button *box, int label_id, int value, color_t color)
{
    draw_panel_number_box(box->x, box->y, box->width, box->height);
    int width = lang_text_get_width(6, label_id, PANEL_FONT) +
        text_get_number_width(value, '@', "", PANEL_FONT);
    int x = box->x + (box->width - width) / 2;
    int y = get_centered_y(box->y, box->height, PANEL_FONT_CAP_HEIGHT);
    x += lang_text_draw_colored(6, label_id, x, y, PANEL_FONT, color);
    text_draw_number(value, '@', "", x, y, PANEL_FONT, color);
}

static int get_month_year_width(int month, int year)
{
    int width = lang_text_get_width(25, month, PANEL_FONT) +
        text_get_number_width(year >= 0 ? year : -year, ' ', " ", PANEL_FONT) +
        lang_text_get_width(20, year >= 0 ? 1 : 0, PANEL_FONT);
    if (year >= 0 && !locale_year_before_ad()) {
        width += font_definition_for(PANEL_FONT)->space_width;
    }
    return width;
}

static void draw_panel_date(const panel_area *box)
{
    draw_panel_number_box(box->x, box->y, box->width, box->height);
    int day = widget_top_menu_get_cosmetic_day_of_month();
    int month = game_time_month();
    int year = game_time_year();
    int day_width = text_get_number_width(day, '@', "", PANEL_FONT) +
        font_definition_for(PANEL_FONT)->space_width;
    int month_year_width = get_month_year_width(month, year);
    int x = box->x + (box->width - day_width - month_year_width) / 2;
    int y = get_centered_y(box->y, box->height, PANEL_FONT_CAP_HEIGHT);
    text_draw_number(day, '@', "", x, y, PANEL_FONT, COLOR_FONT_YELLOW);
    lang_text_draw_month_year_max_width(month, year, x + day_width, y, box->width, PANEL_FONT,
        COLOR_FONT_YELLOW);
}

static void draw_panel_top_bar(void)
{
    draw_panel_date(&panel_date);
    int treasury = city_finance_treasury();
    draw_panel_number(&panel_text_buttons[PANEL_BUTTON_FUNDS], 0, treasury,
        treasury < 0 ? COLOR_FONT_RED : COLOR_WHITE);
    draw_panel_number(&panel_text_buttons[PANEL_BUTTON_POPULATION], 1, city_population(), COLOR_WHITE);
}

static void update_panel_buttons(void)
{
    if (building_menu_has_changed()) {
        enable_building_buttons();
    }
    panel_buttons_tools[0].enabled = game_can_undo();
    panel_buttons_tools[1].enabled = has_messages_to_show();
    panel_buttons_tools[2].enabled = city_message_problem_area_count();
    panel_button_pause[0].image_name = game_state_is_paused() ? "play" : "pause";
}

static void draw_panel_message_count(const image_button *button)
{
    int messages = city_message_count();
    int is_large = messages <= PANEL_MESSAGE_COUNT_LARGE_MAX;
    font_t font = is_large ? PANEL_MESSAGE_COUNT_FONT : PANEL_FONT;
    int cap_height = is_large ? PANEL_MESSAGE_COUNT_CAP_HEIGHT : PANEL_FONT_CAP_HEIGHT;
    uint8_t count[12];
    string_from_int(count, messages, 0);
    int x = button->x_offset + PANEL_MESSAGE_COUNT_CENTER_X - text_get_width(count, font) / 2;
    int y = button->y_offset + PANEL_MESSAGE_COUNT_CENTER_Y - cap_height / 2;
    text_draw(count, x, y - 1, font, COLOR_BLACK);
    text_draw(count, x, y + 1, font, COLOR_BLACK);
    text_draw(count, x - 1, y, font, COLOR_BLACK);
    text_draw(count, x + 1, y, font, COLOR_BLACK);
    text_draw(count, x, y, font, COLOR_WHITE);
}

static int add_image_targets(panel_target *targets, int count, const image_button *buttons, int num_buttons)
{
    for (int i = 0; i < num_buttons; i++) {
        if (buttons[i].enabled) {
            targets[count++] = (panel_target) { buttons[i].x_offset, buttons[i].y_offset, buttons[i].width,
                buttons[i].height, &buttons[i], 0 };
        }
    }
    return count;
}

static int get_panel_targets(panel_target *targets)
{
    int count = add_image_targets(targets, 0, panel_button_pause, 1);
    for (int i = 0; i < PANEL_TEXT_BUTTONS; i++) {
        const generic_button *button = &panel_text_buttons[i];
        targets[count++] = (panel_target) { button->x, button->y, button->width, button->height, 0, button };
    }
    count = add_image_targets(targets, count, panel_buttons_advisors_empire, 2);
    count = add_image_targets(targets, count, panel_buttons_build, 12);
    count = add_image_targets(targets, count, panel_buttons_views, 4);
    return add_image_targets(targets, count, panel_buttons_tools, 3);
}

static const void *get_target_button(const panel_target *target)
{
    return target->image ? (const void *) target->image : (const void *) target->text;
}

static int get_focused_target(const panel_target *targets, int count)
{
    for (int i = 0; i < count; i++) {
        if (get_target_button(&targets[i]) == data.panel_focus.button) {
            return i;
        }
    }
    for (int i = 0; i < count; i++) {
        if (targets[i].image == &panel_buttons_build[0]) {
            return i;
        }
    }
    return 0;
}

static void draw_panel_focus(void)
{
    if (!data.panel_focus.is_active) {
        return;
    }
    panel_target targets[MAX_PANEL_TARGETS];
    int count = get_panel_targets(targets);
    const panel_target *target = &targets[get_focused_target(targets, count)];
    graphics_light_up_rect(target->x, target->y, target->width, target->height, 3);
}

void widget_sidebar_city_draw_panel(void)
{
    second_screen_draw_marble_background();
    update_panel_buttons();
    draw_panel_top_bar();
    draw_panel_ratings(&panel_text_buttons[PANEL_BUTTON_RATINGS]);
    draw_panel_text_button(&panel_text_buttons[PANEL_BUTTON_OVERLAY],
        game_state_overlay() ? get_current_overlay_text() : lang_get_string(6, 4));
    widget_minimap_draw_decorated(panel_minimap.x + 1, panel_minimap.y + 1, panel_minimap.width - 2,
        panel_minimap.height - 1);
    draw_panel_buttons(panel_buttons_views, 4);
    draw_panel_text_button(&panel_text_buttons[PANEL_BUTTON_MENU], translation_for(TR_BUTTON_PANEL_MENU));
    draw_panel_plate_button(&panel_button_pause[0]);
    draw_panel_buttons(panel_buttons_advisors_empire, 2);
    draw_panel_buttons(panel_buttons_build, 12);
    draw_panel_buttons(panel_buttons_tools, 3);
    if (panel_buttons_tools[1].enabled) {
        draw_panel_message_count(&panel_buttons_tools[1]);
    }
    draw_panel_focus();
}

void widget_sidebar_city_draw_panel_background(void)
{
    widget_minimap_update(0);
    widget_sidebar_city_draw_panel();
}

static int is_in_rect(const mouse *m, int x, int y, int width, int height)
{
    return m->x >= x && m->x < x + width && m->y >= y && m->y < y + height;
}

static int get_glide_step(int distance)
{
    int step = distance * MINIMAP_GLIDE_PERCENT / 100;
    if (step == 0 && distance != 0) {
        step = distance > 0 ? 1 : -1;
    }
    return step;
}

void widget_sidebar_city_glide_camera(void)
{
    if (!data.glide_target.is_active) {
        return;
    }
    int x, y;
    city_view_get_camera_in_pixels(&x, &y);
    city_view_scroll(get_glide_step(data.glide_target.x - x), get_glide_step(data.glide_target.y - y));
    int new_x, new_y;
    city_view_get_camera_in_pixels(&new_x, &new_y);
    if (new_x == x && new_y == y) {
        data.glide_target.is_active = 0;
    }
}

static int glide_to_minimap_point(const mouse *m)
{
    int x, y;
    city_view_get_camera_in_pixels(&x, &y);
    if (!widget_minimap_handle_mouse(m)) {
        return 0;
    }
    city_view_get_camera_in_pixels(&data.glide_target.x, &data.glide_target.y);
    city_view_scroll(x - data.glide_target.x, y - data.glide_target.y);
    data.glide_target.is_active = 1;
    return 1;
}

static int handle_panel_minimap(const mouse *m)
{
    if (m->left.went_down) {
        data.is_dragging_minimap = glide_to_minimap_point(m);
        return data.is_dragging_minimap;
    }
    if (!m->left.is_down) {
        int was_dragging = data.is_dragging_minimap;
        data.is_dragging_minimap = 0;
        return was_dragging || widget_minimap_handle_mouse(m);
    }
    if (!data.is_dragging_minimap) {
        return 0;
    }
    mouse drag = *m;
    drag.left.went_down = 1;
    glide_to_minimap_point(&drag);
    return 1;
}

int widget_sidebar_city_handle_panel_mouse(const mouse *m)
{
    if (m->left.went_down) {
        data.panel_focus.is_active = 0;
    }
    if (handle_panel_minimap(m)) {
        return 1;
    }
    return generic_buttons_handle_mouse(m, 0, 0, panel_text_buttons, PANEL_TEXT_BUTTONS, 0) ||
        image_buttons_handle_mouse(m, 0, 0, panel_buttons_views, 4, 0) ||
        image_buttons_handle_mouse(m, 0, 0, panel_button_pause, 1, 0) ||
        image_buttons_handle_mouse(m, 0, 0, panel_buttons_advisors_empire, 2, 0) ||
        image_buttons_handle_mouse(m, 0, 0, panel_buttons_build, 12, 0) ||
        image_buttons_handle_mouse(m, 0, 0, panel_buttons_tools, 3, 0);
}

static int get_target_in_direction(const panel_target *targets, int count, int from, int dx, int dy)
{
    int from_x = targets[from].x + targets[from].width / 2;
    int from_y = targets[from].y + targets[from].height / 2;
    int best = from;
    int best_distance = 0;
    for (int i = 0; i < count; i++) {
        int x = targets[i].x + targets[i].width / 2 - from_x;
        int y = targets[i].y + targets[i].height / 2 - from_y;
        int along = x * dx + y * dy;
        if (along <= 0) {
            continue;
        }
        int distance = along + 2 * abs(x * dy - y * dx);
        if (best == from || distance < best_distance) {
            best = i;
            best_distance = distance;
        }
    }
    return best;
}

static void activate_panel_target(const panel_target *target)
{
    if (target->image) {
        target->image->left_click_handler(target->image->parameter1, target->image->parameter2);
    } else {
        target->text->left_click_handler(target->text);
    }
}

int widget_sidebar_city_handle_panel_focus(const mouse *m, const hotkeys *h)
{
    if (!second_screen_is_active()) {
        data.panel_focus.is_active = 0;
        return 0;
    }
    if (h->focus_panel) {
        data.panel_focus.is_active = !data.panel_focus.is_active;
        return 1;
    }
    if (!data.panel_focus.is_active) {
        return 0;
    }
    if (m->is_touch && m->left.went_down) {
        data.panel_focus.is_active = 0;
        return 0;
    }
    panel_target targets[MAX_PANEL_TARGETS];
    int count = get_panel_targets(targets);
    int focus = get_focused_target(targets, count);
    int dx = h->cycle_overlay;
    int dy = h->decrease_game_speed - h->increase_game_speed;
    if (dx || dy) {
        data.panel_focus.button = get_target_button(&targets[get_target_in_direction(targets, count, focus, dx, dy)]);
        return 1;
    }
    if (m->right.went_up) {
        data.panel_focus.is_active = 0;
        return 1;
    }
    if (m->left.went_up) {
        activate_panel_target(&targets[focus]);
        return 1;
    }
    return m->left.went_down || m->left.is_down || m->right.went_down || m->right.is_down;
}

static int is_on_panel_button(const mouse *m, const image_button *button)
{
    return is_in_rect(m, button->x_offset, button->y_offset, button->width, button->height);
}

static int is_on_button_of_shown_window(const mouse *m)
{
    if (window_is(WINDOW_EMPIRE)) {
        return is_on_panel_button(m, &panel_buttons_advisors_empire[1]);
    }
    if (!window_is(WINDOW_ADVISORS)) {
        return 0;
    }
    if (is_on_panel_button(m, &panel_buttons_advisors_empire[0])) {
        return 1;
    }
    for (int i = 0; i < PANEL_TEXT_BUTTONS; i++) {
        const generic_button *button = &panel_text_buttons[i];
        if (button->left_click_handler == button_panel_advisor &&
            button->parameter1 == (int) window_advisors_get_advisor() &&
            is_in_rect(m, button->x, button->y, button->width, button->height)) {
            return 1;
        }
    }
    return 0;
}

void widget_sidebar_city_handle_panel_mouse_over_window(const mouse *m)
{
    if (!m->left.went_up) {
        return;
    }
    int closes_window = is_on_button_of_shown_window(m);
    window_city_show();
    if (!closes_window) {
        widget_sidebar_city_handle_panel_mouse(m);
    }
}

static int get_panel_list_width(void)
{
    return second_screen_width() - 2 * PANEL_LIST_MARGIN;
}

static int get_panel_list_height(void)
{
    return second_screen_height() - PANEL_LIST_MARGIN - PANEL_LIST_TOP;
}

static generic_button get_panel_list_back_row(void)
{
    generic_button row = { PANEL_LIST_MARGIN, PANEL_LIST_MARGIN, get_panel_list_width(), PANEL_LIST_ROW_HEIGHT };
    return row;
}

void widget_sidebar_city_draw_panel_list_frame(void)
{
    second_screen_draw_marble_background();
    generic_button back_row = get_panel_list_back_row();
    draw_panel_text_button(&back_row, lang_get_string(12, 0));
}

static int get_panel_list_columns(int count)
{
    int max_rows = (get_panel_list_height() + PANEL_LIST_ROW_GAP) / (PANEL_LIST_ROW_HEIGHT + PANEL_LIST_ROW_GAP);
    return count > max_rows ? (count + max_rows - 1) / max_rows : 1;
}

static int get_panel_list_rows(int count)
{
    int columns = get_panel_list_columns(count);
    return (count + columns - 1) / columns;
}

static void place_panel_list_item(int index, int count, generic_button *button)
{
    int columns = get_panel_list_columns(count);
    int rows = get_panel_list_rows(count);
    button->width = (get_panel_list_width() - (columns - 1) * PANEL_LIST_COLUMN_GAP) / columns;
    button->height = PANEL_LIST_ROW_HEIGHT;
    button->x = PANEL_LIST_MARGIN + (index / rows) * (button->width + PANEL_LIST_COLUMN_GAP);
    button->y = PANEL_LIST_TOP + (index % rows) * (PANEL_LIST_ROW_HEIGHT + PANEL_LIST_ROW_GAP);
}

void widget_sidebar_city_draw_panel_list_item(int index, int count, const panel_list_item *item,
    generic_button *button)
{
    place_panel_list_item(index, count, button);
    draw_panel_item(button, item);
}

panel_list_action widget_sidebar_city_handle_panel_list(const mouse *m, generic_button *buttons,
    unsigned int count, unsigned int *focus_button_id)
{
    generic_button back_row = get_panel_list_back_row();
    if (m->left.went_up && is_in_rect(m, back_row.x, back_row.y, back_row.width, back_row.height)) {
        return PANEL_LIST_BACK;
    }
    if (generic_buttons_handle_mouse(m, 0, 0, buttons, count, focus_button_id)) {
        return PANEL_LIST_NO_ACTION;
    }
    int is_in_list = is_in_rect(m, PANEL_LIST_MARGIN, PANEL_LIST_TOP, get_panel_list_width(),
        get_panel_list_height());
    return m->left.went_up && !is_in_list ? PANEL_LIST_OUTSIDE : PANEL_LIST_NO_ACTION;
}

static void move_panel_list_focus(int step, unsigned int count, unsigned int *focus_button_id)
{
    if (!*focus_button_id) {
        *focus_button_id = step > 0 ? 1 : count;
        return;
    }
    int focus = (int) *focus_button_id + step;
    if (focus >= 1 && focus <= (int) count) {
        *focus_button_id = focus;
    }
}

static int get_panel_list_scroll_step(const mouse *m)
{
    static time_millis last_step_time;
    if (m->scrolled == SCROLL_NONE || time_get_millis() - last_step_time < PANEL_LIST_SCROLL_STEP_MILLIS) {
        return 0;
    }
    last_step_time = time_get_millis();
    return m->scrolled == SCROLL_DOWN ? 1 : -1;
}

int widget_sidebar_city_handle_panel_list_controller(const mouse *m, const hotkeys *h, generic_button *buttons,
    unsigned int count, unsigned int *focus_button_id)
{
    int step = get_panel_list_scroll_step(m);
    if (!step && h->cycle_overlay) {
        step = h->cycle_overlay * get_panel_list_rows(count);
    }
    if (step) {
        move_panel_list_focus(step, count, focus_button_id);
        window_request_refresh();
        return 1;
    }
    if (*focus_button_id && m->left.went_up && !m->is_touch) {
        generic_button *button = &buttons[*focus_button_id - 1];
        button->left_click_handler(button);
        return 1;
    }
    return 0;
}

int widget_sidebar_city_handle_mouse_build_menu(const mouse *m)
{
    if (city_view_is_sidebar_collapsed()) {
        return image_buttons_handle_mouse(m,
            sidebar_common_get_x_offset_collapsed(), 24, buttons_build_collapsed, 12, 0);
    } else {
        return image_buttons_handle_mouse(m,
            sidebar_common_get_x_offset_expanded(), 24, buttons_build_expanded, 15, 0);
    }
}

int widget_sidebar_city_get_tooltip_text(tooltip_context *c)
{
    if (data.focus_button_for_tooltip) {
        if (data.focus_button_for_tooltip == 42) {
            c->translation_key = TR_TOGGLE_GRID;
            return 0;
        }
        return data.focus_button_for_tooltip;
    }
    return sidebar_extra_get_tooltip(c);
}

static void slide_finished(void)
{
    city_view_toggle_sidebar();
    window_city_show();
}

static void button_overlay(int param1, int param2)
{
    window_overlay_menu_show();
}

static void button_collapse_expand(int param1, int param2)
{
    city_view_start_sidebar_toggle();
    sidebar_slide(!city_view_is_sidebar_collapsed(),
        draw_collapsed_background, draw_expanded_background, slide_finished);
}

static void button_build(int submenu, int param2)
{
    window_build_menu_show(submenu);
}

static void button_undo(int param1, int param2)
{
    window_build_menu_hide();
    game_undo_perform();
    window_invalidate();
}

static void button_messages(int param1, int param2)
{
    window_build_menu_hide();
    window_message_list_show();
}

static void button_help(int param1, int param2)
{
    window_build_menu_hide();
    window_message_dialog_show(param2, window_city_draw_all);
}

static void button_go_to_problem(int param1, int param2)
{
    window_build_menu_hide();
    int grid_offset = city_message_next_problem_area_grid_offset();
    if (grid_offset) {
        city_view_go_to_grid_offset(grid_offset);
        window_city_show();
    } else {
        window_invalidate();
    }
}

static void button_advisors(int param1, int param2)
{
    window_advisors_show_checked();
}

static void button_empire(int param1, int param2)
{
    window_empire_show_checked();
}

static void button_toggle_grid(int param1, int param2)
{
    config_set(CONFIG_UI_SHOW_GRID, config_get(CONFIG_UI_SHOW_GRID) ^ 1);
}

static void button_rotate_north(int param1, int param2)
{
    game_orientation_rotate_north();
    window_invalidate();
}

static void button_rotate(int clockwise, int param2)
{
    if (clockwise) {
        game_orientation_rotate_right();
    } else {
        game_orientation_rotate_left();
    }
    window_invalidate();
}

static void button_panel_overlay(const generic_button *button)
{
    window_overlay_menu_show();
}

static void button_panel_menu(const generic_button *button)
{
    window_build_menu_hide();
    widget_top_menu_show_panel_menu();
}

static void button_panel_advisors(int param1, int param2)
{
    window_build_menu_hide();
    window_advisors_show_checked();
}

static void button_panel_advisor(const generic_button *button)
{
    window_build_menu_hide();
    window_advisors_show_advisor(button->parameter1);
}

static void button_toggle_pause(int param1, int param2)
{
    game_state_toggle_paused();
}
