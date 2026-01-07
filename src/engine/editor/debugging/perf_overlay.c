#include "editor/debugging/perf_overlay.h"
#include "editor/profiling/statistics/stats_tracker.h"
#include <stdio.h>

void perf_overlay_init(void) {
    // Initialize UI system if needed
}

void perf_overlay_update(float delta_time) {
    stats_update_metric("Frame Time", delta_time * 1000.0f);
    stats_update_metric("FPS", 1.0f / delta_time);
}

void perf_overlay_draw(void) {
    stat_metric_t* frame_time = stats_get_metric("Frame Time");
    stat_metric_t* fps = stats_get_metric("FPS");
    
    if (frame_time && fps) {
        // Mock draw call - would use engine's text rendering
        // draw_text(10, 10, "FPS: %.1f", fps->current);
        // draw_text(10, 30, "Frame Time: %.2f ms", frame_time->current);
    }
}
