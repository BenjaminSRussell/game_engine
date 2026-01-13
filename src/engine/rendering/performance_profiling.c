// src/engine/rendering/profiling/performance_profiling.c
// Performance Profiling Tools - Real-time performance monitoring and analysis

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Performance Profiling Types
// ============================================================================

typedef enum {
    PROFILE_TYPE_CPU,
    PROFILE_TYPE_GPU,
    PROFILE_TYPE_MEMORY,
    PROFILE_TYPE_RENDER,
    PROFILE_TYPE_SHADER,
    PROFILE_TYPE_TEXTURE,
    PROFILE_TYPE_MESH,
    PROFILE_TYPE_AUDIO,
    PROFILE_TYPE_PHYSICS,
    PROFILE_TYPE_COUNT
} ProfileType;

typedef enum {
    METRIC_TYPE_TIME,
    METRIC_TYPE_MEMORY,
    METRIC_TYPE_COUNT,
    METRIC_TYPE_RATE,
    METRIC_TYPE_PERCENTAGE,
    METRIC_TYPE_CUSTOM,
    METRIC_TYPE_COUNT
} MetricType;

typedef struct {
    char name[256];
    ProfileType type;
    MetricType metric_type;
    
    // Timing data
    uint64_t start_time;
    uint64_t end_time;
    float duration_ms;
    float average_time_ms;
    float min_time_ms;
    float max_time_ms;
    uint64_t call_count;
    
    // Memory data
    uint64_t memory_usage;
    uint64_t peak_memory;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    
    // Rate data
    float current_rate;
    float average_rate;
    float peak_rate;
    uint64_t total_samples;
    
    // Percentage data
    float current_percentage;
    float average_percentage;
    float peak_percentage;
    
    // Custom data
    float custom_value;
    float custom_average;
    float custom_min;
    float custom_max;
    
    // Performance
    bool is_active;
    bool track_history;
    uint32_t history_size;
    float *history_data;
    uint32_t history_index;
    
    // Statistics
    float variance;
    float standard_deviation;
    float percentile_95;
    float percentile_99;
} ProfileMetric;

typedef struct {
    ProfileMetric *metrics[256];
    uint32_t metric_count;
    uint32_t metric_capacity;
    
    // Global performance
    float frame_time_ms;
    float fps;
    float cpu_usage;
    float gpu_usage;
    uint64_t memory_usage;
    uint64_t memory_budget;
    
    // Frame timing
    uint64_t frame_start_time;
    uint64_t frame_end_time;
    float frame_times[60]; // Last 60 frames
    uint32_t frame_index;
    
    // Performance budget
    float target_frame_time_ms;
    float target_fps;
    uint64_t target_memory_usage;
    
    // Alerts
    bool enable_alerts;
    float performance_threshold;
    float memory_threshold;
    bool performance_alert;
    bool memory_alert;
    
    // Thread safety
    pthread_mutex_t profile_mutex;
    
    bool initialized;
} PerformanceProfiler;

static PerformanceProfiler g_profiler = {0};

// ============================================================================
// Profiling Functions
// ============================================================================

static void update_statistics(ProfileMetric *metric) {
    if (!metric || metric->call_count == 0) return;
    
    // Calculate variance and standard deviation
    float sum = 0.0f;
    float sum_squared = 0.0f;
    
    for (uint32_t i = 0; i < metric->history_size && i < metric->call_count; i++) {
        float value = metric->history_data[i];
        sum += value;
        sum_squared += value * value;
    }
    
    float mean = sum / metric->call_count;
    metric->variance = (sum_squared / metric->call_count) - (mean * mean);
    metric->standard_deviation = sqrtf(metric->variance);
    
    // Calculate percentiles (simplified - using sorted data would be more accurate)
    metric->percentile_95 = mean + 1.96f * metric->standard_deviation;
    metric->percentile_99 = mean + 2.58f * metric->standard_deviation;
}

static void add_to_history(ProfileMetric *metric, float value) {
    if (!metric || !metric->track_history) return;
    
    metric->history_data[metric->history_index] = value;
    metric->history_index = (metric->history_index + 1) % metric->history_size;
    
    // Update min/max
    if (value < metric->min_time_ms) metric->min_time_ms = value;
    if (value > metric->max_time_ms) metric->max_time_ms = value;
}

