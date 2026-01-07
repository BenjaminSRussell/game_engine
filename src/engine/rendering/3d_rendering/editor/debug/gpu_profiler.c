#include "gpu_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

gpu_profiler_t* gpu_profiler_create(id device_ptr) {
#ifdef __OBJC__
    id<MTLDevice> device = (id<MTLDevice>)device_ptr;
    
    gpu_profiler_t* profiler = (gpu_profiler_t*)calloc(1, sizeof(gpu_profiler_t));
    if (!profiler) return NULL;
    
    profiler->device = device;
    profiler->pass_count = 0;
    profiler->frame_index = 0;
    profiler->enabled = true;
    profiler->overlay_visible = false;
    
    // Initialize timing history
    for (int i = 0; i < GPU_PROFILER_HISTORY_FRAMES; i++) {
        profiler->frame_times_ms[i] = 0.0f;
    }
    
    // Try to create counter sample buffer for GPU timestamps
    // Note: This requires Metal Performance Shaders or Metal 2.3+
    if (@available(macOS 10.15, *)) {
        NSArray<id<MTLCounterSet>>* counterSets = device.counterSets;
        if (counterSets.count > 0) {
            id<MTLCounterSet> timestampCounter = counterSets[0];
            
            MTLCounterSampleBufferDescriptor* desc = [[MTLCounterSampleBufferDescriptor alloc] init];
            desc.counterSet = timestampCounter;
            desc.sampleCount = GPU_PROFILER_MAX_PASSES * 2; // Start and end for each pass
            desc.storageMode = MTLStorageModeShared;
            
            NSError* error = nil;
            profiler->counter_sample_buffer = [device newCounterSampleBufferWithDescriptor:desc error:&error];
        }
    }
    
    return profiler;
#else
    return NULL;
#endif
}

void gpu_profiler_destroy(gpu_profiler_t* profiler) {
    if (profiler) {
        free(profiler);
    }
}

void gpu_profiler_begin_pass(gpu_profiler_t* profiler, const char* pass_name, id command_buffer_ptr) {
#ifdef __OBJC__
    if (!profiler || !profiler->enabled || !pass_name) return;
    if (profiler->pass_count >= GPU_PROFILER_MAX_PASSES) return;
    
    // Find or create pass entry
    gpu_profiler_pass_t* pass = NULL;
    for (uint32_t i = 0; i < profiler->pass_count; i++) {
        if (strcmp(profiler->passes[i].name, pass_name) == 0) {
            pass = &profiler->passes[i];
            break;
        }
    }
    
    if (!pass) {
        pass = &profiler->passes[profiler->pass_count++];
        strncpy(pass->name, pass_name, sizeof(pass->name) - 1);
        pass->name[sizeof(pass->name) - 1] = '\0';
    }
    
    pass->active = true;
    
    // Sample GPU timestamp if available
    id<MTLCommandBuffer> commandBuffer = (id<MTLCommandBuffer>)command_buffer_ptr;
    if (profiler->counter_sample_buffer && commandBuffer) {
        // TODO: Use MTLCommandBuffer.sampleCountersInBuffer for precise GPU timing
    }
#endif
}

void gpu_profiler_end_pass(gpu_profiler_t* profiler, const char* pass_name, id command_buffer_ptr) {
#ifdef __OBJC__
    if (!profiler || !profiler->enabled || !pass_name) return;
    
    // Find pass entry
    gpu_profiler_pass_t* pass = NULL;
    for (uint32_t i = 0; i < profiler->pass_count; i++) {
        if (strcmp(profiler->passes[i].name, pass_name) == 0) {
            pass = &profiler->passes[i];
            break;
        }
    }
    
    if (!pass || !pass->active) return;
    
    pass->active = false;
    
    // Sample GPU timestamp if available
    id<MTLCommandBuffer> commandBuffer = (id<MTLCommandBuffer>)command_buffer_ptr;
    if (profiler->counter_sample_buffer && commandBuffer) {
        // TODO: Use MTLCommandBuffer.sampleCountersInBuffer for precise GPU timing
        // For now, use GPU time from command buffer completion
    }
#endif
}

void gpu_profiler_begin_frame(gpu_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) return;
    
    profiler->total_gpu_time_ms = 0.0f;
}

void gpu_profiler_end_frame(gpu_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) return;
    
    // Sum all pass times
    float frame_time = 0.0f;
    for (uint32_t i = 0; i < profiler->pass_count; i++) {
        frame_time += profiler->passes[i].gpu_time_ms;
    }
    
    profiler->total_gpu_time_ms = frame_time;
    
    // Update history
    profiler->frame_times_ms[profiler->frame_index] = frame_time;
    profiler->frame_index = (profiler->frame_index + 1) % GPU_PROFILER_HISTORY_FRAMES;
    
    // Update statistics
    float sum = 0.0f;
    float peak = 0.0f;
    for (int i = 0; i < GPU_PROFILER_HISTORY_FRAMES; i++) {
        sum += profiler->frame_times_ms[i];
        if (profiler->frame_times_ms[i] > peak) {
            peak = profiler->frame_times_ms[i];
        }
    }
    
    profiler->average_frame_time_ms = sum / GPU_PROFILER_HISTORY_FRAMES;
    profiler->peak_frame_time_ms = peak;
}

float gpu_profiler_get_pass_time(gpu_profiler_t* profiler, const char* pass_name) {
    if (!profiler || !pass_name) return 0.0f;
    
    for (uint32_t i = 0; i < profiler->pass_count; i++) {
        if (strcmp(profiler->passes[i].name, pass_name) == 0) {
            return profiler->passes[i].gpu_time_ms;
        }
    }
    
    return 0.0f;
}

float gpu_profiler_get_frame_time(gpu_profiler_t* profiler) {
    return profiler ? profiler->total_gpu_time_ms : 0.0f;
}

float gpu_profiler_get_average_frame_time(gpu_profiler_t* profiler) {
    return profiler ? profiler->average_frame_time_ms : 0.0f;
}

void gpu_profiler_get_memory_stats(gpu_profiler_t* profiler, uint64_t* out_used, uint64_t* out_peak) {
#ifdef __OBJC__
    if (!profiler) return;
    
    id<MTLDevice> device = (id<MTLDevice>)profiler->device;
    if (device) {
        if (out_used) *out_used = device.currentAllocatedSize;
        // Peak tracking would require instrumentation
        if (out_peak) *out_peak = device.currentAllocatedSize;
    }
#endif
}

void gpu_profiler_get_bandwidth_stats(gpu_profiler_t* profiler, uint64_t* out_bandwidth) {
    if (profiler && out_bandwidth) {
        *out_bandwidth = profiler->total_bandwidth;
    }
}

void gpu_profiler_set_overlay_visible(gpu_profiler_t* profiler, bool visible) {
    if (profiler) {
        profiler->overlay_visible = visible;
    }
}

void gpu_profiler_render_overlay(gpu_profiler_t* profiler, id render_encoder_ptr) {
#ifdef __OBJC__
    if (!profiler || !profiler->overlay_visible) return;
    
    // TODO: Render performance overlay using ImGui or custom UI
    // This would display:
    // - Frame graph
    // - Per-pass timing bars
    // - Memory usage
    // - Bandwidth tracking
    
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)render_encoder_ptr;
    
    // Placeholder for now
#endif
}
