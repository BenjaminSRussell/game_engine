// Game analytics and performance tracking implementation
#include "tools/analytics.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Global analytics state
static struct {
    bool initialized;
    bool enabled;
    bool offline_mode;
    char api_endpoint[256];
    
    AnalyticsEvent events[MAX_ANALYTICS_EVENTS];
    u32 event_count;
    
    PerformanceMetrics performance;
    EngagementMetrics engagement;
    
    HeatmapData heatmaps[MAX_HEATMAPS];
    u32 heatmap_count;
    
    time_t session_start;
    bool session_active;
} g_analytics = {0};

// Helper function to get current timestamp
static time_t get_current_time(void) {
    return time(NULL);
}

// Helper function to find or create heatmap
static HeatmapData* get_heatmap(const char* name) {
    for (u32 i = 0; i < g_analytics.heatmap_count; i++) {
        if (strcmp(g_analytics.heatmaps[i].name, name) == 0) {
            return &g_analytics.heatmaps[i];
        }
    }
    
    if (g_analytics.heatmap_count < MAX_HEATMAPS) {
        HeatmapData* heatmap = &g_analytics.heatmaps[g_analytics.heatmap_count++];
        strncpy(heatmap->name, name, sizeof(heatmap->name) - 1);
        heatmap->point_count = 0;
        return heatmap;
    }
    
    return NULL;
}

// MARK: - System Management

bool analytics_init(const char* api_endpoint) {
    if (g_analytics.initialized) {
        return true;
    }
    
    memset(&g_analytics, 0, sizeof(g_analytics));
    g_analytics.enabled = true;
    g_analytics.offline_mode = false;
    
    if (api_endpoint) {
        strncpy(g_analytics.api_endpoint, api_endpoint, sizeof(g_analytics.api_endpoint) - 1);
    }
    
    // Initialize performance metrics
    g_analytics.performance.current_fps = 60.0;
    g_analytics.performance.min_fps = 60.0;
    g_analytics.performance.max_fps = 60.0;
    g_analytics.performance.memory_usage = 0;
    g_analytics.performance.peak_memory = 0;
    g_analytics.performance.gpu_utilization = 0.0;
    
    // Initialize engagement metrics
    g_analytics.engagement.quests_completed = 0;
    g_analytics.engagement.items_used = 0;
    g_analytics.engagement.deaths = 0;
    g_analytics.engagement.current_level = 1;
    
    g_analytics.initialized = true;
    LOG_INFO("Analytics system initialized");
    return true;
}

void analytics_shutdown(void) {
    if (!g_analytics.initialized) {
        return;
    }
    
    if (g_analytics.session_active) {
        analytics_track_session_end();
    }
    
    g_analytics.initialized = false;
    LOG_INFO("Analytics system shutdown");
}

void analytics_set_enabled(bool enabled) {
    g_analytics.enabled = enabled;
    LOG_INFO("Analytics %s", enabled ? "enabled" : "disabled");
}

void analytics_set_offline_mode(bool offline) {
    g_analytics.offline_mode = offline;
    LOG_INFO("Analytics offline mode %s", offline ? "enabled" : "disabled");
}

// MARK: - Event Tracking

void analytics_track_event(const char* event_name, const char* parameters) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    if (g_analytics.event_count >= MAX_ANALYTICS_EVENTS) {
        // Remove oldest event
        memmove(&g_analytics.events[0], &g_analytics.events[1], 
                sizeof(AnalyticsEvent) * (MAX_ANALYTICS_EVENTS - 1));
        g_analytics.event_count--;
    }
    
    AnalyticsEvent* event = &g_analytics.events[g_analytics.event_count++];
    strncpy(event->event_name, event_name, sizeof(event->event_name) - 1);
    
    if (parameters) {
        strncpy(event->parameters, parameters, sizeof(event->parameters) - 1);
    } else {
        event->parameters[0] = '\0';
    }
    
    event->timestamp = get_current_time();
}

void analytics_track_player_action(const char* action, const char* details) {
    char params[256];
    snprintf(params, sizeof(params), "action=%s,details=%s", action, details ? details : "");
    analytics_track_event("player_action", params);
}

void analytics_track_performance_metric(const char* metric_name, f64 value) {
    char params[256];
    snprintf(params, sizeof(params), "metric=%s,value=%.6f", metric_name, value);
    analytics_track_event("performance_metric", params);
}

void analytics_track_error(const char* error_type, const char* error_message) {
    char params[256];
    snprintf(params, sizeof(params), "type=%s,message=%s", error_type, error_message ? error_message : "");
    analytics_track_event("error", params);
}

// MARK: - Session Tracking

void analytics_track_session_start(void) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    g_analytics.session_start = get_current_time();
    g_analytics.session_active = true;
    g_analytics.engagement.session_start_time = g_analytics.session_start;
    
    analytics_track_event("session_start", "");
    LOG_INFO("Analytics session started");
}

void analytics_track_session_end(void) {
    if (!g_analytics.initialized || !g_analytics.enabled || !g_analytics.session_active) {
        return;
    }
    
    time_t session_end = get_current_time();
    g_analytics.engagement.session_duration = session_end - g_analytics.session_start;
    g_analytics.session_active = false;
    
    char params[256];
    snprintf(params, sizeof(params), "duration=%ld", g_analytics.engagement.session_duration);
    analytics_track_event("session_end", params);
    LOG_INFO("Analytics session ended (duration: %ld seconds)", g_analytics.engagement.session_duration);
}

