#include "analytics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/utsname.h>
#endif

// =================================================================================================
//                                   ANALYTICS & METRICS SYSTEM - IMPLEMENTATION
// =================================================================================================

// Analytics Manager
typedef struct {
    bool initialized;
    bool enabled;
    char user_id[64];
    char session_id[64];
    time_t session_start;
    
    // Event tracking
    AnalyticsEvent events[MAX_ANALYTICS_EVENTS];
    u32 event_count;
    pthread_mutex_t event_mutex;
    
    // Performance metrics
    PerformanceMetrics performance;
    
    // Engagement metrics
    EngagementMetrics engagement;
    
    // Heatmap data
    HeatmapData heatmaps[MAX_HEATMAPS];
    u32 heatmap_count;
    
    // Upload settings
    char api_endpoint[256];
    u32 upload_interval; // seconds
    time_t last_upload;
    
    // Offline mode
    bool offline_mode;
    char offline_cache_file[256];
    
} AnalyticsManager;

static AnalyticsManager g_analytics = {0};

// MARK: - Utility Functions

static void generate_session_id(void) {
    time_t now = time(NULL);
    snprintf(g_analytics.session_id, sizeof(g_analytics.session_id), "session_%ld_%d", 
             now, rand() % 10000);
}

static void generate_user_id(void) {
    // Try to get a persistent user ID from system info
#ifdef __APPLE__
    char hardware_uuid[64] = {0};
    size_t size = sizeof(hardware_uuid);
    if (sysctlbyname("kern.uuid", hardware_uuid, &size, NULL, 0) == 0) {
        strncpy(g_analytics.user_id, hardware_uuid, sizeof(g_analytics.user_id) - 1);
        return;
    }
#endif
    
    // Fallback to random ID
    snprintf(g_analytics.user_id, sizeof(g_analytics.user_id), "user_%ld_%d", 
             time(NULL), rand() % 10000);
}

static time_t get_current_timestamp(void) {
    return time(NULL);
}

// MARK: - System Initialization

bool analytics_init(const char* api_endpoint) {
    memset(&g_analytics, 0, sizeof(g_analytics));
    pthread_mutex_init(&g_analytics.event_mutex, NULL);
    
    // Set default API endpoint
    if (api_endpoint) {
        strncpy(g_analytics.api_endpoint, api_endpoint, sizeof(g_analytics.api_endpoint) - 1);
    } else {
        strcpy(g_analytics.api_endpoint, "https://api.minecraftv2.com/analytics");
    }
    
    // Generate IDs
    generate_user_id();
    generate_session_id();
    g_analytics.session_start = get_current_timestamp();
    
    // Set defaults
    g_analytics.upload_interval = 300; // 5 minutes
    g_analytics.last_upload = g_analytics.session_start;
    strcpy(g_analytics.offline_cache_file, "analytics_cache.json");
    
    // Initialize performance metrics
    g_analytics.performance.session_start_time = g_analytics.session_start;
    
    // Initialize engagement metrics
    g_analytics.engagement.session_start_time = g_analytics.session_start;
    
    g_analytics.initialized = true;
    g_analytics.enabled = true;
    
    printf("Analytics system initialized\n");
    return true;
}

void analytics_shutdown(void) {
    if (!g_analytics.initialized) return;
    
    // Final session tracking
    analytics_track_session_end();
    
    // Upload remaining events
    analytics_upload_events();
    
    pthread_mutex_destroy(&g_analytics.event_mutex);
    memset(&g_analytics, 0, sizeof(g_analytics));
    
    printf("Analytics system shutdown\n");
}

void analytics_set_enabled(bool enabled) {
    g_analytics.enabled = enabled;
}

void analytics_set_offline_mode(bool offline) {
    g_analytics.offline_mode = offline;
}

// MARK: - Event Tracking

void analytics_track_event(const char* event_name, const char* parameters) {
    if (!g_analytics.initialized || !g_analytics.enabled) return;
    
    pthread_mutex_lock(&g_analytics.event_mutex);
    
    if (g_analytics.event_count >= MAX_ANALYTICS_EVENTS) {
        // Remove oldest event
        for (u32 i = 0; i < g_analytics.event_count - 1; i++) {
            g_analytics.events[i] = g_analytics.events[i + 1];
        }
        g_analytics.event_count--;
    }
    
    AnalyticsEvent* event = &g_analytics.events[g_analytics.event_count];
    strncpy(event->event_name, event_name, sizeof(event->event_name) - 1);
    event->timestamp = get_current_timestamp();
    
    if (parameters) {
        strncpy(event->parameters, parameters, sizeof(event->parameters) - 1);
    }
    
    g_analytics.event_count++;
    
    pthread_mutex_unlock(&g_analytics.event_mutex);
}

