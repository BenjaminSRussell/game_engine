#include "tab_widget.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <stdio.h>

static void tab_widget_layout(Widget* widget, float available_width, float available_height);
static void tab_widget_destroy(Widget* widget);
static bool on_tab_button_click(Widget* widget, UIEvent* event, void* user_data);

TabWidget* tab_widget_create(const char* name) {
    TabWidget* tabs = (TabWidget*)memory_alloc(sizeof(TabWidget));
    if (!tabs) {
        LOG_ERROR(LOG_CAT_UI, "Failed to allocate TabWidget");
        return NULL;
    }

    if (!widget_init(&tabs->widget, name)) {
        memory_free(tabs);
        return NULL;
    }

    tabs->widget.destroy = tab_widget_destroy;
    tabs->widget.layout = tab_widget_layout;

    // Create containers
    tabs->header_container = widget_create("TabHeader");
    tabs->content_container = widget_create("TabContent");

    if (!tabs->header_container || !tabs->content_container) {
        LOG_ERROR(LOG_CAT_UI, "Failed to create tab containers");
        if (tabs->header_container) widget_destroy(tabs->header_container);
        if (tabs->content_container) widget_destroy(tabs->content_container);
        memory_free(tabs);
        return NULL;
    }

    // Set up containers
    tabs->header_height = 30.0f;
    widget_set_size(tabs->header_container, (Vec2){100.0f, tabs->header_height}); // width will be adjusted in layout

    widget_add_child((Widget*)tabs, tabs->header_container);
    widget_add_child((Widget*)tabs, tabs->content_container);

    tabs->active_tab_index = -1;
    tabs->active_tab_color = (Vec4){0.3f, 0.3f, 0.3f, 1.0f};
    tabs->inactive_tab_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};

    return tabs;
}

static void tab_widget_destroy(Widget* widget) {
    // Base destroy handles children (header and content containers)
    // We just need to handle TabWidget specific resources if any
    // Nothing extra to free here as containers are children

    // Note: memory_free(widget) is done by the caller of this destroy function usually?
    // In widget.c, widget_destroy calls widget->destroy then frees children then frees widget.
    // So we don't need to free `widget` here.
}

static void tab_widget_layout(Widget* widget, float available_width, float available_height) {
    TabWidget* tabs = (TabWidget*)widget;

    // Position header
    widget_set_position(tabs->header_container, (Vec2){0, 0});
    widget_set_size(tabs->header_container, (Vec2){available_width, tabs->header_height});

    // Layout header buttons (simple horizontal layout)
    float x_offset = 0;
    for (uint32_t i = 0; i < tabs->header_container->child_count; i++) {
        Widget* btn = tabs->header_container->children[i];
        float btn_width = 100.0f; // Fixed width for now
        widget_set_position(btn, (Vec2){x_offset, 0});
        widget_set_size(btn, (Vec2){btn_width, tabs->header_height});
        x_offset += btn_width + 2.0f; // Padding

        // Trigger layout on button
        if (btn->layout) {
            btn->layout(btn, btn_width, tabs->header_height);
        }
    }

    // Position content
    widget_set_position(tabs->content_container, (Vec2){0, tabs->header_height});
    widget_set_size(tabs->content_container, (Vec2){available_width, available_height - tabs->header_height});

    // Layout active content
    if (tabs->active_tab_index >= 0 && (uint32_t)tabs->active_tab_index < tabs->content_container->child_count) {
        Widget* content = tabs->content_container->children[tabs->active_tab_index];
        widget_set_position(content, (Vec2){0, 0});
        widget_set_size(content, (Vec2){available_width, available_height - tabs->header_height});

        if (content->layout) {
            content->layout(content, available_width, available_height - tabs->header_height);
        }
    }
}

void tab_widget_add_tab(TabWidget* tabs, const char* title, Widget* content) {
    if (!tabs || !content) return;

    // Create header button
    Button* btn = button_create("TabButton", title);
    if (btn) {
        widget_set_background_color((Widget*)btn, tabs->inactive_tab_color);
        button_set_on_click(btn, on_tab_button_click, tabs);
        widget_add_child(tabs->header_container, (Widget*)btn);
    }

    // Add content
    widget_set_visible(content, false); // Hidden by default
    widget_add_child(tabs->content_container, content);

    // If first tab, activate it
    if (tabs->active_tab_index == -1) {
        tab_widget_set_active_tab(tabs, 0);
    }
}

void tab_widget_set_active_tab(TabWidget* tabs, int32_t index) {
    if (!tabs) return;

    if (index < 0 || (uint32_t)index >= tabs->content_container->child_count) return;

    // Update previous tab
    if (tabs->active_tab_index >= 0 && (uint32_t)tabs->active_tab_index < tabs->header_container->child_count) {
        Widget* prev_btn = tabs->header_container->children[tabs->active_tab_index];
        widget_set_background_color(prev_btn, tabs->inactive_tab_color);

        Widget* prev_content = tabs->content_container->children[tabs->active_tab_index];
        widget_set_visible(prev_content, false);
    }

    tabs->active_tab_index = index;

    // Update new tab
    if (tabs->active_tab_index >= 0) {
        Widget* curr_btn = tabs->header_container->children[tabs->active_tab_index];
        widget_set_background_color(curr_btn, tabs->active_tab_color);

        Widget* curr_content = tabs->content_container->children[tabs->active_tab_index];
        widget_set_visible(curr_content, true);
    }

    widget_invalidate_layout((Widget*)tabs);
}

int32_t tab_widget_get_active_tab(const TabWidget* tabs) {
    return tabs ? tabs->active_tab_index : -1;
}

static bool on_tab_button_click(Widget* widget, UIEvent* event, void* user_data) {
    TabWidget* tabs = (TabWidget*)user_data;

    // Find which button this is
    for (uint32_t i = 0; i < tabs->header_container->child_count; i++) {
        if (tabs->header_container->children[i] == widget) {
            tab_widget_set_active_tab(tabs, i);
            return true;
        }
    }
    return false;
}
