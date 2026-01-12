#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

// Analytics data types
typedef enum {
    ANALYTICS_METRIC_PLAYER_BEHAVIOR,
    ANALYTICS_METRIC_GAME_PERFORMANCE,
    ANALYTICS_METRIC_SERVER_PERFORMANCE,
    ANALYTICS_METRIC_ECONOMY,
    ANALYTICS_METRIC_QUEST_COMPLETION,
    ANALYTICS_METRIC_COMBAT,
    ANALYTICS_METRIC_SOCIAL
} analytics_metric_type_t;

// Chart types
typedef enum {
    CHART_LINE,
    CHART_BAR,
    CHART_PIE,
    CHART_SCATTER,
    CHART_HEATMAP,
    CHART_HISTOGRAM
} chart_type_t;

// Time periods
typedef enum {
    TIME_PERIOD_HOURLY,
    TIME_PERIOD_DAILY,
    TIME_PERIOD_WEEKLY,
    TIME_PERIOD_MONTHLY,
    TIME_PERIOD_YEARLY
} time_period_t;

// Data point structure
typedef struct {
    time_t timestamp;
    double value;
    char label[128];
    char category[64];
} data_point_t;

// Chart data structure
typedef struct {
    char title[256];
    chart_type_t type;
    data_point_t* data_points;
    int data_point_count;
    int data_point_capacity;
    char x_axis_label[128];
    char y_axis_label[128];
    bool show_grid;
    bool show_legend;
    char color_scheme[64];
} chart_data_t;

// Analytics metric structure
typedef struct {
    char metric_id[128];
    char name[256];
    analytics_metric_type_t type;
    char description[512];
    data_point_t* historical_data;
    int data_count;
    int data_capacity;
    time_t last_updated;
    double current_value;
    double average_value;
    double min_value;
    double max_value;
    char unit[64];
} analytics_metric_t;

// Dashboard widget structure
typedef struct {
    char widget_id[128];
    char title[256];
    chart_data_t* chart;
    analytics_metric_t* metric;
    int x, y, width, height;
    bool is_visible;
    int refresh_interval_seconds;
    time_t last_refresh;
} dashboard_widget_t;

// Dashboard structure
typedef struct {
    char dashboard_id[128];
    char name[256];
    char description[512];
    dashboard_widget_t* widgets;
    int widget_count;
    int widget_capacity;
    time_t created_time;
    time_t last_modified;
    char creator[128];
    bool is_public;
} analytics_dashboard_t;

// Analytics manager structure
typedef struct {
    analytics_metric_t* metrics;
    int metric_count;
    int metric_capacity;
    analytics_dashboard_t* dashboards;
    int dashboard_count;
    int dashboard_capacity;
    data_point_t* data_buffer;
    int buffer_count;
    int buffer_capacity;
    bool is_recording;
    time_t recording_start_time;
} analytics_manager_t;

// Analytics manager functions
analytics_manager_t* analytics_manager_create(void);
void analytics_manager_destroy(analytics_manager_t* manager);
int analytics_manager_add_metric(analytics_manager_t* manager, const analytics_metric_t* metric);
analytics_metric_t* analytics_manager_get_metric(analytics_manager_t* manager, const char* metric_id);
bool analytics_manager_record_data_point(analytics_manager_t* manager, const char* metric_id, double value, const char* label);
bool analytics_manager_start_recording(analytics_manager_t* manager);
bool analytics_manager_stop_recording(analytics_manager_t* manager);
void analytics_manager_update(analytics_manager_t* manager);

// Dashboard functions
int analytics_manager_create_dashboard(analytics_manager_t* manager, const analytics_dashboard_t* dashboard);
analytics_dashboard_t* analytics_manager_get_dashboard(analytics_manager_t* manager, const char* dashboard_id);
int dashboard_add_widget(analytics_dashboard_t* dashboard, const dashboard_widget_t* widget);
bool dashboard_remove_widget(analytics_dashboard_t* dashboard, const char* widget_id);
dashboard_widget_t* dashboard_get_widget(analytics_dashboard_t* dashboard, const char* widget_id);
void dashboard_update_widget(analytics_dashboard_t* dashboard, const char* widget_id);
void dashboard_render(analytics_dashboard_t* dashboard, char* output_buffer, size_t buffer_size);

