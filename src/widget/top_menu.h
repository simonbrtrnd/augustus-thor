#ifndef WINDOW_TOP_MENU_H
#define WINDOW_TOP_MENU_H

#include "graphics/tooltip.h"
#include "input/hotkey.h"
#include "input/mouse.h"

void widget_top_menu_draw(int force);
int widget_top_menu_handle_input(const mouse *m, const hotkeys *h);
int widget_top_menu_get_tooltip_text(tooltip_context *c);

char widget_top_menu_get_cosmetic_day_of_month(void);

/**
 * Opens the game menu as a list on the panel of the second screen: save, load, then the file, options and help menus.
 */
void widget_top_menu_show_panel_menu(void);

/**
 * Opens the list of advisor reports on the panel of the second screen.
 */
void widget_top_menu_show_panel_reports(void);

#endif // WINDOW_TOP_MENU_H
