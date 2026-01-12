// GPU Profiling with Metal Performance Shaders implementation
#include "profiling/gpu_profiling_metal.h"
#include <core/logger.h>
#include <math/vec3.h>
#include <math/math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

// Global GPU profiling system instance
GPUProfilingSystem* g_gpu_profiling_system = NULL;

// MARK: - Helper Functions

static u64 get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000000000LL + ts.tv_nsec);
}

static u64 get_current_time_ms(void) {
    return get_current_time_ns() / 1000000;
}

static float calculate_average(float* values, u32 count) {
    if (!values || count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (u32 i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum / count;
}

static float calculate_variance(float* values, u32 count, float mean) {
    if (!values || count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (u32 i = 0; i < count; i++) {
        float diff = values[i] - mean;
        sum += diff * diff;
    }
    return sum / count;
}

// MARK: - GPU Profiling System Management

bool gpu_profiling_init(GPUProfilingSystem* system, u32 max_sessions, u32 max_frames, u32 max_counters) {
    if (!system || max_sessions == 0 || max_frames == 0 || max_counters == 0) {
        LOG_ERROR("Invalid parameters for gpu_profiling_init");
        return false;
    }
    
    memset(system, 0, sizeof(GPUProfilingSystem));
    
    // Allocate sessions array
    system->sessions = (GPUProfilingSession*)calloc(max_sessions, sizeof(GPUProfilingSession));
    if (!system->sessions) {
        LOG_ERROR("Failed to allocate sessions array");
        return false;
    }
    system->session_capacity = max_sessions;
    
    // Allocate global counters array
    system->global_counters = (GPUPerformanceCounter*)calloc(max_counters * 2, sizeof(GPUPerformanceCounter));
    if (!system->global_counters) {
        free(system->sessions);
        LOG_ERROR("Failed to allocate global counters array");
        return false;
    }
    system->global_counter_capacity = max_counters * 2;
    
    // Set default thresholds
    system->frame_time_warning_threshold_ms = 16.67f; // 60 FPS
    system->frame_time_critical_threshold_ms = 33.33f; // 30 FPS
    system->memory_warning_threshold_mb = 1024.0f; // 1GB
    system->temperature_warning_threshold_c = 85.0f;
    system->utilization_warning_threshold_percent = 90.0f;
    
    // Set default configuration
    system->max_sessions = max_sessions;
    system->max_frames_per_session = max_frames;
    system->max_counters_per_session = max_counters;
    system->enable_auto_export = false;
    system->export_interval_frames = 1000;
    
    // Initialize Metal device if available
#ifdef __APPLE__
    system->metal_device = MTLCreateSystemDefaultDevice();
    if (system->metal_device) {
        system->command_queue = [system->metal_device newCommandQueue];
        LOG_INFO("Metal device initialized for GPU profiling");
    } else {
        LOG_WARN("Failed to create Metal device for GPU profiling");
    }
#endif
    
    system->is_initialized = true;
    
    // Set global instance
    g_gpu_profiling_system = system;
    
    LOG_INFO("GPU profiling system initialized (sessions: %u, frames: %u, counters: %u)", 
             max_sessions, max_frames, max_counters);
    return true;
}

void gpu_profiling_shutdown(GPUProfilingSystem* system) {
    if (!system) {
        return;
    }
    
    // Stop and destroy all sessions
    for (u32 i = 0; i < system->session_count; i++) {
        GPUProfilingSession* session = &system->sessions[i];
        if (session->is_active) {
            gpu_profiling_stop_session(session);
        }
        
        if (session->frames) {
            free(session->frames);
        }
        
        if (session->counters) {
            for (u32 j = 0; j < session->counter_count; j++) {
                GPUPerformanceCounter* counter = &session->counters[j];
                if (counter->history_buffer) {
                    free(counter->history_buffer);
                }
            }
            free(session->counters);
        }
        
#ifdef __APPLE__
        if (session->query_buffer) {
            [session->query_buffer release];
        }
        if (session->result_buffer) {
            [session->result_buffer release];
        }
        if (session->command_queue) {
            [session->command_queue release];
        }
#endif
    }
    
    // Clean up global counters
    if (system->global_counters) {
        for (u32 i = 0; i < system->global_counter_count; i++) {
            if (system->global_counters[i].history_buffer) {
                free(system->global_counters[i].history_buffer);
            }
        }
        free(system->global_counters);
    }
    
    // Clean up Metal resources
#ifdef __APPLE__
    if (system->command_queue) {
        [system->command_queue release];
    }
    if (system->metal_device) {
        [system->metal_device release];
    }
#endif
    
    // Free sessions array
    free(system->sessions);
    
    // Clear global instance
    g_gpu_profiling_system = NULL;
    
    memset(system, 0, sizeof(GPUProfilingSystem));
    LOG_INFO("GPU profiling system shutdown");
}

// MARK: - Session Management

GPUProfilingSession* gpu_profiling_create_session(GPUProfilingSystem* system, const char* session_name) {
    if (!system || !session_name || system->session_count >= system->session_capacity) {
        return NULL;
    }
    
    GPUProfilingSession* session = &system->sessions[system->session_count++];
    memset(session, 0, sizeof(GPUProfilingSession));
    
    strncpy(session->session_name, session_name, sizeof(session->session_name) - 1);
    session->session_name[sizeof(session->session_name) - 1] = '\0';
    
    // Allocate frames array
    session->frames = (GPUFrameProfile*)calloc(system->max_frames_per_session, sizeof(GPUFrameProfile));
    if (!session->frames) {
        LOG_ERROR("Failed to allocate frames array for session");
        system->session_count--;
        return NULL;
    }
    session->frame_capacity = system->max_frames_per_session;
    
    // Allocate counters array
    session->counters = (GPUPerformanceCounter*)calloc(system->max_counters_per_session, sizeof(GPUPerformanceCounter));
    if (!session->counters) {
        free(session->frames);
        LOG_ERROR("Failed to allocate counters array for session");
        system->session_count--;
        return NULL;
    }
    session->counter_capacity = system->max_counters_per_session;
    
    // Set default settings
    session->enable_timing = true;
    session->enable_statistics = true;
    session->enable_memory_tracking = true;
    session->enable_power_tracking = false; // Default to false as it's expensive
    session->target_frame_rate = 60;
    session->max_frame_time_ms = 33.33f;
    
    // Initialize Metal resources
#ifdef __APPLE__
    if (system->metal_device) {
        session->metal_device = system->metal_device;
        session->command_queue = [system->metal_device newCommandQueue];
        gpu_profiling_create_metal_query_buffers(session);
    }
#endif
    
    system->total_sessions_created++;
    LOG_DEBUG("Created GPU profiling session: %s", session_name);
    return session;
}

bool gpu_profiling_start_session(GPUProfilingSession* session) {
    if (!session || session->is_active) {
        return false;
    }
    
    session->is_active = true;
    session->session_start_time_ns = get_current_time_ns();
    session->current_frame_index = 0;
    
    // Reset frame data
    memset(session->frames, 0, session->frame_capacity * sizeof(GPUFrameProfile));
    
    // Reset counters
    for (u32 i = 0; i < session->counter_count; i++) {
        GPUPerformanceCounter* counter = &session->counters[i];
        counter->sample_count = 0;
        counter->total_samples = 0;
        counter->running_average = 0.0f;
        counter->variance = 0.0f;
        counter->standard_deviation = 0.0f;
    }
    
    LOG_INFO("Started GPU profiling session: %s", session->session_name);
    
    if (session->on_session_started) {
        session->on_session_started(session->session_name);
    }
    
    return true;
}

bool gpu_profiling_stop_session(GPUProfilingSession* session) {
    if (!session || !session->is_active) {
        return false;
    }
    
    session->is_active = false;
    session->session_end_time_ns = get_current_time_ns();
    
    // Calculate final statistics
    gpu_profiling_calculate_session_statistics(session);
    
    LOG_INFO("Stopped GPU profiling session: %s (frames: %u, duration: %.2f ms)", 
             session->session_name, session->frame_count,
             (session->session_end_time_ns - session->session_start_time_ns) / 1000000.0f);
    
    if (session->on_session_ended) {
        session->on_session_ended(session->session_name);
    }
    
    return true;
}

// MARK: - Frame Profiling

bool gpu_profiling_begin_frame(GPUProfilingSession* session, u32 frame_number) {
    if (!session || !session->is_active || session->current_frame_index >= session->frame_capacity) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index];
    memset(frame, 0, sizeof(GPUFrameProfile));
    
    frame->frame_number = frame_number;
    frame->frame_start_time_ns = get_current_time_ns();
    
    // Begin Metal timing if available
#ifdef __APPLE__
    if (session->command_queue) {
        id<MTLCommandBuffer> command_buffer = [session->command_queue commandBuffer];
        gpu_profiling_begin_metal_timing(session, command_buffer);
        [command_buffer commit];
    }
#endif
    
    return true;
}

bool gpu_profiling_end_frame(GPUProfilingSession* session) {
    if (!session || !session->is_active || session->current_frame_index >= session->frame_capacity) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index];
    frame->frame_end_time_ns = get_current_time_ns();
    frame->frame_time_ms = (frame->frame_end_time_ns - frame->frame_start_time_ns) / 1000000.0f;
    
    // End Metal timing and collect statistics
#ifdef __APPLE__
    if (session->command_queue) {
        id<MTLCommandBuffer> command_buffer = [session->command_queue commandBuffer];
        gpu_profiling_end_metal_timing(session, command_buffer);
        [command_buffer commit];
        [command_buffer waitUntilCompleted];
        
        gpu_profiling_collect_metal_statistics(session);
    }
#endif
    
    // Update frame-based counters
    for (u32 i = 0; i < session->counter_count; i++) {
        GPUPerformanceCounter* counter = &session->counters[i];
        
        switch (counter->type) {
            case GPU_METRIC_FRAME_TIME:
                gpu_profiling_update_counter(session, counter, frame->frame_time_ms);
                break;
            case GPU_METRIC_DRAW_CALLS:
                gpu_profiling_update_counter_u32(session, counter, frame->draw_calls);
                break;
            case GPU_METRIC_TRIANGLES:
                gpu_profiling_update_counter_u32(session, counter, frame->triangle_count);
                break;
            case GPU_METRIC_VERTICES:
                gpu_profiling_update_counter_u32(session, counter, frame->vertex_count);
                break;
            default:
                break;
        }
    }
    
    // Check performance thresholds
    gpu_profiling_check_performance_thresholds(session);
    
    // Trigger callback
    if (session->on_frame_completed) {
        session->on_frame_completed(frame);
    }
    
    session->current_frame_index++;
    if (session->current_frame_index > session->frame_count) {
        session->frame_count = session->current_frame_index;
    }
    
    return true;
}

bool gpu_profiling_record_draw_call(GPUProfilingSession* session, u32 triangle_count, u32 vertex_count) {
    if (!session || !session->is_active || session->current_frame_index == 0) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index - 1];
    frame->draw_calls++;
    frame->triangle_count += triangle_count;
    frame->vertex_count += vertex_count;
    
    return true;
}

