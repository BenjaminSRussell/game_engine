// Frame Budget Optimizer
// Achieves target frame budget: 16.67ms @ 1440p 60FPS

#include "frame_budget_optimizer.h"
#include "core/logger.h"
#include "core/timer.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Frame budget constants
#define TARGET_FRAME_TIME_MS 16.67f
#define TARGET_FPS 60.0f
#define FRAME_BUDGET_HISTORY_SIZE 60
#define ADAPTIVE_QUALITY_STEP_SIZE 0.1f
#define MIN_QUALITY_SCALE 0.5f
#define MAX_QUALITY_SCALE 2.0f

// Performance metrics
typedef struct {
    f64 frame_time_ms;
    f64 cpu_time_ms;
    f64 gpu_time_ms;
    u32 draw_calls;
    u32 triangles;
    u32 pixels_shaded;
    u64 memory_usage_mb;
    f64 quality_scale;
} FrameMetrics;

typedef struct {
    FrameMetrics history[FRAME_BUDGET_HISTORY_SIZE];
    u32 history_index;
    u32 frame_count;
    
    f64 average_frame_time;
    f64 average_cpu_time;
    f64 average_gpu_time;
    
    f64 target_frame_time;
    f64 current_quality_scale;
    
    bool adaptive_quality_enabled;
    bool dynamic_resolution_enabled;
    bool lod_adjustment_enabled;
    
    // Adaptive settings
    struct {
        bool taa_enabled;
        bool ssao_enabled;
        bool ssr_enabled;
        bool bloom_enabled;
        bool shadows_enabled;
        u32 shadow_resolution;
        u32 ssao_samples;
        u32 ssr_max_steps;
        f32 render_scale;
    } settings;
    
    // Performance thresholds
    struct {
        f64 cpu_threshold;
        f64 gpu_threshold;
        f64 memory_threshold;
        u32 draw_call_threshold;
    } thresholds;
    
} FrameBudgetOptimizer;

// Global optimizer instance
static FrameBudgetOptimizer g_optimizer = {0};

// Initialize frame budget optimizer
void frame_budget_init(void) {
    memset(&g_optimizer, 0, sizeof(FrameBudgetOptimizer));
    
    g_optimizer.target_frame_time = TARGET_FRAME_TIME_MS;
    g_optimizer.current_quality_scale = 1.0f;
    
    // Enable adaptive features
    g_optimizer.adaptive_quality_enabled = true;
    g_optimizer.dynamic_resolution_enabled = true;
    g_optimizer.lod_adjustment_enabled = true;
    
    // Initialize default settings
    g_optimizer.settings.taa_enabled = true;
    g_optimizer.settings.ssr_enabled = true;
    g_optimizer.settings.bloom_enabled = true;
    g_optimizer.settings.shadows_enabled = true;
    g_optimizer.settings.shadow_resolution = 2048;
    g_optimizer.settings.ssao_enabled = true;
    g_optimizer.settings.ssao_samples = 16;
    g_optimizer.settings.ssr_max_steps = 64;
    g_optimizer.settings.render_scale = 1.0f;
    
    // Set performance thresholds
    g_optimizer.thresholds.cpu_threshold = 8.0f;  // 8ms CPU budget
    g_optimizer.thresholds.gpu_threshold = 8.0f;  // 8ms GPU budget
    g_optimizer.thresholds.memory_threshold = 2048.0; // 2GB memory budget
    g_optimizer.thresholds.draw_call_threshold = 1000;
    
    LOG_INFO("Frame budget optimizer initialized");
    LOG_INFO("Target frame time: %.2fms (%.0f FPS)", g_optimizer.target_frame_time, TARGET_FPS);
}

// Shutdown frame budget optimizer
void frame_budget_shutdown(void) {
    memset(&g_optimizer, 0, sizeof(FrameBudgetOptimizer));
    LOG_INFO("Frame budget optimizer shutdown");
}

// Begin frame measurement
void frame_budget_begin_frame(void) {
    // Record frame start time
    g_optimizer.frame_count++;
    
    // Get current frame metrics
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    memset(current, 0, sizeof(FrameMetrics));
    
    current->quality_scale = g_optimizer.current_quality_scale;
}