void analytics_track_player_action(const char* action, const char* details) {
    char params[256];
    snprintf(params, sizeof(params), "action=%s,details=%s", action, details ? details : "");
    analytics_track_event("player_action", params);
}

void analytics_track_performance_metric(const char* metric_name, f64 value) {
    char params[128];
    snprintf(params, sizeof(params), "metric=%s,value=%.6f", metric_name, value);
    analytics_track_event("performance_metric", params);
}

void analytics_track_error(const char* error_type, const char* error_message) {
    char params[512];
    snprintf(params, sizeof(params), "type=%s,message=%s", error_type, error_message);
    analytics_track_event("error", params);
}

// MARK: - Session Tracking

void analytics_track_session_start(void) {
    if (!g_analytics.initialized || !g_analytics.enabled) return;
    
    g_analytics.session_start = get_current_timestamp();
    generate_session_id();
    
    char params[128];
    snprintf(params, sizeof(params), "session_id=%s,user_id=%s", 
             g_analytics.session_id, g_analytics.user_id);
    analytics_track_event("session_start", params);
}

void analytics_track_session_end(void) {
    if (!g_analytics.initialized || !g_analytics.enabled) return;
    
    time_t session_duration = get_current_timestamp() - g_analytics.session_start;
    
    char params[128];
    snprintf(params, sizeof(params), "session_id=%s,duration=%ld", 
             g_analytics.session_id, session_duration);
    analytics_track_event("session_end", params);
}

void analytics_update_session_duration(void) {
    if (!g_analytics.initialized) return;
    
    g_analytics.engagement.session_duration = get_current_timestamp() - g_analytics.session_start;
}

// MARK: - Performance Telemetry

