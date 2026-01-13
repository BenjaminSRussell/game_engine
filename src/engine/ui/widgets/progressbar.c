/*
 * progressbar.c
 * Progress Bar Widget Implementation
 * Visual indicator of operation progress
 */

#include "progressbar.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static float ui_clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static void ui_progressbar_render_impl(Widget* widget) {
    if (!widget || !widget_is_visible(widget)) return;

    UIProgressBar* bar = (UIProgressBar*)widget;

    float fraction;
    if (bar->max_value > bar->min_value) {
        fraction = (bar->value - bar->min_value) / (bar->max_value - bar->min_value);
    } else {
        fraction = 0.0f;
    }
    fraction = ui_clamp(fraction, 0.0f, 1.0f);

    // TODO: Implement actual rendering
    // 1. Draw background
    // 2. Draw fill rect width = total_width * fraction
    // 3. Draw text if enabled

    if (bar->indeterminate) {
        LOG_DEBUG(LOG_CAT_GENERAL, "ProgressBar render (Indeterminate): time=%.2f", bar->animation_time);
    } else {
        LOG_DEBUG(LOG_CAT_GENERAL, "ProgressBar render: %.1f%%", fraction * 100.0f);
    }
}

static void ui_progressbar_layout_impl(Widget* widget, float available_width, float available_height) {
    if (!widget) return;

    Vec2 size = widget_get_size(widget);

    // Default size
    if (size.x <= 0) size.x = available_width > 0 ? fminf(available_width, 200.0f) : 200.0f;
    if (size.y <= 0) size.y = 24.0f;

    widget_set_size(widget, size);
    widget->needs_layout = false;
}

static Size ui_progressbar_measure_impl(Widget* widget, float available_width, float available_height) {
    return (Size){200.0f, 24.0f};
}

static void ui_progressbar_destroy_impl(Widget* widget) {
    if (!widget) return;
    UIProgressBar* bar = (UIProgressBar*)widget;

    if (bar->format_text) {
        free(bar->format_text);
        bar->format_text = NULL;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

UIProgressBar* ui_progressbar_create(const char* name) {
    UIProgressBar* bar = memory_alloc(sizeof(UIProgressBar));
    if (!bar) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI progress bar");
        return NULL;
    }

    Widget* base = widget_create(name ? name : "ProgressBar");
    if (!base) {
        free(bar);
        return NULL;
    }

    memcpy(bar, base, sizeof(Widget));

    bar->value = 0.0f;
    bar->min_value = 0.0f;
    bar->max_value = 100.0f;
    bar->indeterminate = false;

    // Visual defaults
    bar->background_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    bar->fill_color = (Vec4){0.2f, 0.6f, 1.0f, 1.0f};
    bar->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    bar->corner_radius = 4.0f;

    bar->show_text = false;
    bar->format_text = strdup("%.0f%%");
    bar->animation_time = 0.0f;

    bar->base.render = ui_progressbar_render_impl;
    bar->base.layout = ui_progressbar_layout_impl;
    bar->base.measure = ui_progressbar_measure_impl;
    bar->base.destroy = ui_progressbar_destroy_impl;

    // Progress bars generally don't take focus or handle input
    bar->base.focusable = false;

    ui_progressbar_layout_impl(&bar->base, 0, 0);

    return bar;
}

void ui_progressbar_destroy(UIProgressBar* bar) {
    if (!bar) return;

    if (bar->base.destroy) {
        bar->base.destroy(&bar->base);
    }

    if (bar->base.children) {
        free(bar->base.children);
    }
    if (bar->base.name) {
        free(bar->base.name);
    }

    free(bar);
}

void ui_progressbar_set_value(UIProgressBar* bar, float value) {
    if (!bar) return;

    float new_value = ui_clamp(value, bar->min_value, bar->max_value);
    if (bar->value != new_value) {
        bar->value = new_value;
        widget_invalidate_redraw(&bar->base);
    }
}

float ui_progressbar_get_value(const UIProgressBar* bar) {
    if (!bar) return 0.0f;
    return bar->value;
}

void ui_progressbar_set_fraction(UIProgressBar* bar, float fraction) {
    if (!bar) return;
    fraction = ui_clamp(fraction, 0.0f, 1.0f);
    float val = bar->min_value + fraction * (bar->max_value - bar->min_value);
    ui_progressbar_set_value(bar, val);
}

void ui_progressbar_set_range(UIProgressBar* bar, float min, float max) {
    if (!bar) return;
    bar->min_value = min;
    bar->max_value = max;
    ui_progressbar_set_value(bar, bar->value); // Re-clamp
}

void ui_progressbar_set_indeterminate(UIProgressBar* bar, bool indeterminate) {
    if (!bar) return;
    if (bar->indeterminate != indeterminate) {
        bar->indeterminate = indeterminate;
        widget_invalidate_redraw(&bar->base);
    }
}

void ui_progressbar_set_colors(UIProgressBar* bar, Vec4 bg, Vec4 fill) {
    if (!bar) return;
    bar->background_color = bg;
    bar->fill_color = fill;
    widget_invalidate_redraw(&bar->base);
}

void ui_progressbar_set_show_text(UIProgressBar* bar, bool show, const char* format) {
    if (!bar) return;
    bar->show_text = show;

    if (format) {
        if (bar->format_text) free(bar->format_text);
        bar->format_text = strdup(format);
    }

    widget_invalidate_redraw(&bar->base);
}

void ui_progressbar_set_text_color(UIProgressBar* bar, Vec4 color) {
    if (!bar) return;
    bar->text_color = color;
    widget_invalidate_redraw(&bar->base);
}

Widget* ui_progressbar_get_widget(UIProgressBar* bar) {
    if (!bar) return NULL;
    return &bar->base;
}
