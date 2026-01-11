#ifndef GPU_PROFILER_H
#define GPU_PROFILER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_PROFILER_MAX_PASSES 64
#define GPU_PROFILER_HISTORY_FRAMES 120

// GPU profiling pass
typedef struct gpu_profiler_pass {
    char name[64];
    uint64_t gpu_start_time;
    uint64_t gpu_end_time;
    float gpu_time_ms;
    uint32_t memory_used;
    uint32_t bandwidth_estimate;
    bool active;
    uint32_t start_sample_index;
    uint32_t end_sample_index;
} gpu_profiler_pass_t;

// GPU profiler state
typedef struct gpu_profiler {
    id device; // id<MTLDevice>
    id counter_sample_buffer; // id<MTLCounterSampleBuffer>
    
    gpu_profiler_pass_t passes[GPU_PROFILER_MAX_PASSES];
    uint32_t pass_count;
    
    // Frame timing history
    float frame_times_ms[GPU_PROFILER_HISTORY_FRAMES];
    uint32_t frame_index;
    
    // Statistics
    float total_gpu_time_ms;
    float average_frame_time_ms;
    float peak_frame_time_ms;
    
    uint64_t total_memory_used;
    uint64_t total_bandwidth;
    
    // Timestamp sampling
    uint32_t current_sample_index;
    uint32_t max_samples;
    
    bool enabled;
    bool overlay_visible;
} gpu_profiler_t;

// Profiler lifecycle
gpu_profiler_t* gpu_profiler_create(id device);
void gpu_profiler_destroy(gpu_profiler_t* profiler);

// Pass timing
void gpu_profiler_begin_pass(gpu_profiler_t* profiler, const char* pass_name, id command_buffer);
void gpu_profiler_end_pass(gpu_profiler_t* profiler, const char* pass_name, id command_buffer);

// Frame management
void gpu_profiler_begin_frame(gpu_profiler_t* profiler);
void gpu_profiler_end_frame(gpu_profiler_t* profiler);

// Statistics
float gpu_profiler_get_pass_time(gpu_profiler_t* profiler, const char* pass_name);
float gpu_profiler_get_frame_time(gpu_profiler_t* profiler);
float gpu_profiler_get_average_frame_time(gpu_profiler_t* profiler);
void gpu_profiler_get_memory_stats(gpu_profiler_t* profiler, uint64_t* out_used, uint64_t* out_peak);
void gpu_profiler_get_bandwidth_stats(gpu_profiler_t* profiler, uint64_t* out_bandwidth);

// Helper functions
float gpu_profiler_estimate_pass_time(gpu_profiler_t* profiler, const char* pass_name);

// Display control
void gpu_profiler_set_overlay_visible(gpu_profiler_t* profiler, bool visible);
void gpu_profiler_render_overlay(gpu_profiler_t* profiler, id render_encoder);

#ifdef __cplusplus
}
#endif

#endif // GPU_PROFILER_H