// End frame measurement and update statistics
void frame_budget_end_frame(void) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    
    // Get frame timing
    static u64 last_frame_time = 0;
    u64 current_time = common_timer_get_nanos();
    
    if (last_frame_time > 0) {
        current->frame_time_ms = (current_time - last_frame_time) / 1000000.0;
    }
    last_frame_time = current_time;
    
    // Update history index
    g_optimizer.history_index = (g_optimizer.history_index + 1) % FRAME_BUDGET_HISTORY_SIZE;
    
    // Update running averages
    update_averages();
    
    // Perform adaptive quality adjustment
    if (g_optimizer.adaptive_quality_enabled) {
        adjust_adaptive_quality();
    }
}

// Record CPU time for current frame
void frame_budget_record_cpu_time(f64 cpu_time_ms) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    current->cpu_time_ms = cpu_time_ms;
}

// Record GPU time for current frame
void frame_budget_record_gpu_time(f64 gpu_time_ms) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    current->gpu_time_ms = gpu_time_ms;
}

// Record draw calls for current frame
void frame_budget_record_draw_calls(u32 draw_calls) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    current->draw_calls = draw_calls;
}

// Record triangle count for current frame
void frame_budget_record_triangles(u32 triangles) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    current->triangles = triangles;
}

// Record memory usage for current frame
void frame_budget_record_memory_usage(u64 memory_usage_mb) {
    FrameMetrics* current = &g_optimizer.history[g_optimizer.history_index];
    current->memory_usage_mb = memory_usage_mb;
}

// Update running averages
static void update_averages(void) {
    f64 total_frame_time = 0.0;
    f64 total_cpu_time = 0.0;
    f64 total_gpu_time = 0.0;
    u32 valid_frames = 0;
    
    for (u32 i = 0; i < FRAME_BUDGET_HISTORY_SIZE; i++) {
        FrameMetrics* metrics = &g_optimizer.history[i];
        if (metrics->frame_time_ms > 0.0) {
            total_frame_time += metrics->frame_time_ms;
            total_cpu_time += metrics->cpu_time_ms;
            total_gpu_time += metrics->gpu_time_ms;
            valid_frames++;
        }
    }
    
    if (valid_frames > 0) {
        g_optimizer.average_frame_time = total_frame_time / valid_frames;
        g_optimizer.average_cpu_time = total_cpu_time / valid_frames;
        g_optimizer.average_gpu_time = total_gpu_time / valid_frames;
    }
}

// Adaptive quality adjustment
static void adjust_adaptive_quality(void) {
    f64 frame_time_diff = g_optimizer.average_frame_time - g_optimizer.target_frame_time;
    f64 quality_adjustment = 0.0f;
    
    // Determine quality adjustment based on frame time
    if (frame_time_diff > 2.0f) {
        // Frame time too high, reduce quality
        quality_adjustment = -ADAPTIVE_QUALITY_STEP_SIZE;
        
        // More aggressive adjustment for severe overshoot
        if (frame_time_diff > 5.0f) {
            quality_adjustment *= 2.0f;
        }
    } else if (frame_time_diff < -2.0f) {
        // Frame time well under budget, increase quality
        quality_adjustment = ADAPTIVE_QUALITY_STEP_SIZE;
        
        // More aggressive adjustment for significant undershoot
        if (frame_time_diff < -5.0f) {
            quality_adjustment *= 2.0f;
        }
    }
    
    // Apply quality adjustment
    f64 new_quality_scale = g_optimizer.current_quality_scale + quality_adjustment;
    new_quality_scale = common_clamp_float(new_quality_scale, MIN_QUALITY_SCALE, MAX_QUALITY_SCALE);
    
    if (fabs(new_quality_scale - g_optimizer.current_quality_scale) > 0.01f) {
        g_optimizer.current_quality_scale = new_quality_scale;
        apply_quality_settings();
        
        LOG_DEBUG("Quality scale adjusted to %.2f (frame time: %.2fms)", 
                  g_optimizer.current_quality_scale, g_optimizer.average_frame_time);
    }
}