// Chart generation functions
chart_data_t* chart_create_line_chart(const char* title, const data_point_t* data, int count);
chart_data_t* chart_create_bar_chart(const char* title, const data_point_t* data, int count);
chart_data_t* chart_create_pie_chart(const char* title, const data_point_t* data, int count);
void chart_add_data_point(chart_data_t* chart, const data_point_t* point);
void chart_render_ascii(chart_data_t* chart, char* buffer, size_t buffer_size, int width, int height);
void chart_render_html(chart_data_t* chart, char* buffer, size_t buffer_size);
void chart_destroy(chart_data_t* chart);

// Data analysis functions
double analytics_calculate_trend(const analytics_metric_t* metric, time_period_t period);
double analytics_calculate_average(const analytics_metric_t* metric, time_period_t period);
double analytics_calculate_growth_rate(const analytics_metric_t* metric, time_period_t period);
int analytics_get_top_performers(analytics_manager_t* manager, const char* metric_id, int count, char** player_ids);
int analytics_get_anomalies(analytics_manager_t* manager, const char* metric_id, double threshold, data_point_t** anomalies);

// Report generation functions
void analytics_generate_report(analytics_manager_t* manager, const char* dashboard_id, 
                              time_period_t period, char* buffer, size_t buffer_size);
void analytics_export_csv(analytics_manager_t* manager, const char* metric_id, 
                         time_t start_time, time_t end_time, char* buffer, size_t buffer_size);
void analytics_export_json(analytics_manager_t* manager, const char* dashboard_id, 
                          char* buffer, size_t buffer_size);

// Real-time monitoring functions
typedef void (*analytics_alert_callback_t)(const char* metric_id, double value, double threshold);
void analytics_set_alert(analytics_manager_t* manager, const char* metric_id, 
                        double threshold, analytics_alert_callback_t callback);
void analytics_check_alerts(analytics_manager_t* manager);

// Utility functions
const char* analytics_metric_type_to_string(analytics_metric_type_t type);
const char* chart_type_to_string(chart_type_t type);
const char* time_period_to_string(time_period_t period);
void analytics_format_value(char* buffer, size_t buffer_size, double value, const char* unit);
void analytics_debug_print_metric(const analytics_metric_t* metric);
void analytics_debug_print_dashboard(const analytics_dashboard_t* dashboard);

// Implementation
analytics_manager_t* analytics_manager_create(void) {
    analytics_manager_t* manager = malloc(sizeof(analytics_manager_t));
    if (!manager) return NULL;
    
    manager->metrics = malloc(sizeof(analytics_metric_t) * 100);
    manager->metric_count = 0;
    manager->metric_capacity = 100;
    
    manager->dashboards = malloc(sizeof(analytics_dashboard_t) * 20);
    manager->dashboard_count = 0;
    manager->dashboard_capacity = 20;
    
    manager->data_buffer = malloc(sizeof(data_point_t) * 10000);
    manager->buffer_count = 0;
    manager->buffer_capacity = 10000;
    
    manager->is_recording = false;
    manager->recording_start_time = 0;
    
    return manager;
}

void analytics_manager_destroy(analytics_manager_t* manager) {
    if (!manager) return;
    
    for (int i = 0; i < manager->metric_count; i++) {
        free(manager->metrics[i].historical_data);
    }
    
    for (int i = 0; i < manager->dashboard_count; i++) {
        for (int j = 0; j < manager->dashboards[i].widget_count; j++) {
            if (manager->dashboards[i].widgets[j].chart) {
                free(manager->dashboards[i].widgets[j].chart->data_points);
                free(manager->dashboards[i].widgets[j].chart);
            }
        }
        free(manager->dashboards[i].widgets);
    }
    
    free(manager->metrics);
    free(manager->dashboards);
    free(manager->data_buffer);
    free(manager);
}

