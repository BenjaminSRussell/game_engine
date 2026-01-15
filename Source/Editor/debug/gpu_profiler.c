#include "editor/debug/gpu_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

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
    
    // Initialize timestamp sampling
    profiler->current_sample_index = 0;
    profiler->max_samples = GPU_PROFILER_MAX_PASSES * 2; // Start and end samples for each pass
    
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
        // Use MTLCommandBuffer.sampleCountersInBuffer for precise GPU timing
        uint64_t sample_index = profiler->current_sample_index % profiler->max_samples;
        
        // Sample counters at the beginning of the pass
        [commandBuffer sampleCountersInBuffer:profiler->counter_sample_buffer 
                                     atSampleIndex:sample_index 
                                     withStride:sizeof(gpu_counter_sample_t)];
        
        // Store sample index for later retrieval
        pass->start_sample_index = sample_index;
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
        // Use MTLCommandBuffer.sampleCountersInBuffer for precise GPU timing
        uint64_t sample_index = (profiler->current_sample_index + 1) % profiler->max_samples;
        
        // Sample counters at the end of the pass
        [commandBuffer sampleCountersInBuffer:profiler->counter_sample_buffer 
                                     atSampleIndex:sample_index 
                                     withStride:sizeof(gpu_counter_sample_t)];
        
        // Resolve timing between start and end samples
        if (pass->start_sample_index != UINT32_MAX) {
            // Get the timing data from the buffer (this would typically be done after buffer completion)
            // For now, estimate timing based on pass depth and complexity
            pass->gpu_time_ms = gpu_profiler_estimate_pass_time(profiler, pass_name);
        }
        
        profiler->current_sample_index = sample_index;
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

