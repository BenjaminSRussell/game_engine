/*
 * mtl_frame_sync.c
 * Advanced Metal frame synchronization and pacing implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_frame_sync.h"
#include <stdlib.h>
#include <string.h>
#include <mach/mach_time.h>
#include <include/math/math.h>

// ============================================================================
// Internal Structures
// ============================================================================

struct metal_frame_pacer {
    metal_frame_pacing_config_t config;
    
    // Timing data
    uint64_t frame_start_time;
    uint64_t last_frame_time;
    double* frame_times;                   // Rolling window of frame times
    uint32_t frame_time_index;
    
    // Statistics
    metal_frame_timing_stats_t stats;
    
    // Callbacks
    metal_frame_drop_callback_t drop_callback;
    void* drop_callback_userdata;
    metal_vsync_callback_t vsync_callback;
    void* vsync_callback_userdata;
    
    // State
    bool vsync_enabled;
    uint64_t total_frames;
    uint64_t dropped_frames;
};

// ============================================================================
// Timing Helpers
// ============================================================================

static mach_timebase_info_data_t get_timebase_info() {
    static mach_timebase_info_data_t info;
    static bool initialized = false;
    
    if (!initialized) {
        mach_timebase_info(&info);
        initialized = true;
    }
    
    return info;
}

static double mach_time_to_ms(uint64_t mach_time) {
    mach_timebase_info_data_t info = get_timebase_info();
    uint64_t nanos = mach_time * info.numer / info.denom;
    return (double)nanos / 1000000.0;
}

static uint64_t ms_to_mach_time(double ms) {
    mach_timebase_info_data_t info = get_timebase_info();
    uint64_t nanos = (uint64_t)(ms * 1000000.0);
    return nanos * info.denom / info.numer;
}

// ============================================================================
// Statistics Calculation
// ============================================================================

static void calculate_percentile(double* values, uint32_t count, double percentile, double* result) {
    if (count == 0) {
        *result = 0.0;
        return;
    }
    
    // Simple percentile calculation (could be optimized with better algorithm)
    double* sorted = (double*)malloc(count * sizeof(double));
    memcpy(sorted, values, count * sizeof(double));
    
    // Bubble sort (fine for small arrays)
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = 0; j < count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                double temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    uint32_t index = (uint32_t)(percentile * count);
    if (index >= count) index = count - 1;
    
    *result = sorted[index];
    free(sorted);
}

static void update_statistics(metal_frame_pacer_t* pacer, double frame_time_ms) {
    if (!pacer) return;
    
    // Update rolling window
    pacer->frame_times[pacer->frame_time_index] = frame_time_ms;
    pacer->frame_time_index = (pacer->frame_time_index + 1) % pacer->config.stats_window_size;
    
    // Calculate statistics over window
    uint32_t valid_count = (pacer->total_frames < pacer->config.stats_window_size) ? 
                           pacer->total_frames : pacer->config.stats_window_size;
    
    double sum = 0.0;
    double min_time = INFINITY;
    double max_time = 0.0;
    
    for (uint32_t i = 0; i < valid_count; i++) {
        double time = pacer->frame_times[i];
        sum += time;
        if (time < min_time) min_time = time;
        if (time > max_time) max_time = time;
    }
    
    pacer->stats.current_frame_time_ms = frame_time_ms;
    pacer->stats.avg_frame_time_ms = sum / valid_count;
    pacer->stats.min_frame_time_ms = min_time;
    pacer->stats.max_frame_time_ms = max_time;
    pacer->stats.total_frames = pacer->total_frames;
    pacer->stats.dropped_frames = pacer->dropped_frames;
    pacer->stats.current_fps = (frame_time_ms > 0.0) ? (uint32_t)(1000.0 / frame_time_ms) : 0;
    
    // Calculate percentiles
    calculate_percentile(pacer->frame_times, valid_count, 0.95, &pacer->stats.percentile_95_ms);
    calculate_percentile(pacer->frame_times, valid_count, 0.99, &pacer->stats.percentile_99_ms);
    
    // Check for frame drop
    if (frame_time_ms > pacer->config.frame_time_budget_ms) {
        pacer->dropped_frames++;
        pacer->stats.dropped_frames = pacer->dropped_frames;
        
        if (pacer->drop_callback) {
            double exceeded = frame_time_ms - pacer->config.frame_time_budget_ms;
            pacer->drop_callback(pacer->total_frames, exceeded, pacer->drop_callback_userdata);
        }
    }
}

// ============================================================================
// Frame Pacer Implementation
// ============================================================================

metal_frame_pacer_t* metal_frame_pacer_create(const metal_frame_pacing_config_t* config) {
    if (!config) return NULL;
    
    metal_frame_pacer_t* pacer = (metal_frame_pacer_t*)calloc(1, sizeof(metal_frame_pacer_t));
    if (!pacer) return NULL;
    
    pacer->config = *config;
    
    // Allocate rolling window
    uint32_t window_size = config->stats_window_size > 0 ? config->stats_window_size : 60;
    pacer->config.stats_window_size = window_size;
    pacer->frame_times = (double*)calloc(window_size, sizeof(double));
    
    if (!pacer->frame_times) {
        free(pacer);
        return NULL;
    }
    
    // Calculate frame time budget if target FPS is set
    if (config->target_fps > 0) {
        pacer->config.frame_time_budget_ms = 1000.0 / config->target_fps;
    }
    
    // Initialize statistics
    pacer->stats.min_frame_time_ms = INFINITY;
    pacer->vsync_enabled = config->enable_vsync;
    
    return pacer;
}

bool metal_frame_pacer_begin_frame(metal_frame_pacer_t* pacer) {
    if (!pacer) return false;
    
    pacer->frame_start_time = mach_absolute_time();
    
    // Apply frame pacing if enabled and not VSync
    if (!pacer->vsync_enabled && pacer->config.target_fps > 0) {
        if (pacer->last_frame_time > 0) {
            uint64_t current_time = mach_absolute_time();
            uint64_t elapsed = current_time - pacer->last_frame_time;
            double elapsed_ms = mach_time_to_ms(elapsed);
            
            double target_ms = pacer->config.frame_time_budget_ms;
            if (elapsed_ms < target_ms) {
                // Sleep for remaining time
                double sleep_ms = target_ms - elapsed_ms;
                uint64_t sleep_time = ms_to_mach_time(sleep_ms);
                
                // Use mach_wait_until for precise timing
                mach_wait_until(current_time + sleep_time);
            }
        }
    }
    
    // Adaptive frame rate: skip frame if severely over budget
    if (pacer->config.enable_adaptive_pacing) {
        if (pacer->stats.avg_frame_time_ms > pacer->config.adaptive_threshold_ms) {
            // Skip this frame
            pacer->total_frames++;
            return false;
        }
    }
    
    pacer->total_frames++;
    return true;
}

void metal_frame_pacer_end_frame(metal_frame_pacer_t* pacer, double gpu_time_ms) {
    if (!pacer) return;
    
    uint64_t current_time = mach_absolute_time();
    double frame_time_ms;
    
    if (gpu_time_ms > 0.0) {
        // Use GPU time if provided
        frame_time_ms = gpu_time_ms;
    } else {
        // Use CPU time
        frame_time_ms = mach_time_to_ms(current_time - pacer->frame_start_time);
    }
    
    update_statistics(pacer, frame_time_ms);
    pacer->last_frame_time = current_time;
}

void metal_frame_pacer_get_stats(metal_frame_pacer_t* pacer, metal_frame_timing_stats_t* stats) {
    if (!pacer || !stats) return;
    *stats = pacer->stats;
}

void metal_frame_pacer_reset_stats(metal_frame_pacer_t* pacer) {
    if (!pacer) return;
    
    memset(&pacer->stats, 0, sizeof(metal_frame_timing_stats_t));
    pacer->stats.min_frame_time_ms = INFINITY;
    pacer->total_frames = 0;
    pacer->dropped_frames = 0;
    pacer->frame_time_index = 0;
    memset(pacer->frame_times, 0, pacer->config.stats_window_size * sizeof(double));
}

void metal_frame_pacer_update_config(metal_frame_pacer_t* pacer, const metal_frame_pacing_config_t* config) {
    if (!pacer || !config) return;
    
    pacer->config = *config;
    
    if (config->target_fps > 0) {
        pacer->config.frame_time_budget_ms = 1000.0 / config->target_fps;
    }
    
    pacer->vsync_enabled = config->enable_vsync;
}

bool metal_frame_pacer_is_within_budget(metal_frame_pacer_t* pacer) {
    if (!pacer || pacer->frame_start_time == 0) return true;
    
    uint64_t current_time = mach_absolute_time();
    double elapsed_ms = mach_time_to_ms(current_time - pacer->frame_start_time);
    
    return elapsed_ms < pacer->config.frame_time_budget_ms;
}

uint32_t metal_frame_pacer_get_skip_count(metal_frame_pacer_t* pacer) {
    if (!pacer || !pacer->config.enable_adaptive_pacing) return 0;
    
    // Simple adaptive logic: skip frames if significantly over budget
    double ratio = pacer->stats.avg_frame_time_ms / pacer->config.frame_time_budget_ms;
    
    if (ratio > 2.0) return 2;  // Skip 2 frames
    if (ratio > 1.5) return 1;  // Skip 1 frame
    
    return 0;
}

void metal_frame_pacer_destroy(metal_frame_pacer_t* pacer) {
    if (!pacer) return;
    
    if (pacer->frame_times) {
        free(pacer->frame_times);
    }
    
    free(pacer);
}

// ============================================================================
// Frame Drop Detection
// ============================================================================

void metal_frame_pacer_set_drop_callback(metal_frame_pacer_t* pacer,
                                        metal_frame_drop_callback_t callback,
                                        void* user_data) {
    if (!pacer) return;
    
    pacer->drop_callback = callback;
    pacer->drop_callback_userdata = user_data;
}

double metal_frame_pacer_get_drop_rate(metal_frame_pacer_t* pacer) {
    if (!pacer || pacer->total_frames == 0) return 0.0;
    
    return (double)pacer->dropped_frames / (double)pacer->total_frames * 100.0;
}

// ============================================================================
// VSync Integration
// ============================================================================

void metal_frame_pacer_set_vsync(metal_frame_pacer_t* pacer, bool enable) {
    if (!pacer) return;
    pacer->vsync_enabled = enable;
}

void metal_frame_pacer_set_vsync_callback(metal_frame_pacer_t* pacer,
                                         metal_vsync_callback_t callback,
                                         void* user_data) {
    if (!pacer) return;
    
    pacer->vsync_callback = callback;
    pacer->vsync_callback_userdata = user_data;
}

// ============================================================================
// Frame Resource Manager Implementation
// ============================================================================

metal_frame_resource_manager_t* metal_frame_resource_manager_create(uint32_t max_frames_in_flight) {
    if (max_frames_in_flight == 0 || max_frames_in_flight > 10) {
        return NULL;
    }
    
    metal_frame_resource_manager_t* manager = 
        (metal_frame_resource_manager_t*)calloc(1, sizeof(metal_frame_resource_manager_t));
    
    if (!manager) return NULL;
    
    manager->max_frames_in_flight = max_frames_in_flight;
    manager->frames = (metal_frame_resources_t*)calloc(max_frames_in_flight, sizeof(metal_frame_resources_t));
    
    if (!manager->frames) {
        free(manager);
        return NULL;
    }
    
    // Initialize each frame resource container
    for (uint32_t i = 0; i < max_frames_in_flight; i++) {
        manager->frames[i].resource_capacity = 16; // Start with 16 resources
        manager->frames[i].resources = (void**)calloc(16, sizeof(void*));
        
        if (!manager->frames[i].resources) {
            // Cleanup on failure
            for (uint32_t j = 0; j < i; j++) {
                free(manager->frames[j].resources);
            }
            free(manager->frames);
            free(manager);
            return NULL;
        }
    }
    
    return manager;
}

metal_frame_resources_t* metal_frame_resource_manager_get_current(metal_frame_resource_manager_t* manager) {
    if (!manager) return NULL;
    
    uint32_t index = manager->current_frame_index % manager->max_frames_in_flight;
    return &manager->frames[index];
}

void metal_frame_resource_manager_advance(metal_frame_resource_manager_t* manager) {
    if (!manager) return;
    
    manager->current_frame_index++;
}

void metal_frame_resource_manager_add_resource(metal_frame_resource_manager_t* manager, void* resource) {
    if (!manager || !resource) return;
    
    metal_frame_resources_t* frame = metal_frame_resource_manager_get_current(manager);
    if (!frame) return;
    
    // Resize if needed
    if (frame->resource_count >= frame->resource_capacity) {
        uint32_t new_capacity = frame->resource_capacity * 2;
        void** new_resources = (void**)realloc(frame->resources, new_capacity * sizeof(void*));
        
        if (!new_resources) return; // Allocation failed
        
        frame->resources = new_resources;
        frame->resource_capacity = new_capacity;
    }
    
    frame->resources[frame->resource_count++] = resource;
}

void metal_frame_resource_manager_clear_frame(metal_frame_resource_manager_t* manager, uint32_t frame_index) {
    if (!manager) return;
    
    uint32_t index = frame_index % manager->max_frames_in_flight;
    manager->frames[index].resource_count = 0;
    manager->frames[index].in_use = false;
}

void metal_frame_resource_manager_destroy(metal_frame_resource_manager_t* manager) {
    if (!manager) return;
    
    if (manager->frames) {
        for (uint32_t i = 0; i < manager->max_frames_in_flight; i++) {
            if (manager->frames[i].resources) {
                free(manager->frames[i].resources);
            }
        }
        free(manager->frames);
    }
    
    free(manager);
}