int analytics_manager_add_metric(analytics_manager_t* manager, const analytics_metric_t* metric) {
    if (!manager || !metric || manager->metric_count >= manager->metric_capacity) {
        return -1;
    }
    
    analytics_metric_t new_metric = *metric;
    
    // Initialize historical data array
    if (metric->historical_data && metric->data_count > 0) {
        new_metric.historical_data = malloc(sizeof(data_point_t) * metric->data_count);
        memcpy(new_metric.historical_data, metric->historical_data, 
               sizeof(data_point_t) * metric->data_count);
    } else {
        new_metric.historical_data = malloc(sizeof(data_point_t) * 1000);
        new_metric.data_count = 0;
        new_metric.data_capacity = 1000;
    }
    
    manager->metrics[manager->metric_count++] = new_metric;
    return manager->metric_count - 1;
}

analytics_metric_t* analytics_manager_get_metric(analytics_manager_t* manager, const char* metric_id) {
    if (!manager || !metric_id) return NULL;
    
    for (int i = 0; i < manager->metric_count; i++) {
        if (strcmp(manager->metrics[i].metric_id, metric_id) == 0) {
            return &manager->metrics[i];
        }
    }
    return NULL;
}

bool analytics_manager_record_data_point(analytics_manager_t* manager, const char* metric_id, double value, const char* label) {
    if (!manager || !metric_id) return false;
    
    analytics_metric_t* metric = analytics_manager_get_metric(manager, metric_id);
    if (!metric) return false;
    
    if (metric->data_count >= metric->data_capacity) {
        // Expand capacity
        int new_capacity = metric->data_capacity * 2;
        data_point_t* new_data = realloc(metric->historical_data, sizeof(data_point_t) * new_capacity);
        if (!new_data) return false;
        
        metric->historical_data = new_data;
        metric->data_capacity = new_capacity;
    }
    
    data_point_t point = {0};
    point.timestamp = time(NULL);
    point.value = value;
    if (label) {
        strncpy(point.label, label, sizeof(point.label) - 1);
    }
    
    metric->historical_data[metric->data_count++] = point;
    metric->current_value = value;
    metric->last_updated = point.timestamp;
    
    // Update min/max/average
    if (metric->data_count == 1) {
        metric->min_value = value;
        metric->max_value = value;
        metric->average_value = value;
    } else {
        if (value < metric->min_value) metric->min_value = value;
        if (value > metric->max_value) metric->max_value = value;
        
        // Calculate running average
        double sum = 0.0;
        for (int i = 0; i < metric->data_count; i++) {
            sum += metric->historical_data[i].value;
        }
        metric->average_value = sum / metric->data_count;
    }
    
    return true;
}

int analytics_manager_create_dashboard(analytics_manager_t* manager, const analytics_dashboard_t* dashboard) {
    if (!manager || !dashboard || manager->dashboard_count >= manager->dashboard_capacity) {
        return -1;
    }
    
    analytics_dashboard_t new_dashboard = *dashboard;
    
    // Initialize widgets array
    if (dashboard->widgets && dashboard->widget_count > 0) {
        new_dashboard.widgets = malloc(sizeof(dashboard_widget_t) * dashboard->widget_count);
        memcpy(new_dashboard.widgets, dashboard->widgets, 
               sizeof(dashboard_widget_t) * dashboard->widget_count);
    } else {
        new_dashboard.widgets = malloc(sizeof(dashboard_widget_t) * 50);
        new_dashboard.widget_count = 0;
        new_dashboard.widget_capacity = 50;
    }
    
    new_dashboard.created_time = time(NULL);
    new_dashboard.last_modified = new_dashboard.created_time;
    
    manager->dashboards[manager->dashboard_count++] = new_dashboard;
    return manager->dashboard_count - 1;
}

void chart_render_ascii(chart_data_t* chart, char* buffer, size_t buffer_size, int width, int height) {
    if (!chart || !buffer || buffer_size == 0 || width <= 0 || height <= 0) return;
    
    // Simple ASCII chart rendering
    buffer[0] = '\0';
    
    // Add title
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "\n=== %s ===\n", chart->title);
    
    if (chart->data_point_count == 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "No data available\n");
        return;
    }
    
    // Find min and max values
    double min_val = chart->data_points[0].value;
    double max_val = chart->data_points[0].value;
    
    for (int i = 1; i < chart->data_point_count; i++) {
        if (chart->data_points[i].value < min_val) min_val = chart->data_points[i].value;
        if (chart->data_points[i].value > max_val) max_val = chart->data_points[i].value;
    }
    
    // Create chart grid
    char grid[50][100] = {0};
    
    // Plot data points
    for (int i = 0; i < chart->data_point_count && i < width; i++) {
        double normalized = (chart->data_points[i].value - min_val) / (max_val - min_val + 0.001);
        int y = (int)(normalized * (height - 1));
        
        if (y >= 0 && y < height && i < 100) {
            grid[y][i] = '*';
        }
    }
    
    // Render grid
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width && x < 100; x++) {
            if (grid[y][x]) {
                snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%c", grid[y][x]);
            } else {
                snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " ");
            }
        }
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "\n");
    }
    
    // Add axis labels
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), 
             "Min: %.2f Max: %.2f\n", min_val, max_val);
}

