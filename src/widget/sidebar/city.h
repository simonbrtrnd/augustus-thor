#ifndef WIDGET_SIDEBAR_CITY_H
#define WIDGET_SIDEBAR_CITY_H

#include "graphics/tooltip.h"
#include "input/mouse.h"

void widget_sidebar_city_draw_background(void);
void widget_sidebar_city_draw_foreground(void);

int widget_sidebar_city_handle_mouse(const mouse *m);
int widget_sidebar_city_handle_mouse_build_menu(const mouse *m);

int widget_sidebar_city_get_tooltip_text(tooltip_context *c);

void widget_sidebar_city_draw_panel(void);
void widget_sidebar_city_draw_panel_background(void);
int widget_sidebar_city_handle_panel_mouse(const mouse *m);

void widget_sidebar_city_draw_panel_list_frame(void);
void widget_sidebar_city_draw_panel_list_row(int x, int y, int width, int height, int is_highlighted);
void widget_sidebar_city_get_panel_list_item(int index, int count, int *x, int *y, int *width, int *height);
int widget_sidebar_city_panel_list_back_clicked(const mouse *m);
int widget_sidebar_city_is_in_panel_list(const mouse *m);

#endif // WIDGET_SIDEBAR_CITY_H