bool gpu_profiling_record_compute_dispatch(GPUProfilingSession* session, u32 thread_count_x, u32 thread_count_y, u32 thread_count_z) {
    if (!session || !session->is_active || session->current_frame_index == 0) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index - 1];
    frame->compute_thread_count += thread_count_x * thread_count_y * thread_count_z;
    
    return true;
}

bool gpu_profiling_record_memory_operation(GPUProfilingSession* session, u64 bytes, bool is_texture, bool is_upload) {
    if (!session || !session->is_active || session->current_frame_index == 0) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index - 1];
    
    if (is_upload) {
        if (is_texture) {
            frame->texture_uploads++;
            frame->texture_upload_bytes += bytes;
        } else {
            frame->buffer_uploads++;
            frame->buffer_upload_bytes += bytes;
        }
    }
    
    return true;
}

// MARK: - Performance Counters

GPUPerformanceCounter* gpu_profiling_create_counter(GPUProfilingSession* session, const char* name, 
                                                   GPUMetricType type, GPUDataType data_type) {
    if (!session || !name || session->counter_count >= session->counter_capacity) {
        return NULL;
    }
    
    GPUPerformanceCounter* counter = &session->counters[session->counter_count++];
    memset(counter, 0, sizeof(GPUPerformanceCounter));
    
    strncpy(counter->name, name, sizeof(counter->name) - 1);
    counter->name[sizeof(counter->name) - 1] = '\0';
    
    counter->type = type;
    counter->data_type = data_type;
    counter->is_enabled = true;
    counter->history_capacity = 1024; // Default history size
    
    // Allocate history buffer
    switch (data_type) {
        case GPU_DATA_TYPE_FLOAT:
            counter->history_buffer = calloc(counter->history_capacity, sizeof(float));
            break;
        case GPU_DATA_TYPE_UINT32:
            counter->history_buffer = calloc(counter->history_capacity, sizeof(u32));
            break;
        case GPU_DATA_TYPE_UINT64:
            counter->history_buffer = calloc(counter->history_capacity, sizeof(u64));
            break;
        default:
            counter->history_buffer = calloc(counter->history_capacity, sizeof(float));
            break;
    }
    
    if (!counter->history_buffer) {
        LOG_ERROR("Failed to allocate history buffer for counter: %s", name);
        session->counter_count--;
        return NULL;
    }
    
    // Initialize min/max values
    switch (data_type) {
        case GPU_DATA_TYPE_FLOAT:
            counter->min_value.float_value = FLT_MAX;
            counter->max_value.float_value = FLT_MIN;
            break;
        case GPU_DATA_TYPE_UINT32:
            counter->min_value.uint32_value = UINT32_MAX;
            counter->max_value.uint32_value = 0;
            break;
        case GPU_DATA_TYPE_UINT64:
            counter->min_value.uint64_value = UINT64_MAX;
            counter->max_value.uint64_value = 0;
            break;
        default:
            break;
    }
    
    LOG_DEBUG("Created GPU performance counter: %s", name);
    return counter;
}

