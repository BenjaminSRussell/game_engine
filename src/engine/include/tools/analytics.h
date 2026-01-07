#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// =================================================================================================
//                                   ANALYTICS & METRICS SYSTEM API
// =================================================================================================

// Forward declarations
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

// Constants
#define MAX_ANALYTICS_EVENTS 1024
#define MAX_HEATMAPS 16
#define MAX_HEATMAP_POINTS 10000

// Event tracking
typedef struct {
    char event_name[128];
    char parameters[256];
    time_t timestamp;
} AnalyticsEvent;

// Performance metrics
typedef struct {
    f64 current_fps;
    f64 min_fps;
    f64 max_fps;
    u64 memory_usage;
    u64 peak_memory;
    f64 gpu_utilization;
    time_t session_start_time;
} PerformanceMetrics;

// Engagement metrics
typedef struct {
    u32 quests_completed;
    u32 items_used;
    u32 deaths;
    u32 current_level;
    time_t session_start_time;
    time_t session_duration;
} EngagementMetrics;

// Heatmap data
typedef struct {
    f32 x, y, z;
    time_t timestamp;
} HeatmapPoint;

typedef struct {
    char name[64];
    HeatmapPoint points[MAX_HEATMAP_POINTS];
    u32 point_count;
} HeatmapData;

// MARK: - System Management

bool analytics_init(const char* api_endpoint);
void analytics_shutdown(void);
void analytics_set_enabled(bool enabled);
void analytics_set_offline_mode(bool offline);

// MARK: - Event Tracking

void analytics_track_event(const char* event_name, const char* parameters);
void analytics_track_player_action(const char* action, const char* details);
void analytics_track_performance_metric(const char* metric_name, f64 value);
void analytics_track_error(const char* error_type, const char* error_message);

// MARK: - Session Tracking

void analytics_track_session_start(void);
void analytics_track_session_end(void);
void analytics_update_session_duration(void);

// MARK: - Performance Telemetry

void analytics_update_fps(f64 fps);
void analytics_update_memory_usage(u64 memory_bytes);
void analytics_update_gpu_usage(f64 gpu_utilization);

// MARK: - Engagement Metrics

void analytics_track_quest_completion(const char* quest_id);
void analytics_track_item_usage(const char* item_id, u32 count);
void analytics_track_death(const char* death_type, f32 x, f32 y, f32 z);
void analytics_track_level_up(u32 new_level);

// MARK: - Heatmap System

void analytics_add_heatmap_point(const char* heatmap_name, f32 x, f32 y, f32 z);

// MARK: - Data Upload

bool analytics_upload_events(void);
void analytics_process_offline_cache(void);

// MARK: - Data Retrieval

PerformanceMetrics analytics_get_performance_metrics(void);
EngagementMetrics analytics_get_engagement_metrics(void);
u32 analytics_get_event_count(void);

// MARK: - A/B Testing Framework

void analytics_ab_test_init(const char* test_name, const char* variant);
void analytics_ab_track_conversion(const char* test_name, const char* variant);
bool analytics_ab_get_variant(const char* test_name, char* variant, size_t variant_size);

#endif // ANALYTICS_H
