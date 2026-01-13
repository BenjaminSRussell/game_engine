/*
 * tooltip.c
 * Tooltip System Implementation
 */

#include "tooltip.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <string.h>
#include <stdlib.h>

// System state
static struct {
    Vec4 bg_color;
    Vec4 text_color;
    float delay;

    float timer;
    Widget* current_hovered;
    bool visible;
} g_tooltip_system = {
    .bg_color = {0.1f, 0.1f, 0.1f, 0.9f},
    .text_color = {1.0f, 1.0f, 1.0f, 1.0f},
    .delay = 0.5f,
    .timer = 0.0f,
    .current_hovered = NULL,
    .visible = false
};

void ui_tooltip_set(Widget* widget, const char* text) {
    if (!widget) return;

    if (widget->tooltip_text) {
        free(widget->tooltip_text);
        widget->tooltip_text = NULL;
    }

    if (text) {
        widget->tooltip_text = strdup(text);
    }
}

const char* ui_tooltip_get(const Widget* widget) {
    if (!widget) return NULL;
    return widget->tooltip_text;
}

void ui_tooltip_system_init(void) {
    g_tooltip_system.timer = 0.0f;
    g_tooltip_system.current_hovered = NULL;
    g_tooltip_system.visible = false;
    LOG_INFO(LOG_CAT_GENERAL, "Tooltip system initialized");
}

void ui_tooltip_system_update(float delta_time, Widget* hovered_widget) {
    if (hovered_widget != g_tooltip_system.current_hovered) {
        g_tooltip_system.current_hovered = hovered_widget;
        g_tooltip_system.timer = 0.0f;
        g_tooltip_system.visible = false;
    }

    if (g_tooltip_system.current_hovered) {
        if (g_tooltip_system.current_hovered->tooltip_text) {
            g_tooltip_system.timer += delta_time;

            if (g_tooltip_system.timer >= g_tooltip_system.delay) {
                if (!g_tooltip_system.visible) {
                    g_tooltip_system.visible = true;
                    // Trigger rendering or create a popup widget here
                    // For now we just log it as a proof of concept
                    // LOG_DEBUG(LOG_CAT_GENERAL, "Show tooltip: %s", g_tooltip_system.current_hovered->tooltip_text);
                }
            }
        } else {
            g_tooltip_system.visible = false;
            g_tooltip_system.timer = 0.0f;
        }
    } else {
        g_tooltip_system.visible = false;
        g_tooltip_system.timer = 0.0f;
    }
}

void ui_tooltip_configure(Vec4 bg_color, Vec4 text_color, float delay) {
    g_tooltip_system.bg_color = bg_color;
    g_tooltip_system.text_color = text_color;
    g_tooltip_system.delay = delay;
}
