#ifndef ANALYTICS_DASHBOARD_H
#define ANALYTICS_DASHBOARD_H

#include <core/types.h>
#include <math/math.h>
#include <stdbool.h>
#include <stddef.h>

// Chart types
typedef enum {
    CHART_TYPE_LINE,
    CHART_TYPE_BAR,
    CHART_TYPE_PIE,
    CHART_TYPE_SCATTER,
    CHART_TYPE_HEATMAP,
    CHART_TYPE_AREA
} ChartType;

// Data point for charts
typedef struct {
    float x, y;
    char label[64];
    Vec4 color;
} DataPoint;

// Chart data
typedef struct {
    DataPoint* points;
    u32 point_count;
    u32 capacity;
    ChartType type;
    char title[128];
    char x_label[64];
    char y_label[64];
    Vec4 background_color;
    Vec4 grid_color;
    bool show_grid;
    bool show_legend;
    bool show_axes;
} ChartData;

// Dashboard widget types
typedef enum {
    WIDGET_TYPE_CHART,
    WIDGET_TYPE_METRIC_CARD,
    WIDGET_TYPE_PROGRESS_BAR,
    WIDGET_STATUS_INDICATOR,
    WIDGET_TEXT_LABEL,
    WIDGET_BUTTON
} WidgetType;

// Metric card widget
typedef struct {
    char title[64];
    char value[32];
    char unit[16];
    float current_value;
    float target_value;
    float min_value;
    float max_value;
    Vec4 background_color;
    Vec4 value_color;
    Vec4 trend_color;
    bool show_trend;
    bool is_warning;
    bool is_critical;
} MetricCard;

// Progress bar widget
typedef struct {
    char title[64];
    float current_progress;
    float target_progress;
    Vec4 bar_color;
    Vec4 background_color;
    Vec4 text_color;
    bool show_percentage;
} ProgressBar;

// Status indicator widget
typedef struct {
    char title[64];
    bool status;
    Vec4 active_color;
    Vec4 inactive_color;
    Vec4 warning_color;
    Vec4 critical_color;
} StatusIndicator;

// Text label widget
typedef struct {
    char text[256];
    Vec4 text_color;
    Vec4 background_color;
    float font_size;
    bool center_text;
} TextLabel;

// Button widget
typedef struct {
    char text[128];
    Vec4 button_color;
    Vec4 text_color;
    Vec4 hover_color;
    bool is_pressed;
    bool is_hovered;
    void (*on_click)(void* user_data);
    void* user_data;
} Button;

// Dashboard layout
typedef struct {
    WidgetType* widget_types;
    void** widgets;
    u32 widget_count;
    u32 capacity;
    
    // Layout properties
    u32 columns;
    u32 rows;
    float widget_spacing;
    float padding;
    Vec4 background_color;
    
    bool auto_refresh;
    u32 refresh_interval_ms;
    u64 last_refresh_time_ms;
} DashboardLayout;

// Analytics dashboard
typedef struct {
    DashboardLayout layout;
    
    // Chart data storage
    ChartData* charts;
    u32 chart_count;
    u32 chart_capacity;
    
    // Real-time data sources
    void* data_sources;
    u32 data_source_count;
    
    // Dashboard configuration
    bool is_active;
    u64 last_update_time_ms;
    u32 update_interval_ms;
    
    // Callbacks
    void (*on_widget_click)(void* widget, void* user_data);
    void (*on_data_updated)(const char* widget_id);
    void* user_data;
} AnalyticsDashboard;

// MARK: - Analytics Dashboard Management

bool analytics_dashboard_init(AnalyticsDashboard* dashboard, u32 max_charts, u32 max_widgets);
void analytics_dashboard_shutdown(AnalyticsDashboard* dashboard);

bool analytics_dashboard_start(AnalyticsDashboard* dashboard);
void analytics_dashboard_stop(AnalyticsDashboard* dashboard);
void analytics_dashboard_update(AnalyticsDashboard* dashboard);

// MARK: - Chart Management

ChartData* analytics_dashboard_create_chart(AnalyticsDashboard* dashboard, ChartType type, const char* title, 
                                          u32 max_points, const char* x_label, const char* y_label);
bool analytics_dashboard_add_chart(AnalyticsDashboard* dashboard, const ChartData* chart);
bool analytics_dashboard_remove_chart(AnalyticsDashboard* dashboard, const char* chart_id);
ChartData* analytics_dashboard_get_chart(AnalyticsDashboard* dashboard, const char* chart_id);