bool gpu_profiling_update_counter(GPUProfilingSession* session, GPUPerformanceCounter* counter, float value) {
    if (!session || !counter || !counter->is_enabled) {
        return false;
    }
    
    // Update current value
    counter->current_value.float_value = value;
    
    // Update min/max
    if (value < counter->min_value.float_value) {
        counter->min_value.float_value = value;
    }
    if (value > counter->max_value.float_value) {
        counter->max_value.float_value = value;
    }
    
    // Update history
    if (counter->history_buffer && counter->history_capacity > 0) {
        float* history = (float*)counter->history_buffer;
        history[counter->history_write_index] = value;
        counter->history_write_index = (counter->history_write_index + 1) % counter->history_capacity;
        
        if (counter->history_count < counter->history_capacity) {
            counter->history_count++;
        }
    }
    
    // Update statistics
    counter->sample_count++;
    counter->total_samples++;
    
    if (counter->sample_count == 1) {
        counter->running_average = value;
        counter->variance = 0.0f;
    } else {
        float alpha = 1.0f / counter->sample_count;
        float delta = value - counter->running_average;
        counter->running_average += alpha * delta;
        counter->variance = (1.0f - alpha) * (counter->variance + alpha * delta * delta);
    }
    
    counter->standard_deviation = sqrtf(counter->variance);
    counter->average_value.float_value = counter->running_average;
    
    return true;
}