void analytics_update_fps(f64 fps) {
    if (!g_analytics.initialized) return;
    
    g_analytics.performance.current_fps = fps;
    
    if (fps < g_analytics.performance.min_fps || g_analytics.performance.min_fps == 0) {
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
    if (!g_analytics.initialized) return;
    
    g_analytics.performance.memory_usage = memory_bytes;
    
    if (memory_bytes > g_analytics.performance.peak_memory) {
        g_analytics.performance.peak_memory = memory_bytes;
    }
    
    // Track high memory usage events
    u64 memory_mb = memory_bytes / (1024 * 1024);
    if (memory_mb > 1000) { // > 1GB
        analytics_track_performance_metric("high_memory", (f64)memory_mb);
    }
}

void analytics_update_gpu_usage(f64 gpu_utilization) {
    if (!g_analytics.initialized) return;
    
    g_analytics.performance.gpu_utilization = gpu_utilization;
}

// MARK: - Engagement Metrics

void analytics_track_quest_completion(const char* quest_id) {
    g_analytics.engagement.quests_completed++;
    
    char params[128];
    snprintf(params, sizeof(params), "quest_id=%s,total=%u", 
             quest_id, g_analytics.engagement.quests_completed);
    analytics_track_event("quest_completed", params);
}

void analytics_track_item_usage(const char* item_id, u32 count) {
    g_analytics.engagement.items_used += count;
    
    char params[128];
    snprintf(params, sizeof(params), "item_id=%s,count=%u,total=%u", 
             item_id, count, g_analytics.engagement.items_used);
    analytics_track_event("item_used", params);
}

void analytics_track_death(const char* death_type, f32 x, f32 y, f32 z) {
    g_analytics.engagement.deaths++;
    
    // Add to heatmap
    analytics_add_heatmap_point("deaths", x, y, z);
    
    char params[256];
    snprintf(params, sizeof(params), "type=%s,location=%.2f,%.2f,%.2f,total=%u", 
             death_type, x, y, z, g_analytics.engagement.deaths);
    analytics_track_event("player_death", params);
}

void analytics_track_level_up(u32 new_level) {
    g_analytics.engagement.current_level = new_level;
    
    char params[64];
    snprintf(params, sizeof(params), "level=%u", new_level);
    analytics_track_event("level_up", params);
}

// MARK: - Heatmap System

void analytics_add_heatmap_point(const char* heatmap_name, f32 x, f32 y, f32 z) {
    if (!g_analytics.initialized) return;
    
    // Find existing heatmap
    HeatmapData* heatmap = NULL;
    for (u32 i = 0; i < g_analytics.heatmap_count; i++) {
        if (strcmp(g_analytics.heatmaps[i].name, heatmap_name) == 0) {
            heatmap = &g_analytics.heatmaps[i];
            break;
        }
    }
    
    // Create new heatmap if not found
    if (!heatmap) {
        if (g_analytics.heatmap_count >= MAX_HEATMAPS) return;
        
        heatmap = &g_analytics.heatmaps[g_analytics.heatmap_count++];
        strncpy(heatmap->name, heatmap_name, sizeof(heatmap->name) - 1);
        heatmap->point_count = 0;
    }
    
    // Add point
    if (heatmap->point_count < MAX_HEATMAP_POINTS) {
        HeatmapPoint* point = &heatmap->points[heatmap->point_count++];
        point->x = x;
        point->y = y;
        point->z = z;
        point->timestamp = get_current_timestamp();
    }
}

// MARK: - Data Upload

bool analytics_upload_events(void) {
    if (!g_analytics.initialized || g_analytics.offline_mode) return false;
    
    pthread_mutex_lock(&g_analytics.event_mutex);
    
    if (g_analytics.event_count == 0) {
        pthread_mutex_unlock(&g_analytics.event_mutex);
        return true; // Nothing to upload
    }
    
    // In a real implementation, this would use HTTP to upload to the server
    // For now, we'll simulate the upload and cache to file
    printf("Uploading %u analytics events...\n", g_analytics.event_count);
    
    // Cache events to file for offline backup
    FILE* cache_file = fopen(g_analytics.offline_cache_file, "a");
    if (cache_file) {
        for (u32 i = 0; i < g_analytics.event_count; i++) {
            AnalyticsEvent* event = &g_analytics.events[i];
            fprintf(cache_file, "%ld,%s,%s,%s\n", 
                   event->timestamp, g_analytics.session_id, 
                   event->event_name, event->parameters);
        }
        fclose(cache_file);
    }
    
    // Clear uploaded events
    g_analytics.event_count = 0;
    g_analytics.last_upload = get_current_timestamp();
    
    pthread_mutex_unlock(&g_analytics.event_mutex);
    
    printf("Analytics upload completed\n");
    return true;
}

void analytics_process_offline_cache(void) {
    if (!g_analytics.initialized) return;
    
    FILE* cache_file = fopen(g_analytics.offline_cache_file, "r");
    if (!cache_file) return;
    
    printf("Processing offline analytics cache...\n");
    
    char line[512];
    while (fgets(line, sizeof(line), cache_file)) {
        // Parse cached event and re-queue for upload
        // Format: timestamp,session_id,event_name,parameters
        char* token = strtok(line, ",");
        if (!token) continue;
        
        time_t timestamp = atol(token);
        token = strtok(NULL, ",");
        if (!token) continue;
        
        char* session_id = token;
        token = strtok(NULL, ",");
        if (!token) continue;
        
        char* event_name = token;
        token = strtok(NULL, "\n");
        char* parameters = token ? token : "";
        
        // Re-queue event if it's from current session or recent
        time_t age = get_current_timestamp() - timestamp;
        if (age < 86400) { // Less than 24 hours old
            pthread_mutex_lock(&g_analytics.event_mutex);
            if (g_analytics.event_count < MAX_ANALYTICS_EVENTS) {
                AnalyticsEvent* event = &g_analytics.events[g_analytics.event_count];
                strncpy(event->event_name, event_name, sizeof(event->event_name) - 1);
                strncpy(event->parameters, parameters, sizeof(event->parameters) - 1);
                event->timestamp = timestamp;
                g_analytics.event_count++;
            }
            pthread_mutex_unlock(&g_analytics.event_mutex);
        }
    }
    
    fclose(cache_file);
    
    // Clear cache file
    remove(g_analytics.offline_cache_file);
    
    printf("Offline cache processing completed\n");
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
    pthread_mutex_lock(&g_analytics.event_mutex);
    u32 count = g_analytics.event_count;
    pthread_mutex_unlock(&g_analytics.event_mutex);
    return count;
}

// MARK: - A/B Testing Framework

void analytics_ab_test_init(const char* test_name, const char* variant) {
    char params[128];
    snprintf(params, sizeof(params), "test=%s,variant=%s", test_name, variant);
    analytics_track_event("ab_test_init", params);
}

void analytics_ab_track_conversion(const char* test_name, const char* variant) {
    char params[128];
    snprintf(params, sizeof(params), "test=%s,variant=%s,converted=true", test_name, variant);
    analytics_track_event("ab_test_conversion", params);
}

bool analytics_ab_get_variant(const char* test_name, char* variant, size_t variant_size) {
    // In a real implementation, this would query the server for the assigned variant
    // For now, we'll use a simple hash-based assignment
    u32 hash = 0;
    for (size_t i = 0; test_name[i]; i++) {
        hash = hash * 31 + test_name[i];
    }
    
    const char* variants[] = {"control", "variant_a", "variant_b"};
    u32 variant_index = hash % 3;
    
    strncpy(variant, variants[variant_index], variant_size - 1);
    return true;
}