// Apply quality settings based on current quality scale
static void apply_quality_settings(void) {
    f64 scale = g_optimizer.current_quality_scale;
    
    // Adjust render scale
    if (g_optimizer.dynamic_resolution_enabled) {
        g_optimizer.settings.render_scale = common_clamp_float(scale, 0.5f, 1.0f);
    }
    
    // Adjust shadow resolution
    if (g_optimizer.settings.shadows_enabled) {
        u32 base_shadow_res = 2048;
        g_optimizer.settings.shadow_resolution = (u32)(base_shadow_res * scale);
        g_optimizer.settings.shadow_resolution = common_next_power_of_two(g_optimizer.settings.shadow_resolution);
        g_optimizer.settings.shadow_resolution = common_clamp_size(g_optimizer.settings.shadow_resolution, 512, 4096);
    }
    
    // Adjust SSAO samples
    if (g_optimizer.settings.ssao_enabled) {
        u32 base_ssao_samples = 16;
        g_optimizer.settings.ssao_samples = (u32)(base_ssao_samples * scale);
        g_optimizer.settings.ssao_samples = common_clamp_size(g_optimizer.settings.ssao_samples, 8, 32);
        // Ensure it's a power of 2 for better performance
        g_optimizer.settings.ssao_samples = common_next_power_of_two(g_optimizer.settings.ssao_samples);
    }
    
    // Adjust SSR max steps
    if (g_optimizer.settings.ssr_enabled) {
        u32 base_ssr_steps = 64;
        g_optimizer.settings.ssr_max_steps = (u32)(base_ssr_steps * scale);
        g_optimizer.settings.ssr_max_steps = common_clamp_size(g_optimizer.settings.ssr_max_steps, 16, 128);
    }
    
    // Disable expensive effects at very low quality
    if (scale < 0.7f) {
        g_optimizer.settings.ssr_enabled = false;
        g_optimizer.settings.bloom_enabled = false;
    } else {
        g_optimizer.settings.ssr_enabled = true;
        g_optimizer.settings.bloom_enabled = true;
    }
    
    // Disable TAA at very low quality (can cause artifacts)
    if (scale < 0.6f) {
        g_optimizer.settings.taa_enabled = false;
    } else {
        g_optimizer.settings.taa_enabled = true;
    }
    
    // Disable SSAO at very low quality
    if (scale < 0.5f) {
        g_optimizer.settings.ssao_enabled = false;
    } else {
        g_optimizer.settings.ssao_enabled = true;
    }
}

// Get current render settings
void frame_budget_get_settings(RenderSettings* settings) {
    if (!settings) return;
    
    settings->taa_enabled = g_optimizer.settings.taa_enabled;
    settings->ssao_enabled = g_optimizer.settings.ssao_enabled;
    settings->ssr_enabled = g_optimizer.settings.ssr_enabled;
    settings->bloom_enabled = g_optimizer.settings.bloom_enabled;
    settings->shadows_enabled = g_optimizer.settings.shadows_enabled;
    settings->shadow_resolution = g_optimizer.settings.shadow_resolution;
    settings->ssao_samples = g_optimizer.settings.ssao_samples;
    settings->ssr_max_steps = g_optimizer.settings.ssr_max_steps;
    settings->render_scale = g_optimizer.settings.render_scale;
}

// Get performance statistics
void frame_budget_get_stats(FrameBudgetStats* stats) {
    if (!stats) return;
    
    stats->average_frame_time = g_optimizer.average_frame_time;
    stats->target_frame_time = g_optimizer.target_frame_time;
    stats->current_fps = (g_optimizer.average_frame_time > 0.0) ? (1000.0 / g_optimizer.average_frame_time) : 0.0;
    stats->target_fps = TARGET_FPS;
    stats->quality_scale = g_optimizer.current_quality_scale;
    stats->frame_count = g_optimizer.frame_count;
    
    // Calculate performance percentage
    stats->performance_percentage = (g_optimizer.target_frame_time / g_optimizer.average_frame_time) * 100.0;
    stats->performance_percentage = common_clamp_float(stats->performance_percentage, 0.0f, 200.0f);
    
    // Get current frame metrics
    FrameMetrics* current = &g_optimizer.history[(g_optimizer.history_index + FRAME_BUDGET_HISTORY_SIZE - 1) % FRAME_BUDGET_HISTORY_SIZE];
    stats->current_frame_time = current->frame_time_ms;
    stats->current_cpu_time = current->cpu_time_ms;
    stats->current_gpu_time = current->gpu_time_ms;
    stats->current_draw_calls = current->draw_calls;
    stats->current_triangles = current->triangles;
    stats->current_memory_usage = current->memory_usage_mb;
}

// Check if frame budget is being met
bool frame_budget_is_target_met(void) {
    return g_optimizer.average_frame_time <= g_optimizer.target_frame_time;
}

