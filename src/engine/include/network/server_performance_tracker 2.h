#ifndef SERVER_PERFORMANCE_TRACKER_H
#define SERVER_PERFORMANCE_TRACKER_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;

// Server performance metrics
typedef struct {
    // CPU metrics
    float cpu_usage_percent;
    u32 cpu_cores_available;
    u32 cpu_cores_active;
    
    // Memory metrics
    u64 total_memory_mb;
    u64 used_memory_mb;
    u64 available_memory_mb;
    float memory_usage_percent;
    
    // Network metrics
    float network_latency_ms;
    u32 packets_sent_per_second;
    u32 packets_received_per_second;
    u32 bytes_sent_per_second;
    u32 bytes_received_per_second;
    u32 active_connections;
    u32 max_connections;
    
    // Server-specific metrics
    float tick_rate;
    float tick_time_ms;
    u32 entities_processed_per_tick;
    u32 network_events_processed_per_tick;
    
    // Performance indicators
    float server_load_percentage;
    bool is_performance_healthy;
    u32 performance_warnings;
    
    // Timestamp
    u64 timestamp_ms;
} ServerPerformanceMetrics;

// Performance thresholds for alerts
typedef struct {
    float cpu_warning_threshold;
    float cpu_critical_threshold;
    float memory_warning_threshold;
    float memory_critical_threshold;
    float latency_warning_threshold;
    float latency_critical_threshold;
    float tick_time_warning_threshold;
    float tick_time_critical_threshold;
} PerformanceThresholds;

// Performance history
typedef struct {
    ServerPerformanceMetrics* metrics;
    u32 capacity;
    u32 count;
    u32 current_index;
    bool is_circular;
} PerformanceHistory;

// Performance alert
typedef enum {
    ALERT_NONE,
    ALERT_WARNING,
    ALERT_CRITICAL
} AlertLevel;

typedef struct {
    AlertLevel level;
    char message[256];
    u64 timestamp_ms;
    const char* metric_name;
    float current_value;
    float threshold_value;
} PerformanceAlert;

// Server performance tracker
typedef struct {
    ServerPerformanceMetrics current_metrics;
    PerformanceHistory history;
    PerformanceThresholds thresholds;
    
    PerformanceAlert* alerts;
    u32 alert_capacity;
    u32 alert_count;
    
    bool is_tracking;
    u64 last_update_time_ms;
    u32 update_interval_ms;
    
    // Callbacks for performance events
    void (*on_warning)(const PerformanceAlert* alert);
    void (*on_critical)(const PerformanceAlert* alert);
    void (*on_recovery)(const char* metric_name);
    
    void* user_data;
} ServerPerformanceTracker;

// MARK: - Server Performance Tracker Management

bool server_perf_tracker_init(ServerPerformanceTracker* tracker, u32 history_capacity, u32 alert_capacity);
void server_perf_tracker_shutdown(ServerPerformanceTracker* tracker);

bool server_perf_tracker_start(ServerPerformanceTracker* tracker);
void server_perf_tracker_stop(ServerPerformanceTracker* tracker);
void server_perf_tracker_update(ServerPerformanceTracker* tracker);

// MARK: - Metrics Collection

void server_perf_collect_cpu_metrics(ServerPerformanceTracker* tracker);
void server_perf_collect_memory_metrics(ServerPerformanceTracker* tracker);
void server_perf_collect_network_metrics(ServerPerformanceTracker* tracker);
void server_perf_collect_server_metrics(ServerPerformanceTracker* tracker);

// MARK: - Performance Analysis

bool server_perf_is_healthy(const ServerPerformanceTracker* tracker);
AlertLevel server_perf_get_worst_alert_level(const ServerPerformanceTracker* tracker);
u32 server_perf_get_active_alert_count(const ServerPerformanceTracker* tracker);

// MARK: - History Management

const ServerPerformanceMetrics* server_perf_get_current_metrics(const ServerPerformanceTracker* tracker);
const ServerPerformanceMetrics* server_perf_get_metrics_at_time(const ServerPerformanceTracker* tracker, u32 index);
const ServerPerformanceMetrics* server_perf_get_average_metrics(const ServerPerformanceTracker* tracker, u32 sample_count);

float server_perf_get_average_cpu_usage(const ServerPerformanceTracker* tracker, u32 sample_count);
float server_perf_get_average_memory_usage(const ServerPerformanceTracker* tracker, u32 sample_count);
float server_perf_get_average_latency(const ServerPerformanceTracker* tracker, u32 sample_count);

// MARK: - Alert Management

void server_perf_set_thresholds(ServerPerformanceTracker* tracker, const PerformanceThresholds* thresholds);
PerformanceThresholds server_perf_get_default_thresholds(void);

const PerformanceAlert* server_perf_get_alerts(const ServerPerformanceTracker* tracker, u32* count);
void server_perf_clear_alerts(ServerPerformanceTracker* tracker);

// MARK: - Configuration

void server_perf_set_update_interval(ServerPerformanceTracker* tracker, u32 interval_ms);
void server_perf_set_callbacks(ServerPerformanceTracker* tracker, 
                                void (*on_warning)(const PerformanceAlert*),
                                void (*on_critical)(const PerformanceAlert*),
                                void (*on_recovery)(const char*));

// MARK: - Utility Functions

void server_perf_print_metrics(const ServerPerformanceMetrics* metrics);
void server_perf_print_summary(const ServerPerformanceTracker* tracker);
void server_perf_export_to_csv(const ServerPerformanceTracker* tracker, const char* filename);

// MARK: - Platform-specific implementations (to be implemented per platform)

bool platform_get_cpu_usage(float* usage_percent, u32* cores_available, u32* cores_active);
bool platform_get_memory_usage(u64* total_mb, u64* used_mb, u64* available_mb);
bool platform_get_network_stats(u32* packets_sent, u32* packets_received, 
                                 u32* bytes_sent, u32* bytes_received, float* latency_ms);
bool platform_get_process_time(float* user_time_ms, float* system_time_ms);

#endif // SERVER_PERFORMANCE_TRACKER_H
