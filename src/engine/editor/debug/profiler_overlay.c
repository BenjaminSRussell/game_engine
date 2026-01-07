#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

typedef struct gpu_profiler {
    id counter_sample_buffer; // id<MTLCounterSampleBuffer>
    double frame_times[120];
    uint32_t frame_index;
    
    struct {
        char name[32];
        double time_ms;
    } pass_times[32];
    uint32_t pass_count;
    
    uint64_t cpu_start_time;
} gpu_profiler_t;

gpu_profiler_t* profiler_create(id device_ptr) {
#ifdef __OBJC__
    id<MTLDevice> device = (id<MTLDevice>)device_ptr;
    gpu_profiler_t* profiler = (gpu_profiler_t*)calloc(1, sizeof(gpu_profiler_t));
    
    // Check for counter support
    // if ([device supportsCounterSampling:MTLCommonCounterSetTimestamp]) ...
    
    return profiler;
#else
    return NULL;
#endif
}

void profiler_destroy(gpu_profiler_t* profiler) {
    if (profiler) free(profiler);
}

void profiler_begin_frame(gpu_profiler_t* profiler) {
    if (!profiler) return;
    profiler->pass_count = 0;
}

void profiler_begin_pass(gpu_profiler_t* profiler, const char* name) {
    if (!profiler || profiler->pass_count >= 32) return;
    strncpy(profiler->pass_times[profiler->pass_count].name, name, 31);
    // Record GPU timestamp start
}

void profiler_end_pass(gpu_profiler_t* profiler) {
    if (!profiler || profiler->pass_count >= 32) return;
    // Record GPU timestamp end
    // Store calc in pass_times
    // Mock value:
    profiler->pass_times[profiler->pass_count].time_ms = 0.5f; 
    profiler->pass_count++;
}

void profiler_end_frame(gpu_profiler_t* profiler) {
    if (!profiler) return;
    // Push total frame time
    profiler->frame_times[profiler->frame_index] = 16.6f; // Mock 60fps
    profiler->frame_index = (profiler->frame_index + 1) % 120;
}

void profiler_render_overlay(id encoder_ptr, gpu_profiler_t* profiler) {
#ifdef __OBJC__
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)encoder_ptr;
    // Draw graph using debug_renderer (lines)
    // Draw text using debug_renderer (text)
    
    // Example: Draw frame time graph
    /*
    for (int i = 0; i < 119; i++) {
        float h1 = profiler->frame_times[i];
        float h2 = profiler->frame_times[i+1];
        // debug_draw_line(...)
    }
    */
#endif
}
