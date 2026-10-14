#ifndef UI_TAB_WIDGET_H
#define UI_TAB_WIDGET_H

#include "widget.h"
#include "button.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TabWidget TabWidget;

struct TabWidget {
    Widget widget;

    Widget* header_container;
    Widget* content_container;

    int32_t active_tab_index;

    // Visual properties
    float header_height;
    Vec4 header_background_color;
    Vec4 active_tab_color;
    Vec4 inactive_tab_color;
};

// API
TabWidget* tab_widget_create(const char* name);
void tab_widget_add_tab(TabWidget* tabs, const char* title, Widget* content);
void tab_widget_set_active_tab(TabWidget* tabs, int32_t index);
int32_t tab_widget_get_active_tab(const TabWidget* tabs);

#ifdef __cplusplus
}
#endif

#endif // UI_TAB_WIDGET_H