void analytics_update_session_duration(void) {
    if (g_analytics.session_active) {
        g_analytics.engagement.session_duration = get_current_time() - g_analytics.session_start;
    }
}

// MARK: - Performance Telemetry

void analytics_update_fps(f64 fps) {
    if (!g_analytics.initialized) {
        return;
    }
    
    g_analytics.performance.current_fps = fps;
    
    if (fps < g_analytics.performance.min_fps) {
        g_analytics.performance.min_fps = fps;
    }
    if (fps > g_analytics.performance.max_fps) {
        g_analytics.performance.max_fps = fps;
    }
    
    // Track low FPS events
    if (fps < 30.0) {
        analytics_track_performance_metric("low_fps", fps);
    }
}

void analytics_update_memory_usage(u64 memory_bytes) {
    if (!g_analytics.initialized) {
        return;
    }
    
    g_analytics.performance.memory_usage = memory_bytes;
    
    if (memory_bytes > g_analytics.performance.peak_memory) {
        g_analytics.performance.peak_memory = memory_bytes;
    }
    
    // Track high memory usage
    if (memory_bytes > 1024 * 1024 * 1024) { // > 1GB
        analytics_track_performance_metric("high_memory", (f64)memory_bytes);
    }
}

void analytics_update_gpu_usage(f64 gpu_utilization) {
    if (!g_analytics.initialized) {
        return;
    }
    
    g_analytics.performance.gpu_utilization = gpu_utilization;
    
    // Track high GPU usage
    if (gpu_utilization > 90.0) {
        analytics_track_performance_metric("high_gpu", gpu_utilization);
    }
}

// MARK: - Engagement Metrics

void analytics_track_quest_completion(const char* quest_id) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    g_analytics.engagement.quests_completed++;
    
    char params[256];
    snprintf(params, sizeof(params), "quest_id=%s,total_completed=%u", 
             quest_id, g_analytics.engagement.quests_completed);
    analytics_track_event("quest_completed", params);
}

void analytics_track_item_usage(const char* item_id, u32 count) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    g_analytics.engagement.items_used += count;
    
    char params[256];
    snprintf(params, sizeof(params), "item_id=%s,count=%u,total_used=%u", 
             item_id, count, g_analytics.engagement.items_used);
    analytics_track_event("item_used", params);
}

void analytics_track_death(const char* death_type, f32 x, f32 y, f32 z) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    g_analytics.engagement.deaths++;
    
    char params[256];
    snprintf(params, sizeof(params), "type=%s,position=%.2f,%.2f,%.2f,total_deaths=%u", 
             death_type, x, y, z, g_analytics.engagement.deaths);
    analytics_track_event("player_death", params);
    
    // Add death location to heatmap
    analytics_add_heatmap_point("death_locations", x, y, z);
}

void analytics_track_level_up(u32 new_level) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    g_analytics.engagement.current_level = new_level;
    
    char params[256];
    snprintf(params, sizeof(params), "new_level=%u", new_level);
    analytics_track_event("level_up", params);
}

// MARK: - Heatmap System

void analytics_add_heatmap_point(const char* heatmap_name, f32 x, f32 y, f32 z) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    HeatmapData* heatmap = get_heatmap(heatmap_name);
    if (!heatmap) {
        return;
    }
    
    if (heatmap->point_count >= MAX_HEATMAP_POINTS) {
        // Remove oldest point
        memmove(&heatmap->points[0], &heatmap->points[1], 
                sizeof(HeatmapPoint) * (MAX_HEATMAP_POINTS - 1));
        heatmap->point_count--;
    }
    
    HeatmapPoint* point = &heatmap->points[heatmap->point_count++];
    point->x = x;
    point->y = y;
    point->z = z;
    point->timestamp = get_current_time();
}

// MARK: - Data Upload

bool analytics_upload_events(void) {
    if (!g_analytics.initialized || g_analytics.offline_mode) {
        return false;
    }
    
    // TODO: Implement actual HTTP upload to analytics endpoint
    // For now, just log the upload attempt
    LOG_INFO("Uploading %u analytics events to %s", g_analytics.event_count, g_analytics.api_endpoint);
    
    // Clear events after successful upload
    g_analytics.event_count = 0;
    return true;
}

void analytics_process_offline_cache(void) {
    if (!g_analytics.initialized) {
        return;
    }
    
    // TODO: Implement offline cache processing
    // For now, just log the attempt
    LOG_INFO("Processing offline analytics cache");
}

// MARK: - Data Retrieval

PerformanceMetrics analytics_get_performance_metrics(void) {
    return g_analytics.performance;
}

EngagementMetrics analytics_get_engagement_metrics(void) {
    analytics_update_session_duration();
    return g_analytics.engagement;
}

u32 analytics_get_event_count(void) {
    return g_analytics.event_count;
}

// MARK: - A/B Testing Framework

void analytics_ab_test_init(const char* test_name, const char* variant) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    char params[256];
    snprintf(params, sizeof(params), "test=%s,variant=%s", test_name, variant);
    analytics_track_event("ab_test_init", params);
}

void analytics_ab_track_conversion(const char* test_name, const char* variant) {
    if (!g_analytics.initialized || !g_analytics.enabled) {
        return;
    }
    
    char params[256];
    snprintf(params, sizeof(params), "test=%s,variant=%s", test_name, variant);
    analytics_track_event("ab_test_conversion", params);
}

bool analytics_ab_get_variant(const char* test_name, char* variant, size_t variant_size) {
    // TODO: Implement A/B test variant assignment logic
    // For now, just return a default variant
    strncpy(variant, "control", variant_size - 1);
    return true;
}