bool gpu_profiling_update_counter_u32(GPUProfilingSession* session, GPUPerformanceCounter* counter, u32 value) {
    return gpu_profiling_update_counter(session, counter, (float)value);
}

bool gpu_profiling_update_counter_u64(GPUProfilingSession* session, GPUPerformanceCounter* counter, u64 value) {
    return gpu_profiling_update_counter(session, counter, (float)value);
}

// MARK: - Metal Performance Shaders Integration

#ifdef __APPLE__
bool gpu_profiling_create_metal_query_buffers(GPUProfilingSession* session) {
    if (!session || !session->metal_device) {
        return false;
    }
    
    // Create query buffer for timing
    MTLResourceOptions options = MTLResourceStorageModeShared;
    session->query_buffer = [session->metal_device newBufferWithLength:1024 * sizeof(u64) options:options];
    session->result_buffer = [session->metal_device newBufferWithLength:1024 * sizeof(u64) options:options];
    
    if (!session->query_buffer || !session->result_buffer) {
        LOG_ERROR("Failed to create Metal query buffers");
        return false;
    }
    
    return true;
}

bool gpu_profiling_begin_metal_timing(GPUProfilingSession* session, id<MTLCommandBuffer> command_buffer) {
    if (!session || !command_buffer || !session->query_buffer) {
        return false;
    }
    
    // Begin timestamp query
    [command_buffer sampleTimestampsInBuffer:session->query_buffer
                                     atIndex:0
                              withBarrier:YES];
    
    return true;
}

