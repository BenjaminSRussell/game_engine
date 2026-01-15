// Frame Budget Optimizer Header
// Achieves target frame budget: 16.67ms @ 1440p 60FPS

#ifndef FRAME_BUDGET_OPTIMIZER_H
#define FRAME_BUDGET_OPTIMIZER_H

#include "core/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Render settings structure
typedef struct {
    bool taa_enabled;
    bool ssao_enabled;
    bool ssr_enabled;
    bool bloom_enabled;
    bool shadows_enabled;
    u32 shadow_resolution;
    u32 ssao_samples;
    u32 ssr_max_steps;
    f32 render_scale;
} RenderSettings;

// Frame budget statistics
typedef struct {
    f64 average_frame_time;
    f64 target_frame_time;
    f64 current_frame_time;
    f64 current_cpu_time;
    f64 current_gpu_time;
    f64 current_fps;
    f64 target_fps;
    f64 quality_scale;
    f64 performance_percentage;
    u64 frame_count;
    u32 current_draw_calls;
    u32 current_triangles;
    u64 current_memory_usage;
} FrameBudgetStats;

// Frame budget status
typedef enum {
    FRAME_BUDGET_UNDER_BUDGET,
    FRAME_BUDGET_ON_TARGET,
    FRAME_BUDGET_OVER_BUDGET
} FrameBudgetStatus;

// Core functions
void frame_budget_init(void);
void frame_budget_shutdown(void);

// Frame measurement
void frame_budget_begin_frame(void);
void frame_budget_end_frame(void);

// Performance recording
void frame_budget_record_cpu_time(f64 cpu_time_ms);
void frame_budget_record_gpu_time(f64 gpu_time_ms);
void frame_budget_record_draw_calls(u32 draw_calls);
void frame_budget_record_triangles(u32 triangles);
void frame_budget_record_memory_usage(u64 memory_usage_mb);

// Settings and statistics
void frame_budget_get_settings(RenderSettings* settings);
void frame_budget_get_stats(FrameBudgetStats* stats);
bool frame_budget_is_target_met(void);
FrameBudgetStatus frame_budget_get_status(void);

// Manual controls
void frame_budget_set_quality_scale(f64 scale);
void frame_budget_set_adaptive_quality_enabled(bool enabled);
void frame_budget_set_dynamic_resolution_enabled(bool enabled);

// Utilities
void frame_budget_reset_stats(void);
void frame_budget_print_report(void);

#ifdef __cplusplus
}
#endif

#endif // FRAME_BUDGET_OPTIMIZER_H