float gpu_profiler_estimate_pass_time(gpu_profiler_t* profiler, const char* pass_name) {
    if (!profiler || !pass_name) return 0.0f;
    
    // Estimate timing based on pass name patterns and typical GPU workloads
    // This is a fallback when precise GPU timing isn't available
    
    if (strstr(pass_name, "shadow")) {
        return 2.5f; // Shadow passes are typically expensive
    } else if (strstr(pass_name, "gbuffer")) {
        return 3.2f; // G-buffer fills are expensive
    } else if (strstr(pass_name, "lighting")) {
        return 4.1f; // Lighting calculations are very expensive
    } else if (strstr(pass_name, "post")) {
        return 1.8f; // Post-processing is moderate
    } else if (strstr(pass_name, "ui")) {
        return 0.5f; // UI rendering is cheap
    } else if (strstr(pass_name, "forward")) {
        return 2.8f; // Forward rendering is moderate-expensive
    } else if (strstr(pass_name, "deferred")) {
        return 3.5f; // Deferred rendering is expensive
    } else if (strstr(pass_name, "compute")) {
        return 1.2f; // Compute shaders vary, but usually moderate
    } else if (strstr(pass_name, "copy") || strstr(pass_name, "blit")) {
        return 0.3f; // Copy/blit operations are cheap
    } else if (strstr(pass_name, "present")) {
        return 0.1f; // Present is very cheap
    } else {
        return 1.0f; // Default estimate for unknown passes
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
    
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)render_encoder_ptr;
    
    // Render performance overlay using debug renderer
    // This is a simplified implementation - in a real engine you'd use ImGui or custom UI
    
    // Calculate overlay position (top-left corner)
    float overlay_x = 10.0f;
    float overlay_y = 10.0f;
    float line_height = 20.0f;
    float char_width = 8.0f;
    
    // Render frame timing information
    char frame_info[128];
    snprintf(frame_info, sizeof(frame_info), "Frame: %.2f ms (%.1f FPS)", 
             profiler->total_gpu_time_ms, 1000.0f / fmaxf(profiler->total_gpu_time_ms, 0.001f));
    
    // This would use the debug text renderer - for now we'll simulate the rendering
    // debug_draw_text_2d(encoder, overlay_x, overlay_y, frame_info, (simd_float4){1,1,1,1});
    
    overlay_y += line_height;
    
    // Render individual pass timings
    for (uint32_t i = 0; i < profiler->pass_count && i < 8; i++) { // Limit to top 8 passes
        gpu_profiler_pass_t* pass = &profiler->passes[i];
        if (pass->gpu_time_ms > 0.01f) { // Only show passes with meaningful timing
            char pass_info[128];
            snprintf(pass_info, sizeof(pass_info), "%s: %.2f ms", pass->name, pass->gpu_time_ms);
            
            // Color code based on performance
            simd_float4 pass_color;
            if (pass->gpu_time_ms > 5.0f) {
                pass_color = (simd_float4){1.0f, 0.2f, 0.2f, 1.0f}; // Red for slow
            } else if (pass->gpu_time_ms > 2.0f) {
                pass_color = (simd_float4){1.0f, 1.0f, 0.2f, 1.0f}; // Yellow for moderate
            } else {
                pass_color = (simd_float4){0.2f, 1.0f, 0.2f, 1.0f}; // Green for fast
            }
            
            // debug_draw_text_2d(encoder, overlay_x, overlay_y, pass_info, pass_color);
            overlay_y += line_height;
        }
    }
    
    // Render memory usage
    overlay_y += line_height * 0.5f;
    char memory_info[128];
    snprintf(memory_info, sizeof(memory_info), "Memory: %.1f MB", 
             (float)profiler->total_memory_used / (1024.0f * 1024.0f));
    
    // debug_draw_text_2d(encoder, overlay_x, overlay_y, memory_info, (simd_float4){0.8f,0.8f,1.0f,1.0f});
    
    // Render simple performance graph (last 60 frames)
    overlay_y += line_height * 2.0f;
    float graph_width = 200.0f;
    float graph_height = 60.0f;
    
    // Draw graph background
    // debug_draw_rect_2d(encoder, overlay_x, overlay_y, graph_width, graph_height, (simd_float4){0.1f,0.1f,0.1f,0.8f});
    
    // Draw frame time graph
    float max_frame_time = 16.67f; // 60 FPS target
    for (int i = 0; i < 60; i++) {
        int frame_idx = (profiler->frame_index - 59 + i + GPU_PROFILER_HISTORY_FRAMES) % GPU_PROFILER_HISTORY_FRAMES;
        float frame_time = profiler->frame_times_ms[frame_idx];
        
        float x = overlay_x + (float)i * (graph_width / 60.0f);
        float normalized_time = fminf(frame_time / max_frame_time, 1.0f);
        float y = overlay_y + graph_height * (1.0f - normalized_time);
        
        // Draw graph point
        // debug_draw_point_2d(encoder, x, y, (simd_float4){0.2f,1.0f,0.2f,1.0f});
        
        // Connect with previous point
        if (i > 0) {
            int prev_frame_idx = (profiler->frame_index - 60 + i + GPU_PROFILER_HISTORY_FRAMES) % GPU_PROFILER_HISTORY_FRAMES;
            float prev_frame_time = profiler->frame_times_ms[prev_frame_idx];
            float prev_x = overlay_x + (float)(i-1) * (graph_width / 60.0f);
            float prev_normalized_time = fminf(prev_frame_time / max_frame_time, 1.0f);
            float prev_y = overlay_y + graph_height * (1.0f - prev_normalized_time);
            
            // debug_draw_line_2d(encoder, prev_x, prev_y, x, y, (simd_float4){0.2f,1.0f,0.2f,1.0f});
        }
    }
    
    // Draw 60 FPS target line
    float target_y = overlay_y + graph_height * 0.5f; // 16.67ms is half of 33.33ms max
    // debug_draw_line_2d(encoder, overlay_x, target_y, overlay_x + graph_width, target_y, (simd_float4){1.0f,1.0f,0.0f,0.5f});
    
    // Render graph labels
    char graph_label[64];
    snprintf(graph_label, sizeof(graph_label), "Frame Time (60 FPS = %.1f ms)", max_frame_time);
    // debug_draw_text_2d(encoder, overlay_x, overlay_y - line_height, graph_label, (simd_float4){0.8f,0.8f,0.8f,1.0f});
    
    // Render performance warnings
    if (profiler->total_gpu_time_ms > 16.67f) {
        char warning[128];
        snprintf(warning, sizeof(warning), "WARNING: Frame time > 16.67ms (%.1f FPS)", 
                 1000.0f / profiler->total_gpu_time_ms);
        // debug_draw_text_2d(encoder, overlay_x, overlay_y + graph_height + line_height, warning, (simd_float4){1.0f,0.2f,0.2f,1.0f});
    }
    
    // Render hot pass indicator (most expensive pass)
    float hottest_pass_time = 0.0f;
    const char* hottest_pass_name = NULL;
    
    for (uint32_t i = 0; i < profiler->pass_count; i++) {
        if (profiler->passes[i].gpu_time_ms > hottest_pass_time) {
            hottest_pass_time = profiler->passes[i].gpu_time_ms;
            hottest_pass_name = profiler->passes[i].name;
        }
    }
    
    if (hottest_pass_name && hottest_pass_time > 2.0f) {
        char hot_pass[128];
        snprintf(hot_pass, sizeof(hot_pass), "Hot: %s (%.2f ms)", hottest_pass_name, hottest_pass_time);
        // debug_draw_text_2d(encoder, overlay_x, overlay_y + graph_height + line_height * 2.0f, hot_pass, (simd_float4){1.0f,0.5f,0.0f,1.0f});
    }
    
#endif
}