bool gpu_profiling_end_metal_timing(GPUProfilingSession* session, id<MTLCommandBuffer> command_buffer) {
    if (!session || !command_buffer || !session->query_buffer) {
        return false;
    }
    
    // End timestamp query
    [command_buffer sampleTimestampsInBuffer:session->query_buffer
                                     atIndex:1
                              withBarrier:YES];
    
    // Resolve timestamps
    [command_buffer resolveTimestamps:session->result_buffer
                              inBuffer:session->query_buffer
                               atIndex:0
                               count:2];
    
    return true;
}

bool gpu_profiling_collect_metal_statistics(GPUProfilingSession* session) {
    if (!session || !session->result_buffer) {
        return false;
    }
    
    u64* timestamps = (u64*)[session->result_buffer contents];
    if (timestamps[0] > 0 && timestamps[1] > 0) {
        u64 elapsed_ns = timestamps[1] - timestamps[0];
        float elapsed_ms = elapsed_ns / 1000000.0f;
        
        // Update frame time counter if available
        GPUPerformanceCounter* frame_time_counter = gpu_profiling_get_counter(session, "metal_frame_time");
        if (frame_time_counter) {
            gpu_profiling_update_counter(session, frame_time_counter, elapsed_ms);
        }
    }
    
    return true;
}
#endif

// MARK: - Statistics and Analysis

void gpu_profiling_calculate_session_statistics(GPUProfilingSession* session) {
    if (!session || session->frame_count == 0) {
        return;
    }
    
    // Calculate frame statistics
    float total_frame_time = 0.0f;
    u32 total_draw_calls = 0;
    u32 total_triangles = 0;
    u32 total_vertices = 0;
    
    for (u32 i = 0; i < session->frame_count; i++) {
        GPUFrameProfile* frame = &session->frames[i];
        total_frame_time += frame->frame_time_ms;
        total_draw_calls += frame->draw_calls;
        total_triangles += frame->triangle_count;
        total_vertices += frame->vertex_count;
    }
    
    if (session->frame_count > 0) {
        float avg_frame_time = total_frame_time / session->frame_count;
        float fps = 1000.0f / avg_frame_time;
        
        LOG_INFO("Session Statistics: %s", session->session_name);
        LOG_INFO("  Frames: %u", session->frame_count);
        LOG_INFO("  Average Frame Time: %.2f ms", avg_frame_time);
        LOG_INFO("  Average FPS: %.1f", fps);
        LOG_INFO("  Total Draw Calls: %u", total_draw_calls);
        LOG_INFO("  Total Triangles: %u", total_triangles);
        LOG_INFO("  Total Vertices: %u", total_vertices);
    }
}

float gpu_profiling_get_average_frame_time(GPUProfilingSession* session) {
    if (!session || session->frame_count == 0) {
        return 0.0f;
    }
    
    float total = 0.0f;
    for (u32 i = 0; i < session->frame_count; i++) {
        total += session->frames[i].frame_time_ms;
    }
    
    return total / session->frame_count;
}

// MARK: - Performance Monitoring

