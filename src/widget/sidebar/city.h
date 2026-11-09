#ifndef WIDGET_SIDEBAR_CITY_H
#define WIDGET_SIDEBAR_CITY_H

#include "graphics/generic_button.h"
#include "graphics/tooltip.h"
#include "input/hotkey.h"
#include "input/mouse.h"

#include <stdint.h>

typedef struct {
    const uint8_t *label;
    const uint8_t *value;
    int has_submenu;
    int is_label_centered;
    int is_highlighted;
} panel_list_item;

typedef enum {
    PANEL_LIST_NO_ACTION,
    PANEL_LIST_BACK,
    PANEL_LIST_OUTSIDE
} panel_list_action;

void widget_sidebar_city_draw_background(void);
void widget_sidebar_city_draw_foreground(void);

int widget_sidebar_city_handle_mouse(const mouse *m);
int widget_sidebar_city_handle_mouse_build_menu(const mouse *m);

int widget_sidebar_city_get_tooltip_text(tooltip_context *c);

void widget_sidebar_city_draw_panel(void);

/**
 * Moves the camera a step towards the point last touched on the panel minimap, so a drag glides.
 */
void widget_sidebar_city_glide_camera(void);

void widget_sidebar_city_draw_panel_background(void);
int widget_sidebar_city_handle_panel_mouse(const mouse *m);

/**
 * Handles a tap on the panel while a window such as the advisors or the empire map covers the city:
 * the tap closes that window, then acts on the panel unless it hit the button that opened the window.
 */
void widget_sidebar_city_handle_panel_mouse_over_window(const mouse *m);

void widget_sidebar_city_draw_panel_list_frame(void);

/**
 * Draws the item at the given index of a panel list of count items, and places its button.
 * The caller sets the button's click handler and parameters.
 */
void widget_sidebar_city_draw_panel_list_item(int index, int count, const panel_list_item *item,
    generic_button *button);

/**
 * Handles a touch on a panel list. Returns PANEL_LIST_BACK when the back row is tapped,
 * PANEL_LIST_OUTSIDE when a tap lands outside the list, and PANEL_LIST_NO_ACTION otherwise.
 */
panel_list_action widget_sidebar_city_handle_panel_list(const mouse *m, generic_button *buttons,
    unsigned int count, unsigned int *focus_button_id);

/**
 * Lets the controller use the panel: the focus panel button shows a highlight, the d-pad moves it to the next
 * button in that direction, the left click button presses it and the right click button hides it.
 * Returns 1 when the input was used.
 */
int widget_sidebar_city_handle_panel_focus(const mouse *m, const hotkeys *h);

/**
 * Lets the controller use a panel list: the d-pad moves the highlight, across columns with left and right,
 * and the left click button chooses the highlighted item. Returns 1 when the input was used.
 */
int widget_sidebar_city_handle_panel_list_controller(const mouse *m, const hotkeys *h, generic_button *buttons,
    unsigned int count, unsigned int *focus_button_id);

#endif // WIDGET_SIDEBAR_CITY_H