void dashboard_render(analytics_dashboard_t* dashboard, char* output_buffer, size_t buffer_size) {
    if (!dashboard || !output_buffer || buffer_size == 0) return;
    
    output_buffer[0] = '\0';
    
    snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
             "\n========================================\n");
    snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
             "Dashboard: %s\n", dashboard->name);
    snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
             "Description: %s\n", dashboard->description);
    snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
             "========================================\n");
    
    for (int i = 0; i < dashboard->widget_count; i++) {
        dashboard_widget_t* widget = &dashboard->widgets[i];
        if (!widget->is_visible) continue;
        
        snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
                 "\n--- Widget: %s ---\n", widget->title);
        
        if (widget->chart) {
            char chart_buffer[2048];
            chart_render_ascii(widget->chart, chart_buffer, sizeof(chart_buffer), 40, 10);
            snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
                     "%s\n", chart_buffer);
        }
        
        if (widget->metric) {
            char value_str[128];
            analytics_format_value(value_str, sizeof(value_str), 
                                  widget->metric->current_value, widget->metric->unit);
            snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
                     "Current Value: %s\n", value_str);
            snprintf(output_buffer + strlen(output_buffer), buffer_size - strlen(output_buffer),
                     "Average: %.2f %s\n", widget->metric->average_value, widget->metric->unit);
        }
    }
}

void analytics_format_value(char* buffer, size_t buffer_size, double value, const char* unit) {
    if (!buffer || buffer_size == 0) return;
    
    if (fabs(value) >= 1000000) {
        snprintf(buffer, buffer_size, "%.2fM %s", value / 1000000.0, unit ? unit : "");
    } else if (fabs(value) >= 1000) {
        snprintf(buffer, buffer_size, "%.2fK %s", value / 1000.0, unit ? unit : "");
    } else {
        snprintf(buffer, buffer_size, "%.2f %s", value, unit ? unit : "");
    }
}

const char* analytics_metric_type_to_string(analytics_metric_type_t type) {
    switch (type) {
        case ANALYTICS_METRIC_PLAYER_BEHAVIOR: return "Player Behavior";
        case ANALYTICS_METRIC_GAME_PERFORMANCE: return "Game Performance";
        case ANALYTICS_METRIC_SERVER_PERFORMANCE: return "Server Performance";
        case ANALYTICS_METRIC_ECONOMY: return "Economy";
        case ANALYTICS_METRIC_QUEST_COMPLETION: return "Quest Completion";
        case ANALYTICS_METRIC_COMBAT: return "Combat";
        case ANALYTICS_METRIC_SOCIAL: return "Social";
        default: return "Unknown";
    }
}

void analytics_debug_print_metric(const analytics_metric_t* metric) {
    if (!metric) return;
    
    printf("Metric: %s (%s)\n", metric->name, metric->metric_id);
    printf("Type: %s\n", analytics_metric_type_to_string(metric->type));
    printf("Description: %s\n", metric->description);
    printf("Current Value: %.2f %s\n", metric->current_value, metric->unit);
    printf("Average: %.2f, Min: %.2f, Max: %.2f\n", 
           metric->average_value, metric->min_value, metric->max_value);
    printf("Data Points: %d\n", metric->data_count);
}

// TODO: High - Implement a system for tracking player behavior, such as time spent in different game modes, items crafted, and mobs killed.
// TODO: High - Implement a system for tracking game performance, such as frame rate, memory usage, and loading times.
// TODO: Medium - Implement a system for tracking server performance, such as CPU usage, memory usage, and network latency.
// TODO: Low - Implement a system for A/B testing new features and content.