bool gpu_profiling_check_performance_thresholds(GPUProfilingSession* session) {
    if (!session || !session->is_active || session->current_frame_index == 0) {
        return false;
    }
    
    GPUFrameProfile* frame = &session->frames[session->current_frame_index - 1];
    bool warning_triggered = false;
    
    // Check frame time thresholds
    if (g_gpu_profiling_system) {
        if (frame->frame_time_ms > g_gpu_profiling_system->frame_time_critical_threshold_ms) {
            LOG_ERROR("Critical frame time: %.2f ms (threshold: %.2f ms)", 
                     frame->frame_time_ms, g_gpu_profiling_system->frame_time_critical_threshold_ms);
            warning_triggered = true;
        } else if (frame->frame_time_ms > g_gpu_profiling_system->frame_time_warning_threshold_ms) {
            LOG_WARN("High frame time: %.2f ms (threshold: %.2f ms)", 
                    frame->frame_time_ms, g_gpu_profiling_system->frame_time_warning_threshold_ms);
            warning_triggered = true;
        }
        
        // Check memory usage
        float memory_mb = frame->memory_usage_bytes / (1024.0f * 1024.0f);
        if (memory_mb > g_gpu_profiling_system->memory_warning_threshold_mb) {
            LOG_WARN("High memory usage: %.1f MB (threshold: %.1f MB)", 
                    memory_mb, g_gpu_profiling_system->memory_warning_threshold_mb);
            warning_triggered = true;
        }
        
        // Check temperature
        if (frame->temperature_celsius > g_gpu_profiling_system->temperature_warning_threshold_c) {
            LOG_WARN("High GPU temperature: %.1fC (threshold: %.1fC)", 
                    frame->temperature_celsius, g_gpu_profiling_system->temperature_warning_threshold_c);
            warning_triggered = true;
        }
        
        // Check utilization
        if (frame->gpu_utilization_percent > g_gpu_profiling_system->utilization_warning_threshold_percent) {
            LOG_WARN("High GPU utilization: %.1f%% (threshold: %.1f%%)", 
                    frame->gpu_utilization_percent, g_gpu_profiling_system->utilization_warning_threshold_percent);
            warning_triggered = true;
        }
    }
    
    if (warning_triggered && g_gpu_profiling_system) {
        g_gpu_profiling_system->total_warnings_triggered++;
    }
    
    return warning_triggered;
}

// MARK: - Export and Reporting

