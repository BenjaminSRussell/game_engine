#pragma once

#include <stdbool.h>

typedef struct {
    float *frame_times; // Ring buffer
    int frame_time_count;
    int frame_time_capacity;
    
    // Memory tracking
    size_t current_memory_usage;
    size_t peak_memory_usage;
    
    // Display options
    bool show_frame_graph;
    bool show_memory_chart;
    bool show_job_viz;
    bool show_gpu_times;
    bool show_hierarchy;
    bool is_paused;
    
    // Selected frame
    int paused_frame;
} ProfilerOverlayUI;

void profiler_overlay_init(ProfilerOverlayUI *overlay);
void profiler_overlay_destroy(ProfilerOverlayUI *overlay);

// Data updates
void profiler_overlay_add_frame_time(ProfilerOverlayUI *overlay, float time_ms);
void profiler_overlay_update_memory(ProfilerOverlayUI *overlay, size_t current, size_t peak);

// Rendering
void profiler_overlay_draw(ProfilerOverlayUI *overlay);
void profiler_overlay_draw_frame_graph(ProfilerOverlayUI *overlay);
void profiler_overlay_draw_memory_chart(ProfilerOverlayUI *overlay);

// Control
void profiler_overlay_toggle_pause(ProfilerOverlayUI *overlay);
void profiler_overlay_step_frame(ProfilerOverlayUI *overlay);