// Get frame budget status
FrameBudgetStatus frame_budget_get_status(void) {
    f64 frame_time_diff = g_optimizer.average_frame_time - g_optimizer.target_frame_time;
    
    if (fabs(frame_time_diff) < 1.0f) {
        return FRAME_BUDGET_ON_TARGET;
    } else if (frame_time_diff > 0) {
        return FRAME_BUDGET_OVER_BUDGET;
    } else {
        return FRAME_BUDGET_UNDER_BUDGET;
    }
}

// Force quality scale (for manual override)
void frame_budget_set_quality_scale(f64 scale) {
    g_optimizer.current_quality_scale = common_clamp_float(scale, MIN_QUALITY_SCALE, MAX_QUALITY_SCALE);
    apply_quality_settings();
    
    LOG_INFO("Quality scale manually set to %.2f", g_optimizer.current_quality_scale);
}

// Enable/disable adaptive quality
void frame_budget_set_adaptive_quality_enabled(bool enabled) {
    g_optimizer.adaptive_quality_enabled = enabled;
    LOG_INFO("Adaptive quality %s", enabled ? "enabled" : "disabled");
}

// Enable/disable dynamic resolution
void frame_budget_set_dynamic_resolution_enabled(bool enabled) {
    g_optimizer.dynamic_resolution_enabled = enabled;
    LOG_INFO("Dynamic resolution %s", enabled ? "enabled" : "disabled");
}

// Reset optimizer statistics
void frame_budget_reset_stats(void) {
    memset(g_optimizer.history, 0, sizeof(g_optimizer.history));
    g_optimizer.history_index = 0;
    g_optimizer.frame_count = 0;
    g_optimizer.average_frame_time = 0.0;
    g_optimizer.average_cpu_time = 0.0;
    g_optimizer.average_gpu_time = 0.0;
    g_optimizer.current_quality_scale = 1.0f;
    
    LOG_INFO("Frame budget statistics reset");
}

// Print detailed performance report
void frame_budget_print_report(void) {
    FrameBudgetStats stats;
    frame_budget_get_stats(&stats);
    
    LOG_INFO("=== Frame Budget Performance Report ===");
    LOG_INFO("Target: %.2fms (%.0f FPS)", stats.target_frame_time, stats.target_fps);
    LOG_INFO("Current: %.2fms (%.1f FPS)", stats.current_frame_time, stats.current_fps);
    LOG_INFO("Average: %.2fms (%.1f FPS)", stats.average_frame_time, stats.current_fps);
    LOG_INFO("Performance: %.1f%% of target", stats.performance_percentage);
    LOG_INFO("Quality Scale: %.2f", stats.quality_scale);
    LOG_INFO("Frame Count: %llu", stats.frame_count);
    
    LOG_INFO("CPU Time: %.2fms", stats.current_cpu_time);
    LOG_INFO("GPU Time: %.2fms", stats.current_gpu_time);
    LOG_INFO("Draw Calls: %u", stats.current_draw_calls);
    LOG_INFO("Triangles: %u", stats.current_triangles);
    LOG_INFO("Memory Usage: %.1f MB", stats.current_memory_usage);
    
    LOG_INFO("=== Current Settings ===");
    LOG_INFO("TAA: %s", g_optimizer.settings.taa_enabled ? "enabled" : "disabled");
    LOG_INFO("SSAO: %s (%u samples)", g_optimizer.settings.ssao_enabled ? "enabled" : "disabled", g_optimizer.settings.ssao_samples);
    LOG_INFO("SSR: %s (%u steps)", g_optimizer.settings.ssr_enabled ? "enabled" : "disabled", g_optimizer.settings.ssr_max_steps);
    LOG_INFO("Bloom: %s", g_optimizer.settings.bloom_enabled ? "enabled" : "disabled");
    LOG_INFO("Shadows: %s (%ux%u)", g_optimizer.settings.shadows_enabled ? "enabled" : "disabled", 
             g_optimizer.settings.shadow_resolution, g_optimizer.settings.shadow_resolution);
    LOG_INFO("Render Scale: %.2f", g_optimizer.settings.render_scale);
    
    // Status
    const char* status_strings[] = {"Under Budget", "On Target", "Over Budget"};
    LOG_INFO("Status: %s", status_strings[frame_budget_get_status()]);
}