bool gpu_profiling_export_session_to_json(GPUProfilingSession* session, const char* filename) {
    if (!session || !filename) {
        return false;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        LOG_ERROR("Failed to open JSON file: %s", filename);
        return false;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"session_name\": \"%s\",\n", session->session_name);
    fprintf(fp, "  \"is_active\": %s,\n", session->is_active ? "true" : "false");
    fprintf(fp, "  \"session_start_time_ns\": %llu,\n", session->session_start_time_ns);
    fprintf(fp, "  \"session_end_time_ns\": %llu,\n", session->session_end_time_ns);
    fprintf(fp, "  \"frame_count\": %u,\n", session->frame_count);
    fprintf(fp, "  \"counter_count\": %u,\n", session->counter_count);
    
    // Export frame data
    fprintf(fp, "  \"frames\": [\n");
    for (u32 i = 0; i < session->frame_count; i++) {
        GPUFrameProfile* frame = &session->frames[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"frame_number\": %u,\n", frame->frame_number);
        fprintf(fp, "      \"frame_time_ms\": %.4f,\n", frame->frame_time_ms);
        fprintf(fp, "      \"draw_calls\": %u,\n", frame->draw_calls);
        fprintf(fp, "      \"triangle_count\": %u,\n", frame->triangle_count);
        fprintf(fp, "      \"vertex_count\": %u,\n", frame->vertex_count);
        fprintf(fp, "      \"fragment_count\": %u,\n", frame->fragment_count);
        fprintf(fp, "      \"memory_usage_bytes\": %llu,\n", frame->memory_usage_bytes);
        fprintf(fp, "      \"gpu_utilization_percent\": %.2f,\n", frame->gpu_utilization_percent);
        fprintf(fp, "      \"temperature_celsius\": %.2f\n", frame->temperature_celsius);
        fprintf(fp, "    }");
        if (i < session->frame_count - 1) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "  ],\n");
    
    // Export counter data
    fprintf(fp, "  \"counters\": [\n");
    for (u32 i = 0; i < session->counter_count; i++) {
        GPUPerformanceCounter* counter = &session->counters[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"name\": \"%s\",\n", counter->name);
        fprintf(fp, "      \"type\": %u,\n", counter->type);
        fprintf(fp, "      \"data_type\": %u,\n", counter->data_type);
        fprintf(fp, "      \"current_value\": %.4f,\n", counter->current_value.float_value);
        fprintf(fp, "      \"average_value\": %.4f,\n", counter->average_value.float_value);
        fprintf(fp, "      \"min_value\": %.4f,\n", counter->min_value.float_value);
        fprintf(fp, "      \"max_value\": %.4f,\n", counter->max_value.float_value);
        fprintf(fp, "      \"sample_count\": %llu,\n", counter->sample_count);
        fprintf(fp, "      \"standard_deviation\": %.4f\n", counter->standard_deviation);
        fprintf(fp, "    }");
        if (i < session->counter_count - 1) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    LOG_INFO("Exported GPU profiling session to JSON: %s", filename);
    return true;
}

void gpu_profiling_print_session_summary(GPUProfilingSession* session) {
    if (!session) {
        printf("GPU profiling session is NULL\n");
        return;
    }
    
    printf("=== GPU PROFILING SESSION SUMMARY ===\n");
    printf("Session Name: %s\n", session->session_name);
    printf("Status: %s\n", session->is_active ? "Active" : "Inactive");
    printf("Frames: %u / %u\n", session->frame_count, session->frame_capacity);
    printf("Counters: %u / %u\n", session->counter_count, session->counter_capacity);
    
    if (session->frame_count > 0) {
        float avg_frame_time = gpu_profiling_get_average_frame_time(session);
        printf("Average Frame Time: %.2f ms\n", avg_frame_time);
        printf("Average FPS: %.1f\n", 1000.0f / avg_frame_time);
    }
    
    printf("Timing Enabled: %s\n", session->enable_timing ? "Yes" : "No");
    printf("Statistics Enabled: %s\n", session->enable_statistics ? "Yes" : "No");
    printf("Memory Tracking: %s\n", session->enable_memory_tracking ? "Yes" : "No");
    printf("Power Tracking: %s\n", session->enable_power_tracking ? "Yes" : "No");
    printf("====================================\n");
}

// MARK: - Utility Functions

void gpu_profiling_get_system_statistics(GPUProfilingSystem* system, u32* active_sessions, 
                                       u64* total_frames, u64* total_warnings) {
    if (!system) {
        return;
    }
    
    u32 active = 0;
    u64 frames = 0;
    
    for (u32 i = 0; i < system->session_count; i++) {
        if (system->sessions[i].is_active) {
            active++;
        }
        frames += system->sessions[i].frame_count;
    }
    
    if (active_sessions) *active_sessions = active;
    if (total_frames) *total_frames = frames;
    if (total_warnings) *total_warnings = system->total_warnings_triggered;
}

void gpu_profiling_print_system_statistics(GPUProfilingSystem* system) {
    if (!system) {
        printf("GPU profiling system is NULL\n");
        return;
    }
    
    u32 active_sessions, total_frames, total_warnings;
    gpu_profiling_get_system_statistics(system, &active_sessions, &total_frames, &total_warnings);
    
    printf("=== GPU PROFILING SYSTEM STATISTICS ===\n");
    printf("Total Sessions Created: %llu\n", system->total_sessions_created);
    printf("Active Sessions: %u / %u\n", active_sessions, system->session_count);
    printf("Total Frames Profiled: %llu\n", total_frames);
    printf("Total Warnings Triggered: %llu\n", total_warnings);
    printf("Global Counters: %u / %u\n", system->global_counter_count, system->global_counter_capacity);
    printf("Frame Time Warning Threshold: %.2f ms\n", system->frame_time_warning_threshold_ms);
    printf("Frame Time Critical Threshold: %.2f ms\n", system->frame_time_critical_threshold_ms);
    printf("Memory Warning Threshold: %.1f MB\n", system->memory_warning_threshold_mb);
    printf("Temperature Warning Threshold: %.1fC\n", system->temperature_warning_threshold_c);
    printf("Utilization Warning Threshold: %.1f%%\n", system->utilization_warning_threshold_percent);
    printf("Auto Export: %s\n", system->enable_auto_export ? "Enabled" : "Disabled");
    printf("=====================================\n");
}