static void update_frame_statistics(void) {
    // Calculate average frame time and FPS
    float total_frame_time = 0.0f;
    for (uint32_t i = 0; i < 60; i++) {
        total_frame_time += g_profiler.frame_times[i];
    }
    
    g_profiler.frame_time_ms = total_frame_time / 60.0f;
    g_profiler.fps = 1000.0f / g_profiler.frame_time_ms;
    
    // Check performance alerts
    if (g_profiler.enable_alerts) {
        g_profiler.performance_alert = (g_profiler.frame_time_ms > g_profiler.performance_threshold);
        g_profiler.memory_alert = (g_profiler.memory_usage > g_profiler.memory_threshold);
    }
}

// ============================================================================
// Performance Profiler API
// ============================================================================

bool performance_profiler_init(uint32_t max_metrics, float target_fps, uint64_t memory_budget, bool enable_alerts) {
    if (g_profiler.initialized) {
        LOG_WARN("Performance profiler already initialized");
        return true;
    }
    
    memset(&g_profiler, 0, sizeof(PerformanceProfiler));
    
    g_profiler.metric_capacity = max_metrics;
    g_profiler.target_fps = target_fps;
    g_profiler.target_frame_time_ms = 1000.0f / target_fps;
    g_profiler.target_memory_usage = memory_budget;
    g_profiler.memory_budget = memory_budget;
    g_profiler.enable_alerts = enable_alerts;
    
    g_profiler.performance_threshold = g_profiler.target_frame_time_ms * 1.2f; // 20% over target
    g_profiler.memory_threshold = memory_budget * 0.9f; // 90% of budget
    
    if (pthread_mutex_init(&g_profiler.profile_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize profiler mutex");
        return false;
    }
    
    // Initialize frame timing
    g_profiler.frame_start_time = get_time_nanos();
    
    g_profiler.initialized = true;
    LOG_INFO("Performance profiler initialized (metrics: %u, target_fps: %.1f, memory: %.1f MB, alerts: %s)",
             max_metrics, target_fps, memory_budget / (1024.0f * 1024.0f), enable_alerts ? "yes" : "no");
    return true;
}

void performance_profiler_shutdown(void) {
    if (!g_profiler.initialized)
        return;
    
    LOG_INFO("Shutting down performance profiler");
    
    // Destroy all metrics
    for (uint32_t i = 0; i < g_profiler.metric_count; i++) {
        if (g_profiler.metrics[i]) {
            performance_metric_destroy(g_profiler.metrics[i]);
        }
    }
    
    pthread_mutex_destroy(&g_profiler.profile_mutex);
    
    memset(&g_profiler, 0, sizeof(PerformanceProfiler));
    
    LOG_INFO("Performance profiler shutdown complete");
}

ProfileMetric *performance_metric_create(const char *name, ProfileType type, MetricType metric_type, bool track_history, uint32_t history_size) {
    if (!g_profiler.initialized || !name) {
        LOG_ERROR("Performance profiler not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_profiler.profile_mutex);
    
    if (g_profiler.metric_count >= g_profiler.metric_capacity) {
        LOG_ERROR("Too many performance metrics");
        pthread_mutex_unlock(&g_profiler.profile_mutex);
        return NULL;
    }
    
    ProfileMetric *metric = calloc(1, sizeof(ProfileMetric));
    if (!metric) {
        LOG_ERROR("Failed to allocate performance metric");
        pthread_mutex_unlock(&g_profiler.profile_mutex);
        return NULL;
    }
    
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->type = type;
    metric->metric_type = metric_type;
    metric->is_active = true;
    metric->track_history = track_history;
    metric->history_size = history_size;
    
    // Initialize history buffer
    if (track_history && history_size > 0) {
        metric->history_data = calloc(history_size, sizeof(float));
        if (!metric->history_data) {
            LOG_ERROR("Failed to allocate metric history buffer");
            free(metric);
            pthread_mutex_unlock(&g_profiler.profile_mutex);
            return NULL;
        }
    }
    
    // Initialize min/max values
    metric->min_time_ms = FLT_MAX;
    metric->max_time_ms = 0.0f;
    metric->custom_min = FLT_MAX;
    metric->custom_max = 0.0f;
    
    g_profiler.metrics[g_profiler.metric_count++] = metric;
    
    pthread_mutex_unlock(&g_profiler.profile_mutex);
    
    LOG_INFO("Created performance metric: %s (type: %d, metric: %d, history: %s, size: %u)",
             name, (int)type, (int)metric_type, track_history ? "yes" : "no", history_size);
    return metric;
}

void performance_metric_destroy(ProfileMetric *metric) {
    if (!metric) return;
    
    pthread_mutex_lock(&g_profiler.profile_mutex);
    
    // Remove from profiler
    for (uint32_t i = 0; i < g_profiler.metric_count; i++) {
        if (g_profiler.metrics[i] == metric) {
            g_profiler.metrics[i] = g_profiler.metrics[g_profiler.metric_count - 1];
            g_profiler.metric_count--;
            break;
        }
    }
    
    // Free history buffer
    free(metric->history_data);
    
    free(metric);
    
    pthread_mutex_unlock(&g_profiler.profile_mutex);
    
    LOG_DEBUG("Destroyed performance metric: %s", metric->name);
}

void performance_metric_start(ProfileMetric *metric) {
    if (!metric || !metric->is_active) return;
    
    metric->start_time = get_time_nanos();
}

void performance_metric_end(ProfileMetric *metric) {
    if (!metric || !metric->is_active) return;
    
    metric->end_time = get_time_nanos();
    metric->duration_ms = nanos_to_ms(metric->end_time - metric->start_time);
    
    // Update statistics
    metric->call_count++;
    metric->average_time_ms = (metric->average_time_ms * (metric->call_count - 1) + metric->duration_ms) / metric->call_count;
    
    // Add to history
    add_to_history(metric, metric->duration_ms);
    
    // Update advanced statistics
    update_statistics(metric);
}

void performance_metric_record_memory(ProfileMetric *metric, uint64_t memory_usage) {
    if (!metric || !metric->is_active) return;
    
    metric->memory_usage = memory_usage;
    metric->memory_allocations++;
    
    if (memory_usage > metric->peak_memory) {
        metric->peak_memory = memory_usage;
    }
    
    // Add to history
    add_to_history(metric, (float)memory_usage);
}

void performance_metric_record_rate(ProfileMetric *metric, float rate) {
    if (!metric || !metric->is_active) return;
    
    metric->current_rate = rate;
    metric->total_samples++;
    metric->average_rate = (metric->average_rate * (metric->total_samples - 1) + rate) / metric->total_samples;
    
    if (rate > metric->peak_rate) {
        metric->peak_rate = rate;
    }
    
    // Add to history
    add_to_history(metric, rate);
}

void performance_metric_record_percentage(ProfileMetric *metric, float percentage) {
    if (!metric || !metric->is_active) return;
    
    metric->current_percentage = percentage;
    metric->average_percentage = (metric->average_percentage * (metric->call_count - 1) + percentage) / metric->call_count;
    
    if (percentage > metric->peak_percentage) {
        metric->peak_percentage = percentage;
    }
    
    // Add to history
    add_to_history(metric, percentage);
}

void performance_metric_record_custom(ProfileMetric *metric, float value) {
    if (!metric || !metric->is_active) return;
    
    metric->custom_value = value;
    metric->call_count++;
    metric->custom_average = (metric->custom_average * (metric->call_count - 1) + value) / metric->call_count;
    
    if (value < metric->custom_min) metric->custom_min = value;
    if (value > metric->custom_max) metric->custom_max = value;
    
    // Add to history
    add_to_history(metric, value);
}

void performance_profiler_begin_frame(void) {
    if (!g_profiler.initialized) return;
    
    g_profiler.frame_start_time = get_time_nanos();
}

void performance_profiler_end_frame(void) {
    if (!g_profiler.initialized) return;
    
    g_profiler.frame_end_time = get_time_nanos();
    float frame_time = nanos_to_ms(g_profiler.frame_end_time - g_profiler.frame_start_time);
    
    // Update frame history
    g_profiler.frame_times[g_profiler.frame_index] = frame_time;
    g_profiler.frame_index = (g_profiler.frame_index + 1) % 60;
    
    // Update frame statistics
    update_frame_statistics();
    
    LOG_DEBUG("Frame time: %.2f ms (%.1f FPS)", frame_time, g_profiler.fps);
}

void performance_profiler_update_system_metrics(float cpu_usage, float gpu_usage, uint64_t memory_usage) {
    if (!g_profiler.initialized) return;
    
    g_profiler.cpu_usage = cpu_usage;
    g_profiler.gpu_usage = gpu_usage;
    g_profiler.memory_usage = memory_usage;
    
    // Update global metrics
    ProfileMetric *cpu_metric = performance_metric_find("CPU Usage");
    if (cpu_metric) {
        performance_metric_record_percentage(cpu_metric, cpu_usage);
    }
    
    ProfileMetric *gpu_metric = performance_metric_find("GPU Usage");
    if (gpu_metric) {
        performance_metric_record_percentage(gpu_metric, gpu_usage);
    }
    
    ProfileMetric *memory_metric = performance_metric_find("Memory Usage");
    if (memory_metric) {
        performance_metric_record_memory(memory_metric, memory_usage);
    }
}

ProfileMetric *performance_metric_find(const char *name) {
    if (!g_profiler.initialized || !name) return NULL;
    
    for (uint32_t i = 0; i < g_profiler.metric_count; i++) {
        if (strcmp(g_profiler.metrics[i]->name, name) == 0) {
            return g_profiler.metrics[i];
        }
    }
    
    return NULL;
}

void performance_metric_get_statistics(ProfileMetric *metric, float *average, float *min, float *max, float *std_dev) {
    if (!metric) return;
    
    if (average) *average = metric->average_time_ms;
    if (min) *min = metric->min_time_ms;
    if (max) *max = metric->max_time_ms;
    if (std_dev) *std_dev = metric->standard_deviation;
}

void performance_metric_get_memory_stats(ProfileMetric *metric, uint64_t *current, uint64_t *peak, uint64_t *allocations) {
    if (!metric) return;
    
    if (current) *current = metric->memory_usage;
    if (peak) *peak = metric->peak_memory;
    if (allocations) *allocations = metric->memory_allocations;
}

void performance_metric_get_rate_stats(ProfileMetric *metric, float *current, float *average, float *peak) {
    if (!metric) return;
    
    if (current) *current = metric->current_rate;
    if (average) *average = metric->average_rate;
    if (peak) *peak = metric->peak_rate;
}

void performance_profiler_get_frame_stats(float *frame_time, float *fps, uint32_t *frame_count) {
    if (!g_profiler.initialized) return;
    
    if (frame_time) *frame_time = g_profiler.frame_time_ms;
    if (fps) *fps = g_profiler.fps;
    if (frame_count) *frame_count = g_profiler.frame_index;
}

void performance_profiler_get_system_stats(float *cpu_usage, float *gpu_usage, uint64_t *memory_usage, uint64_t *memory_budget) {
    if (!g_profiler.initialized) return;
    
    if (cpu_usage) *cpu_usage = g_profiler.cpu_usage;
    if (gpu_usage) *gpu_usage = g_profiler.gpu_usage;
    if (memory_usage) *memory_usage = g_profiler.memory_usage;
    if (memory_budget) *memory_budget = g_profiler.memory_budget;
}

void performance_profiler_get_alerts(bool *performance_alert, bool *memory_alert) {
    if (!g_profiler.initialized) return;
    
    if (performance_alert) *performance_alert = g_profiler.performance_alert;
    if (memory_alert) *memory_alert = g_profiler.memory_alert;
}

void performance_profiler_set_thresholds(float performance_threshold, uint64_t memory_threshold) {
    if (!g_profiler.initialized) return;
    
    g_profiler.performance_threshold = performance_threshold;
    g_profiler.memory_threshold = memory_threshold;
    
    LOG_DEBUG("Updated thresholds: performance=%.2f ms, memory=%.1f MB", 
             performance_threshold, memory_threshold / (1024.0f * 1024.0f));
}

void performance_profiler_enable_alerts(bool enable) {
    if (!g_profiler.initialized) return;
    
    g_profiler.enable_alerts = enable;
    
    LOG_DEBUG("Performance alerts: %s", enable ? "enabled" : "disabled");
}

void performance_profiler_reset_metric(ProfileMetric *metric) {
    if (!metric) return;
    
    metric->start_time = 0;
    metric->end_time = 0;
    metric->duration_ms = 0.0f;
    metric->average_time_ms = 0.0f;
    metric->min_time_ms = FLT_MAX;
    metric->max_time_ms = 0.0f;
    metric->call_count = 0;
    
    metric->memory_usage = 0;
    metric->peak_memory = 0;
    metric->memory_allocations = 0;
    metric->memory_deallocations = 0;
    
    metric->current_rate = 0.0f;
    metric->average_rate = 0.0f;
    metric->peak_rate = 0.0f;
    metric->total_samples = 0;
    
    metric->current_percentage = 0.0f;
    metric->average_percentage = 0.0f;
    metric->peak_percentage = 0.0f;
    
    metric->custom_value = 0.0f;
    metric->custom_average = 0.0f;
    metric->custom_min = FLT_MAX;
    metric->custom_max = 0.0f;
    
    metric->variance = 0.0f;
    metric->standard_deviation = 0.0f;
    metric->percentile_95 = 0.0f;
    metric->percentile_99 = 0.0f;
    
    // Clear history
    if (metric->track_history && metric->history_data) {
        memset(metric->history_data, 0, metric->history_size * sizeof(float));
    }
    metric->history_index = 0;
    
    LOG_DEBUG("Reset performance metric: %s", metric->name);
}

void performance_profiler_reset_all_metrics(void) {
    if (!g_profiler.initialized) return;
    
    for (uint32_t i = 0; i < g_profiler.metric_count; i++) {
        performance_profiler_reset_metric(g_profiler.metrics[i]);
    }
    
    // Reset frame statistics
    memset(g_profiler.frame_times, 0, sizeof(g_profiler.frame_times));
    g_profiler.frame_index = 0;
    g_profiler.frame_time_ms = 0.0f;
    g_profiler.fps = 0.0f;
    
    LOG_DEBUG("Reset all performance metrics");
}

void performance_profiler_dump_stats(void) {
    if (!g_profiler.initialized) return;
    
    LOG_INFO("=== Performance Profiler Statistics ===");
    LOG_INFO("Frame Time: %.2f ms (%.1f FPS)", g_profiler.frame_time_ms, g_profiler.fps);
    LOG_INFO("CPU Usage: %.1f%%", g_profiler.cpu_usage);
    LOG_INFO("GPU Usage: %.1f%%", g_profiler.gpu_usage);
    LOG_INFO("Memory Usage: %.1f MB / %.1f MB", 
             g_profiler.memory_usage / (1024.0f * 1024.0f), 
             g_profiler.memory_budget / (1024.0f * 1024.0f));
    
    LOG_INFO("Performance Alert: %s", g_profiler.performance_alert ? "YES" : "NO");
    LOG_INFO("Memory Alert: %s", g_profiler.memory_alert ? "YES" : "NO");
    
    LOG_INFO("=== Metrics (%u) ===", g_profiler.metric_count);
    for (uint32_t i = 0; i < g_profiler.metric_count; i++) {
        ProfileMetric *metric = g_profiler.metrics[i];
        LOG_INFO("Metric: %s", metric->name);
        LOG_INFO("  Type: %d, Metric: %d, Calls: %llu", (int)metric->type, (int)metric->metric_type, metric->call_count);
        
        switch (metric->metric_type) {
            case METRIC_TYPE_TIME:
                LOG_INFO("  Time: avg=%.2f ms, min=%.2f ms, max=%.2f ms, std=%.2f ms",
                         metric->average_time_ms, metric->min_time_ms, metric->max_time_ms, metric->standard_deviation);
                break;
                
            case METRIC_TYPE_MEMORY:
                LOG_INFO("  Memory: current=%.1f MB, peak=%.1f MB, allocs=%llu",
                         metric->memory_usage / (1024.0f * 1024.0f), 
                         metric->peak_memory / (1024.0f * 1024.0f), 
                         metric->memory_allocations);
                break;
                
            case METRIC_TYPE_RATE:
                LOG_INFO("  Rate: current=%.1f, avg=%.1f, peak=%.1f",
                         metric->current_rate, metric->average_rate, metric->peak_rate);
                break;
                
            case METRIC_TYPE_PERCENTAGE:
                LOG_INFO("  Percentage: current=%.1f%%, avg=%.1f%%, peak=%.1f%%",
                         metric->current_percentage, metric->average_percentage, metric->peak_percentage);
                break;
                
            case METRIC_TYPE_CUSTOM:
                LOG_INFO("  Custom: value=%.2f, avg=%.2f, min=%.2f, max=%.2f",
                         metric->custom_value, metric->custom_average, metric->custom_min, metric->custom_max);
                break;
                
            default:
                break;
        }
    }
    
    LOG_INFO("=== End Statistics ===");
}

bool performance_profiler_is_initialized(void) {
    return g_profiler.initialized;
}