bool analytics_chart_add_point(ChartData* chart, float x, float y, const char* label, Vec4 color);
bool analytics_chart_add_points(ChartData* chart, const DataPoint* points, u32 count);
void analytics_chart_clear_data(ChartData* chart);

// MARK: - Widget Management

MetricCard* analytics_dashboard_create_metric_card(AnalyticsDashboard* dashboard, const char* title, 
                                               const char* unit, float min_val, float max_val, Vec4 bg_color);
ProgressBar* analytics_dashboard_create_progress_bar(AnalyticsDashboard* dashboard, const char* title, 
                                              Vec4 bar_color, Vec4 bg_color);
StatusIndicator* analytics_dashboard_create_status_indicator(AnalyticsDashboard* dashboard, const char* title, 
                                                       Vec4 active_color, Vec4 inactive_color);
TextLabel* analytics_dashboard_create_text_label(AnalyticsDashboard* dashboard, const char* text, 
                                             Vec4 text_color, Vec4 bg_color, float font_size);
Button* analytics_dashboard_create_button(AnalyticsDashboard* dashboard, const char* text, 
                                         Vec4 button_color, Vec4 text_color, 
                                         void (*on_click)(void*, void*), void* user_data);

bool analytics_dashboard_add_widget(AnalyticsDashboard* dashboard, WidgetType type, void* widget);
bool analytics_dashboard_remove_widget(AnalyticsDashboard* dashboard, void* widget);
void* analytics_dashboard_get_widget(AnalyticsDashboard* dashboard, const char* widget_id);

// MARK: - Data Management

void analytics_dashboard_set_chart_data_source(AnalyticsDashboard* dashboard, const char* chart_id, void* data_source);
void analytics_dashboard_update_chart_from_source(AnalyticsDashboard* dashboard, const char* chart_id);
void analytics_dashboard_update_all_charts(AnalyticsDashboard* dashboard);

void analytics_dashboard_set_metric_value(MetricCard* card, float value);
void analytics_dashboard_set_progress_value(ProgressBar* bar, float progress);
void analytics_dashboard_set_status(StatusIndicator* indicator, bool status);
void analytics_dashboard_set_text(TextLabel* label, const char* text);

// MARK: - Layout Management

void analytics_dashboard_set_layout(AnalyticsDashboard* dashboard, u32 columns, u32 rows, 
                                     float spacing, float padding, Vec4 bg_color);
void analytics_dashboard_auto_refresh(AnalyticsDashboard* dashboard, bool enabled, u32 interval_ms);

// MARK: - Rendering

void analytics_dashboard_render(AnalyticsDashboard* dashboard);
void analytics_dashboard_render_chart(const ChartData* chart);
void analytics_dashboard_render_widget(void* widget, WidgetType type);

// MARK: - Chart Generation Helpers

void analytics_generate_line_chart(ChartData* chart, const float* x_data, const float* y_data, u32 count, Vec4 color);
void analytics_generate_bar_chart(ChartData* chart, const char* labels[], const float* values, u32 count, Vec4 color);
void analytics_generate_pie_chart(ChartData* chart, const float* values, const char* labels[], u32 count, const Vec4* colors);
void analytics_generate_scatter_plot(ChartData* chart, const float* x_data, const float* y_data, u32 count, Vec4 color);

// MARK: - Utility Functions

void analytics_dashboard_print_summary(const AnalyticsDashboard* dashboard);
void analytics_dashboard_export_to_json(const AnalyticsDashboard* dashboard, const char* filename);
void analytics_dashboard_export_to_csv(const AnalyticsDashboard* dashboard, const char* filename);

// MARK: - Real-time Data Updates

void analytics_dashboard_update_fps_chart(AnalyticsDashboard* dashboard, const float* fps_history, u32 count);
void analytics_dashboard_update_memory_chart(AnalyticsDashboard* dashboard, const float* memory_history, u32 count);
void analytics_dashboard_update_latency_chart(AnalyticsDashboard* dashboard, const float* latency_history, u32 count);
void analytics_dashboard_update_player_activity_heatmap(AnalyticsDashboard* dashboard, const u32* activity_data, u32 width, u32 height);

#endif // ANALYTICS_DASHBOARD_H
