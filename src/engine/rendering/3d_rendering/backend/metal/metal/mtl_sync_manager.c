/*
 * mtl_sync_manager.c
 * Unified Metal synchronization manager implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_sync_manager.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Internal Structure
// ============================================================================

struct metal_sync_manager {
    // Sub-systems
    metal_frame_sync_t* frame_sync;
    metal_frame_pacer_t* frame_pacer;
    metal_barrier_manager_t* barrier_manager;
    metal_timestamp_pool_t* timestamp_pool;
    metal_vsync_manager_t* vsync_manager;
    
    // Configuration
    metal_sync_manager_config_t config;
    
    // State
    uint64_t frame_index;
    bool frame_in_progress;
};

// ============================================================================
// Synchronization Manager Implementation
// ============================================================================

#ifdef __OBJC__
metal_sync_manager_t* metal_sync_manager_create(id<MTLDevice> device,
                                                const metal_sync_manager_config_t* config) {
#else
metal_sync_manager_t* metal_sync_manager_create(id device,
                                                const metal_sync_manager_config_t* config)  {
#endif
    if (!device || !config) return NULL;
    
    metal_sync_manager_t* manager = (metal_sync_manager_t*)calloc(1, sizeof(metal_sync_manager_t));
    if (!manager) return NULL;
    
    manager->config = *config;
    
    // Create frame synchronization
    manager->frame_sync = metal_frame_sync_create(device, config->max_frames_in_flight);
    if (!manager->frame_sync) {
        free(manager);
        return NULL;
    }
    
    // Create frame pacer
    metal_frame_pacing_config_t pacer_config = {
        .target_fps = config->target_fps,
        .frame_time_budget_ms = 1000.0 / config->target_fps,
        .enable_vsync = config->enable_vsync,
        .enable_adaptive_pacing = true,
        .stats_window_size = 60,
        .adaptive_threshold_ms = (1000.0 / config->target_fps) * 1.5
    };
    
    manager->frame_pacer = metal_frame_pacer_create(&pacer_config);
    if (!manager->frame_pacer) {
        metal_frame_sync_destroy(manager->frame_sync);
        free(manager);
        return NULL;
    }
    
    // Create barrier manager if enabled
    if (config->enable_hazard_tracking) {
        manager->barrier_manager = metal_barrier_manager_create(device, config->barrier_policy);
    }
    
    // Create timestamp pool if enabled
    if (config->enable_gpu_timestamps) {
        metal_timestamp_config_t ts_config = {
            .max_queries = 128,
            .enable_cpu_correlation = true,
            .enable_validation = true
        };
        manager->timestamp_pool = metal_timestamp_pool_create(device, &ts_config);
    }
    
    // Create VSync manager if enabled
    if (config->enable_vsync) {
        metal_vsync_config_t vsync_config = {
            .display_id = 0,
            .enable_adaptive_sync = false,
            .target_frame_rate = (double)config->target_fps
        };
        manager->vsync_manager = metal_vsync_manager_create(&vsync_config);
        
        if (manager->vsync_manager) {
            metal_vsync_manager_start(manager->vsync_manager);
        }
    }
    
    return manager;
}

bool metal_sync_manager_begin_frame(metal_sync_manager_t* manager) {
    if (!manager) return false;
    
    // Wait for frame slot
    metal_frame_begin(manager->frame_sync);
    
    // Begin frame pacing
    bool should_render = metal_frame_pacer_begin_frame(manager->frame_pacer);
    if (!should_render) {
        // Frame skipped, release frame slot
        // Note: would need to track this properly
        return false;
    }
    
    manager->frame_in_progress = true;
    manager->frame_index++;
    
    return true;
}

void metal_sync_manager_end_frame(metal_sync_manager_t* manager,
                                  metal_command_buffer_t* cmd_buffer) {
    if (!manager || !cmd_buffer || !manager->frame_in_progress) return;
    
    // End frame sync
    metal_frame_end(manager->frame_sync, cmd_buffer);
    
    // End frame pacing (get GPU time if available)
    double gpu_time_ms = 0.0;
    // Could fetch from timestamp pool here
    metal_frame_pacer_end_frame(manager->frame_pacer, gpu_time_ms);
    
    manager->frame_in_progress = false;
}

metal_frame_sync_t* metal_sync_manager_get_frame_sync(metal_sync_manager_t* manager) {
    return manager ? manager->frame_sync : NULL;
}

metal_frame_pacer_t* metal_sync_manager_get_frame_pacer(metal_sync_manager_t* manager) {
    return manager ? manager->frame_pacer : NULL;
}

metal_barrier_manager_t* metal_sync_manager_get_barrier_manager(metal_sync_manager_t* manager) {
    return manager ? manager->barrier_manager : NULL;
}

metal_timestamp_pool_t* metal_sync_manager_get_timestamp_pool(metal_sync_manager_t* manager) {
    return manager ? manager->timestamp_pool : NULL;
}

metal_vsync_manager_t* metal_sync_manager_get_vsync_manager(metal_sync_manager_t* manager) {
    return manager ? manager->vsync_manager : NULL;
}

uint64_t metal_sync_manager_get_frame_index(metal_sync_manager_t* manager) {
    return manager ? manager->frame_index : 0;
}

void metal_sync_manager_get_stats(metal_sync_manager_t* manager,
                                  metal_frame_timing_stats_t* frame_stats,
                                  metal_hazard_report_t* hazard_report,
                                  metal_vsync_stats_t* vsync_stats) {
    if (!manager) return;
    
    if (frame_stats && manager->frame_pacer) {
        metal_frame_pacer_get_stats(manager->frame_pacer, frame_stats);
    }
    
    if (hazard_report && manager->barrier_manager) {
        metal_generate_hazard_report(manager->barrier_manager, hazard_report);
    }
    
    if (vsync_stats && manager->vsync_manager) {
        metal_vsync_manager_get_stats(manager->vsync_manager, vsync_stats);
    }
}

void metal_sync_manager_reset_stats(metal_sync_manager_t* manager) {
    if (!manager) return;
    
    if (manager->frame_pacer) {
        metal_frame_pacer_reset_stats(manager->frame_pacer);
    }
    
    if (manager->barrier_manager) {
        metal_barrier_manager_reset_stats(manager->barrier_manager);
    }
    
    if (manager->vsync_manager) {
        metal_vsync_manager_reset_stats(manager->vsync_manager);
    }
    
    if (manager->timestamp_pool) {
        metal_timestamp_reset_stats(manager->timestamp_pool);
    }
}

void metal_sync_manager_print_report(metal_sync_manager_t* manager) {
    if (!manager) return;
    
    printf("======================================\n");
    printf("  Metal Synchronization Report\n");
    printf("======================================\n\n");
    
    // Frame stats
    if (manager->frame_pacer) {
        metal_frame_timing_stats_t stats;
        metal_frame_pacer_get_stats(manager->frame_pacer, &stats);
        
        printf("[Frame Pacing]\n");
        printf("  Current FPS:      %u\n", stats.current_fps);
        printf("  Avg Frame Time:   %.2f ms\n", stats.avg_frame_time_ms);
        printf("  Min Frame Time:   %.2f ms\n", stats.min_frame_time_ms);
        printf("  Max Frame Time:   %.2f ms\n", stats.max_frame_time_ms);
        printf("  95th Percentile:  %.2f ms\n", stats.percentile_95_ms);
        printf("  99th Percentile:  %.2f ms\n", stats.percentile_99_ms);
        printf("  Total Frames:     %llu\n", stats.total_frames);
        printf("  Dropped Frames:   %llu\n", stats.dropped_frames);
        printf("\n");
    }
    
    // Hazard report
    if (manager->barrier_manager) {
        metal_hazard_report_t report;
        metal_generate_hazard_report(manager->barrier_manager, &report);
        metal_print_hazard_report(&report);
        printf("\n");
    }
    
    // VSync stats
    if (manager->vsync_manager) {
        metal_vsync_stats_t stats;
        metal_vsync_manager_get_stats(manager->vsync_manager, &stats);
        
        printf("[VSync]\n");
        printf("  Refresh Rate:     %.2f Hz\n", stats.refresh_rate_hz);
        printf("  Frame Time:       %.2f ms\n", stats.actual_frame_time_ms);
        printf("  Jitter:           %.2f ms\n", stats.jitter_ms);
        printf("  Total VBlanks:    %llu\n", stats.total_vblanks);
        printf("  Missed VBlanks:   %llu\n", stats.missed_vblanks);
        printf("\n");
    }
    
    // GPU timestamps
    if (manager->timestamp_pool) {
        metal_timestamp_print_stats(manager->timestamp_pool);
        printf("\n");
    }
    
    printf("======================================\n");
}

void metal_sync_manager_destroy(metal_sync_manager_t* manager) {
    if (!manager) return;
    
    if (manager->vsync_manager) {
        metal_vsync_manager_destroy(manager->vsync_manager);
    }
    
    if (manager->timestamp_pool) {
        metal_timestamp_pool_destroy(manager->timestamp_pool);
    }
    
    if (manager->barrier_manager) {
        metal_barrier_manager_destroy(manager->barrier_manager);
    }
    
    if (manager->frame_pacer) {
        metal_frame_pacer_destroy(manager->frame_pacer);
    }
    
    if (manager->frame_sync) {
        metal_frame_sync_destroy(manager->frame_sync);
    }
    
    free(manager);
}
