// Analytics Dashboard implementation
#include "tools/analytics_dashboard.h"
#include <core/logger.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// MARK: - Helper Functions

static u64 get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static Vec4 get_chart_color(u32 index) {
    static const Vec4 colors[] = {
        {1.0f, 0.0f, 0.0f, 1.0f},  // Red
        {0.0f, 1.0f, 0.0f, 1.0f},  // Green
        {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
        {1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
        {1.0f, 0.0f, 1.0f, 1.0f},  // Magenta
        {0.0f, 1.0f, 1.0f, 1.0f},  // Cyan
        {1.0f, 0.5f, 0.0f, 1.0f},  // Orange
        {0.5f, 0.0f, 1.0f, 1.0f},  // Purple
    };
    
    return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

static void render_line_chart(const ChartData* chart) {
    if (!chart || chart->point_count < 2) {
        return;
    }
    
    // TODO: Implement actual rendering with graphics API
    LOG_DEBUG("Rendering line chart: %s (%u points)", chart->title, chart->point_count);
    
    // For now, just log the data points
    for (u32 i = 0; i < chart->point_count; i++) {
        LOG_DEBUG("Point %u: (%.2f, %.2f) - %s", i, chart->points[i].x, chart->points[i].y, chart->points[i].label);
    }
}

static void render_bar_chart(const ChartData* chart) {
    if (!chart || chart->point_count == 0) {
        return;
    }
    
    // TODO: Implement actual rendering with graphics API
    LOG_DEBUG("Rendering bar chart: %s (%u bars)", chart->title, chart->point_count);
    
    for (u32 i = 0; i < chart->point_count; i++) {
        LOG_DEBUG("Bar %u: %.2f - %s", i, chart->points[i].y, chart->points[i].label);
    }
}

static void render_pie_chart(const ChartData* chart) {
    if (!chart || chart->point_count == 0) {
        return;
    }
    
    // TODO: Implement actual rendering with graphics API
    LOG_DEBUG("Rendering pie chart: %s (%u slices)", chart->title, chart->point_count);
    
    float total = 0.0f;
    for (u32 i = 0; i < chart->point_count; i++) {
        total += chart->points[i].y;
    }
    
    for (u32 i = 0; i < chart->point_count; i++) {
        float percentage = (chart->points[i].y / total) * 100.0f;
        LOG_DEBUG("Slice %u: %.2f%% - %s", i, percentage, chart->points[i].label);
    }
}

// MARK: - Analytics Dashboard Management

bool analytics_dashboard_init(AnalyticsDashboard* dashboard, u32 max_charts, u32 max_widgets) {
    if (!dashboard || max_charts == 0 || max_widgets == 0) {
        LOG_ERROR("Invalid parameters for analytics_dashboard_init");
        return false;
    }
    
    memset(dashboard, 0, sizeof(AnalyticsDashboard));
    
    // Allocate charts array
    dashboard->charts = (ChartData*)calloc(max_charts, sizeof(ChartData));
    if (!dashboard->charts) {
        LOG_ERROR("Failed to allocate charts array");
        return false;
    }
    dashboard->chart_capacity = max_charts;
    
    // Initialize layout
    dashboard->layout.widget_types = (WidgetType*)calloc(max_widgets, sizeof(WidgetType));
    dashboard->layout.widgets = (void**)calloc(max_widgets, sizeof(void*));
    if (!dashboard->layout.widget_types || !dashboard->layout.widgets) {
        free(dashboard->charts);
        free(dashboard->layout.widget_types);
        free(dashboard->layout.widgets);
        LOG_ERROR("Failed to allocate layout arrays");
        return false;
    }
    dashboard->layout.capacity = max_widgets;
    
    // Set default layout
    dashboard->layout.columns = 4;
    dashboard->layout.rows = 3;
    dashboard->layout.widget_spacing = 10.0f;
    dashboard->layout.padding = 20.0f;
    dashboard->layout.background_color = (Vec4){0.1f, 0.1f, 0.1f, 1.0f};
    
    // Set default update interval
    dashboard->update_interval_ms = 1000; // 1 second
    
    LOG_INFO("Analytics dashboard initialized (charts: %u, widgets: %u)", max_charts, max_widgets);
    return true;
}

void analytics_dashboard_shutdown(AnalyticsDashboard* dashboard) {
    if (!dashboard) {
        return;
    }
    
    // Free charts
    if (dashboard->charts) {
        for (u32 i = 0; i < dashboard->chart_count; i++) {
            if (dashboard->charts[i].points) {
                free(dashboard->charts[i].points);
            }
        }
        free(dashboard->charts);
        dashboard->charts = NULL;
    }
    
    // Free widgets
    if (dashboard->layout.widgets) {
        for (u32 i = 0; i < dashboard->layout.widget_count; i++) {
            if (dashboard->layout.widgets[i]) {
                free(dashboard->layout.widgets[i]);
            }
        }
        free(dashboard->layout.widgets);
        dashboard->layout.widgets = NULL;
    }
    
    if (dashboard->layout.widget_types) {
        free(dashboard->layout.widget_types);
        dashboard->layout.widget_types = NULL;
    }
    
    memset(dashboard, 0, sizeof(AnalyticsDashboard));
    LOG_INFO("Analytics dashboard shutdown");
}

bool analytics_dashboard_start(AnalyticsDashboard* dashboard) {
    if (!dashboard) {
        return false;
    }
    
    dashboard->is_active = true;
    dashboard->last_update_time_ms = get_current_time_ms();
    
    LOG_INFO("Analytics dashboard started");
    return true;
}

void analytics_dashboard_stop(AnalyticsDashboard* dashboard) {
    if (!dashboard) {
        return;
    }
    
    dashboard->is_active = false;
    LOG_INFO("Analytics dashboard stopped");
}

void analytics_dashboard_update(AnalyticsDashboard* dashboard) {
    if (!dashboard || !dashboard->is_active) {
        return;
    }
    
    u64 current_time = get_current_time_ms();
    if (current_time - dashboard->last_update_time_ms < dashboard->update_interval_ms) {
        return; // Not time to update yet
    }
    
    // Update all charts from data sources
    analytics_dashboard_update_all_charts(dashboard);
    
    // Auto-refresh layout if enabled
    if (dashboard->layout.auto_refresh) {
        u64 refresh_time = current_time - dashboard->layout.last_refresh_time_ms;
        if (refresh_time >= dashboard->layout.refresh_interval_ms) {
            dashboard->layout.last_refresh_time_ms = current_time;
            // TODO: Trigger layout refresh
        }
    }
    
    dashboard->last_update_time_ms = current_time;
}

// MARK: - Chart Management

ChartData* analytics_dashboard_create_chart(AnalyticsDashboard* dashboard, ChartType type, const char* title, 
                                          u32 max_points, const char* x_label, const char* y_label) {
    if (!dashboard || !title || max_points == 0) {
        return NULL;
    }
    
    if (dashboard->chart_count >= dashboard->chart_capacity) {
        LOG_ERROR("Chart capacity exceeded");
        return NULL;
    }
    
    ChartData* chart = &dashboard->charts[dashboard->chart_count];
    memset(chart, 0, sizeof(ChartData));
    
    // Allocate points array
    chart->points = (DataPoint*)calloc(max_points, sizeof(DataPoint));
    if (!chart->points) {
        LOG_ERROR("Failed to allocate chart points");
        return NULL;
    }
    
    // Set chart properties
    chart->type = type;
    chart->capacity = max_points;
    strncpy(chart->title, title, sizeof(chart->title) - 1);
    chart->title[sizeof(chart->title) - 1] = '\0';
    
    if (x_label) {
        strncpy(chart->x_label, x_label, sizeof(chart->x_label) - 1);
        chart->x_label[sizeof(chart->x_label) - 1] = '\0';
    }
    
    if (y_label) {
        strncpy(chart->y_label, y_label, sizeof(chart->y_label) - 1);
        chart->y_label[sizeof(chart->y_label) - 1] = '\0';
    }
    
    // Set default colors
    chart->background_color = (Vec4){0.2f, 0.2f, 0.2f, 1.0f};
    chart->grid_color = (Vec4){0.5f, 0.5f, 0.5f, 0.3f};
    chart->show_grid = true;
    chart->show_legend = true;
    chart->show_axes = true;
    
    dashboard->chart_count++;
    LOG_DEBUG("Created chart: %s", title);
    return chart;
}

bool analytics_chart_add_point(ChartData* chart, float x, float y, const char* label, Vec4 color) {
    if (!chart || chart->point_count >= chart->capacity) {
        return false;
    }
    
    DataPoint* point = &chart->points[chart->point_count++];
    point->x = x;
    point->y = y;
    point->color = color;
    
    if (label) {
        strncpy(point->label, label, sizeof(point->label) - 1);
        point->label[sizeof(point->label) - 1] = '\0';
    }
    
    return true;
}

bool analytics_chart_add_points(ChartData* chart, const DataPoint* points, u32 count) {
    if (!chart || !points || count == 0) {
        return false;
    }
    
    if (chart->point_count + count > chart->capacity) {
        LOG_ERROR("Insufficient capacity for chart points");
        return false;
    }
    
    for (u32 i = 0; i < count; i++) {
        chart->points[chart->point_count++] = points[i];
    }
    
    return true;
}

void analytics_chart_clear_data(ChartData* chart) {
    if (!chart) {
        return;
    }
    
    chart->point_count = 0;
    LOG_DEBUG("Cleared chart data: %s", chart->title);
}

// MARK: - Widget Management

MetricCard* analytics_dashboard_create_metric_card(AnalyticsDashboard* dashboard, const char* title, 
                                               const char* unit, float min_val, float max_val, Vec4 bg_color) {
    if (!dashboard || !title) {
        return NULL;
    }
    
    if (dashboard->layout.widget_count >= dashboard->layout.capacity) {
        LOG_ERROR("Widget capacity exceeded");
        return NULL;
    }
    
    MetricCard* card = (MetricCard*)calloc(1, sizeof(MetricCard));
    if (!card) {
        LOG_ERROR("Failed to allocate metric card");
        return NULL;
    }
    
    // Set card properties
    strncpy(card->title, title, sizeof(card->title) - 1);
    card->title[sizeof(card->title) - 1] = '\0';
    
    if (unit) {
        strncpy(card->unit, unit, sizeof(card->unit) - 1);
        card->unit[sizeof(card->unit) - 1] = '\0';
    }
    
    card->min_value = min_val;
    card->max_value = max_val;
    card->background_color = bg_color;
    card->value_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    card->trend_color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f};
    card->show_trend = true;
    
    // Add to layout
    dashboard->layout.widget_types[dashboard->layout.widget_count] = WIDGET_TYPE_METRIC_CARD;
    dashboard->layout.widgets[dashboard->layout.widget_count++] = card;
    
    LOG_DEBUG("Created metric card: %s", title);
    return card;
}

ProgressBar* analytics_dashboard_create_progress_bar(AnalyticsDashboard* dashboard, const char* title, 
                                              Vec4 bar_color, Vec4 bg_color) {
    if (!dashboard || !title) {
        return NULL;
    }
    
    if (dashboard->layout.widget_count >= dashboard->layout.capacity) {
        LOG_ERROR("Widget capacity exceeded");
        return NULL;
    }
    
    ProgressBar* bar = (ProgressBar*)calloc(1, sizeof(ProgressBar));
    if (!bar) {
        LOG_ERROR("Failed to allocate progress bar");
        return NULL;
    }
    
    // Set progress bar properties
    strncpy(bar->title, title, sizeof(bar->title) - 1);
    bar->title[sizeof(bar->title) - 1] = '\0';
    
    bar->bar_color = bar_color;
    bar->background_color = bg_color;
    bar->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    bar->show_percentage = true;
    
    // Add to layout
    dashboard->layout.widget_types[dashboard->layout.widget_count] = WIDGET_TYPE_PROGRESS_BAR;
    dashboard->layout.widgets[dashboard->layout.widget_count++] = bar;
    
    LOG_DEBUG("Created progress bar: %s", title);
    return bar;
}

StatusIndicator* analytics_dashboard_create_status_indicator(AnalyticsDashboard* dashboard, const char* title, 
                                                       Vec4 active_color, Vec4 inactive_color) {
    if (!dashboard || !title) {
        return NULL;
    }
    
    if (dashboard->layout.widget_count >= dashboard->layout.capacity) {
        LOG_ERROR("Widget capacity exceeded");
        return NULL;
    }
    
    StatusIndicator* indicator = (StatusIndicator*)calloc(1, sizeof(StatusIndicator));
    if (!indicator) {
        LOG_ERROR("Failed to allocate status indicator");
        return NULL;
    }
    
    // Set indicator properties
    strncpy(indicator->title, title, sizeof(indicator->title) - 1);
    indicator->title[sizeof(indicator->title) - 1] = '\0';
    
    indicator->active_color = active_color;
    indicator->inactive_color = inactive_color;
    indicator->warning_color = (Vec4){1.0f, 1.0f, 0.0f, 1.0f};
    indicator->critical_color = (Vec4){1.0f, 0.0f, 0.0f, 1.0f};
    
    // Add to layout
    dashboard->layout.widget_types[dashboard->layout.widget_count] = WIDGET_STATUS_INDICATOR;
    dashboard->layout.widgets[dashboard->layout.widget_count++] = indicator;
    
    LOG_DEBUG("Created status indicator: %s", title);
    return indicator;
}

TextLabel* analytics_dashboard_create_text_label(AnalyticsDashboard* dashboard, const char* text, 
                                             Vec4 text_color, Vec4 bg_color, float font_size) {
    if (!dashboard || !text) {
        return NULL;
    }
    
    if (dashboard->layout.widget_count >= dashboard->layout.capacity) {
        LOG_ERROR("Widget capacity exceeded");
        return NULL;
    }
    
    TextLabel* label = (TextLabel*)calloc(1, sizeof(TextLabel));
    if (!label) {
        LOG_ERROR("Failed to allocate text label");
        return NULL;
    }
    
    // Set label properties
    strncpy(label->text, text, sizeof(label->text) - 1);
    label->text[sizeof(label->text) - 1] = '\0';
    
    label->text_color = text_color;
    label->background_color = bg_color;
    label->font_size = font_size;
    label->center_text = true;
    
    // Add to layout
    dashboard->layout.widget_types[dashboard->layout.widget_count] = WIDGET_TEXT_LABEL;
    dashboard->layout.widgets[dashboard->layout.widget_count++] = label;
    
    LOG_DEBUG("Created text label: %s", text);
    return label;
}

// MARK: - Data Management

void analytics_dashboard_set_metric_value(MetricCard* card, float value) {
    if (!card) {
        return;
    }
    
    card->current_value = value;
    
    // Format value string
    snprintf(card->value, sizeof(card->value), "%.2f", value);
    
    // Check warning/critical thresholds
    float percentage = (value - card->min_value) / (card->max_value - card->min_value);
    card->is_warning = percentage > 0.7f && percentage <= 0.85f;
    card->is_critical = percentage > 0.85f;
    
    if (card->is_critical) {
        card->trend_color = (Vec4){1.0f, 0.0f, 0.0f, 1.0f}; // Red
    } else if (card->is_warning) {
        card->trend_color = (Vec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
    } else {
        card->trend_color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
    }
}

void analytics_dashboard_set_progress_value(ProgressBar* bar, float progress) {
    if (!bar) {
        return;
    }
    
    bar->current_progress = progress;
    if (bar->current_progress > 1.0f) {
        bar->current_progress = 1.0f;
    } else if (bar->current_progress < 0.0f) {
        bar->current_progress = 0.0f;
    }
}

void analytics_dashboard_set_status(StatusIndicator* indicator, bool status) {
    if (!indicator) {
        return;
    }
    
    indicator->status = status;
}

void analytics_dashboard_set_text(TextLabel* label, const char* text) {
    if (!label || !text) {
        return;
    }
    
    strncpy(label->text, text, sizeof(label->text) - 1);
    label->text[sizeof(label->text) - 1] = '\0';
}

// MARK: - Chart Generation Helpers

void analytics_generate_line_chart(ChartData* chart, const float* x_data, const float* y_data, u32 count, Vec4 color) {
    if (!chart || !x_data || !y_data || count == 0) {
        return;
    }
    
    analytics_chart_clear_data(chart);
    
    for (u32 i = 0; i < count && i < chart->capacity; i++) {
        analytics_chart_add_point(chart, x_data[i], y_data[i], NULL, color);
    }
    
    LOG_DEBUG("Generated line chart with %u points", count);
}

void analytics_generate_bar_chart(ChartData* chart, const char* labels[], const float* values, u32 count, Vec4 color) {
    if (!chart || !labels || !values || count == 0) {
        return;
    }
    
    analytics_chart_clear_data(chart);
    
    for (u32 i = 0; i < count && i < chart->capacity; i++) {
        analytics_chart_add_point(chart, (float)i, values[i], labels[i], color);
    }
    
    LOG_DEBUG("Generated bar chart with %u bars", count);
}

void analytics_generate_pie_chart(ChartData* chart, const float* values, const char* labels[], u32 count, const Vec4* colors) {
    if (!chart || !values || !labels || count == 0) {
        return;
    }
    
    analytics_chart_clear_data(chart);
    
    for (u32 i = 0; i < count && i < chart->capacity; i++) {
        Vec4 color = colors ? colors[i] : get_chart_color(i);
        analytics_chart_add_point(chart, 0.0f, values[i], labels[i], color);
    }
    
    LOG_DEBUG("Generated pie chart with %u slices", count);
}

void analytics_generate_scatter_plot(ChartData* chart, const float* x_data, const float* y_data, u32 count, Vec4 color) {
    if (!chart || !x_data || !y_data || count == 0) {
        return;
    }
    
    analytics_chart_clear_data(chart);
    
    for (u32 i = 0; i < count && i < chart->capacity; i++) {
        analytics_chart_add_point(chart, x_data[i], y_data[i], NULL, color);
    }
    
    LOG_DEBUG("Generated scatter plot with %u points", count);
}

// MARK: - Real-time Data Updates

void analytics_dashboard_update_fps_chart(AnalyticsDashboard* dashboard, const float* fps_history, u32 count) {
    if (!dashboard || !fps_history || count == 0) {
        return;
    }
    
    // Find or create FPS chart
    ChartData* fps_chart = NULL;
    for (u32 i = 0; i < dashboard->chart_count; i++) {
        if (strcmp(dashboard->charts[i].title, "FPS Performance") == 0) {
            fps_chart = &dashboard->charts[i];
            break;
        }
    }
    
    if (!fps_chart) {
        fps_chart = analytics_dashboard_create_chart(dashboard, CHART_TYPE_LINE, "FPS Performance", 
                                                   count, "Time", "FPS");
        if (!fps_chart) {
            return;
        }
    }
    
    // Generate time data
    float* time_data = (float*)malloc(count * sizeof(float));
    for (u32 i = 0; i < count; i++) {
        time_data[i] = (float)i;
    }
    
    // Update chart
    analytics_generate_line_chart(fps_chart, time_data, fps_history, count, (Vec4){0.0f, 1.0f, 0.0f, 1.0f});
    
    free(time_data);
    LOG_DEBUG("Updated FPS chart with %u samples", count);
}

void analytics_dashboard_update_memory_chart(AnalyticsDashboard* dashboard, const float* memory_history, u32 count) {
    if (!dashboard || !memory_history || count == 0) {
        return;
    }
    
    // Find or create memory chart
    ChartData* mem_chart = NULL;
    for (u32 i = 0; i < dashboard->chart_count; i++) {
        if (strcmp(dashboard->charts[i].title, "Memory Usage") == 0) {
            mem_chart = &dashboard->charts[i];
            break;
        }
    }
    
    if (!mem_chart) {
        mem_chart = analytics_dashboard_create_chart(dashboard, CHART_TYPE_AREA, "Memory Usage", 
                                                    count, "Time", "Memory (MB)");
        if (!mem_chart) {
            return;
        }
    }
    
    // Generate time data
    float* time_data = (float*)malloc(count * sizeof(float));
    for (u32 i = 0; i < count; i++) {
        time_data[i] = (float)i;
    }
    
    // Update chart
    analytics_generate_line_chart(mem_chart, time_data, memory_history, count, (Vec4){1.0f, 0.0f, 1.0f, 1.0f});
    
    free(time_data);
    LOG_DEBUG("Updated memory chart with %u samples", count);
}

void analytics_dashboard_update_latency_chart(AnalyticsDashboard* dashboard, const float* latency_history, u32 count) {
    if (!dashboard || !latency_history || count == 0) {
        return;
    }
    
    // Find or create latency chart
    ChartData* latency_chart = NULL;
    for (u32 i = 0; i < dashboard->chart_count; i++) {
        if (strcmp(dashboard->charts[i].title, "Network Latency") == 0) {
            latency_chart = &dashboard->charts[i];
            break;
        }
    }
    
    if (!latency_chart) {
        latency_chart = analytics_dashboard_create_chart(dashboard, CHART_TYPE_LINE, "Network Latency", 
                                                        count, "Time", "Latency (ms)");
        if (!latency_chart) {
            return;
        }
    }
    
    // Generate time data
    float* time_data = (float*)malloc(count * sizeof(float));
    for (u32 i = 0; i < count; i++) {
        time_data[i] = (float)i;
    }
    
    // Update chart
    analytics_generate_line_chart(latency_chart, time_data, latency_history, count, (Vec4){1.0f, 1.0f, 0.0f, 1.0f});
    
    free(time_data);
    LOG_DEBUG("Updated latency chart with %u samples", count);
}

// MARK: - Utility Functions

void analytics_dashboard_print_summary(const AnalyticsDashboard* dashboard) {
    if (!dashboard) {
        printf("Dashboard is NULL\n");
        return;
    }
    
    printf("=== ANALYTICS DASHBOARD SUMMARY ===\n");
    printf("Status: %s\n", dashboard->is_active ? "ACTIVE" : "INACTIVE");
    printf("Charts: %u/%u\n", dashboard->chart_count, dashboard->chart_capacity);
    printf("Widgets: %u/%u\n", dashboard->layout.widget_count, dashboard->layout.capacity);
    printf("Layout: %ux%u grid\n", dashboard->layout.columns, dashboard->layout.rows);
    printf("Auto-refresh: %s\n", dashboard->layout.auto_refresh ? "ENABLED" : "DISABLED");
    
    if (dashboard->chart_count > 0) {
        printf("\nCharts:\n");
        for (u32 i = 0; i < dashboard->chart_count; i++) {
        }
    }
    
    printf("===============================\n");
}

void analytics_dashboard_export_to_json(const AnalyticsDashboard* dashboard, const char* filename) {
    if (!dashboard || !filename) {
        return;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        LOG_ERROR("Failed to open JSON file: %s", filename);
        return;
    }
    
    // Write JSON header
    fprintf(fp, "{\n");
    fprintf(fp, "  \"dashboard\": {\n");
    fprintf(fp, "    \"is_active\": %s,\n", dashboard->is_active ? "true" : "false");
    fprintf(fp, "    \"chart_count\": %u,\n", dashboard->chart_count);
    fprintf(fp, "    \"widget_count\": %u,\n", dashboard->layout.widget_count);
    fprintf(fp, "    \"layout\": {\n");
    fprintf(fp, "      \"columns\": %u,\n", dashboard->layout.columns);
    fprintf(fp, "      \"rows\": %u,\n", dashboard->layout.rows);
    fprintf(fp, "      \"widget_spacing\": %.2f,\n", dashboard->layout.widget_spacing);
    fprintf(fp, "      \"padding\": %.2f\n", dashboard->layout.padding);
    fprintf(fp, "    },\n");
    
    // Write charts
    for (u32 i = 0; i < dashboard->chart_count; i++) {
        const ChartData* chart = &dashboard->charts[i];
        fprintf(fp, "      {\n");
        fprintf(fp, "        \"title\": \"%s\",\n", chart->title);
        fprintf(fp, "        \"type\": %u,\n", chart->type);
        fprintf(fp, "        \"point_count\": %u,\n", chart->point_count);
        
        for (u32 j = 0; j < chart->point_count; j++) {
            fprintf(fp, "          {\"x\": %.2f, \"y\": %.2f, \"label\": \"%s\"}",
                   chart->points[j].x, chart->points[j].y, chart->points[j].label);
            if (j < chart->point_count - 1) {
                fprintf(fp, ",");
            }
            fprintf(fp, "\n");
        }
        
        fprintf(fp, "        ]\n");
        fprintf(fp, "      }");
        if (i < dashboard->chart_count - 1) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "    ]\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    LOG_INFO("Exported dashboard to JSON: %s", filename);
}
