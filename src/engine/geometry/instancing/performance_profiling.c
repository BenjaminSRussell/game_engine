/*
 * performance_profiling.c
 * Performance profiling implementation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "performance_profiling.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

/* ============================================================================
 * UTILITIES
 * ============================================================================ */

double performance_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

float performance_calculate_percentage(float before, float after) {
    if (before == 0.0f) {
        return 0.0f;
    }
    return ((after - before) / before) * 100.0f;
}

/* ============================================================================
 * PROFILER MANAGEMENT
 * ============================================================================ */

performance_profiler_t* performance_profiler_create(
    uint32_t history_size,
    uint32_t window_size) {
    
    if (history_size == 0 || window_size == 0 || window_size > history_size) {
        return NULL;
    }
    
    performance_profiler_t* profiler = (performance_profiler_t*)calloc(
        1, sizeof(performance_profiler_t));
    if (!profiler) {
        return NULL;
    }
    
    profiler->frame_history = (frame_metrics_t*)calloc(
        history_size, sizeof(frame_metrics_t));
    if (!profiler->frame_history) {
        free(profiler);
        return NULL;
    }
    
    profiler->history_capacity = history_size;
    profiler->history_count = 0;
    profiler->current_frame_index = 0;
    profiler->stats.window_size = window_size;
    profiler->enabled = true;
    profiler->gpu_timing_enabled = false;
    
    snprintf(profiler->label, sizeof(profiler->label), "Profiler_%p", (void*)profiler);
    
    printf("[Profiler] Created with %u frame history, %u frame window\n",
           history_size, window_size);
    
    return profiler;
}

void performance_profiler_destroy(performance_profiler_t* profiler) {
    if (!profiler) {
        return;
    }
    
    free(profiler->frame_history);
    free(profiler);
}

void performance_profiler_set_enabled(performance_profiler_t* profiler, bool enabled) {
    if (profiler) {
        profiler->enabled = enabled;
    }
}

void performance_profiler_reset(performance_profiler_t* profiler) {
    if (!profiler) {
        return;
    }
    
    memset(profiler->frame_history, 0, 
           profiler->history_capacity * sizeof(frame_metrics_t));
    memset(&profiler->stats, 0, sizeof(performance_stats_t));
    
    profiler->stats.window_size = profiler->history_capacity;
    profiler->history_count = 0;
    profiler->current_frame_index = 0;
}

/* ============================================================================
 * FRAME TIMING
 * ============================================================================ */

void performance_profiler_begin_frame(performance_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    profiler->frame_start_time = performance_get_time_ms();
}

void performance_profiler_end_frame(performance_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    double frame_end_time = performance_get_time_ms();
    
    // Get current frame metrics
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->cpu_time_total = frame_end_time - profiler->frame_start_time;
    
    // Calculate derived metrics
    if (current->total_instances > 0) {
        current->overall_cull_rate = 
            (1.0f - ((float)current->visible_instances / current->total_instances)) * 100.0f;
    }
    
    if (current->draw_calls_before > 0) {
        current->draw_call_reduction = 
            (1.0f - ((float)current->draw_calls_after / current->draw_calls_before)) * 100.0f;
    }
    
    // Update rolling statistics
    profiler->stats.frame_count++;
    
    // Update averages (simple moving average over window)
    uint32_t window_start = (profiler->current_frame_index >= profiler->stats.window_size) ?
        (profiler->current_frame_index - profiler->stats.window_size + 1) : 0;
    
    double sum_cpu = 0.0, sum_gpu = 0.0;
    float sum_reduction = 0.0f, sum_cull = 0.0f;
    
    for (uint32_t i = window_start; i <= profiler->current_frame_index && i < profiler->history_capacity; i++) {
        sum_cpu += profiler->frame_history[i].cpu_time_total;
        sum_gpu += profiler->frame_history[i].gpu_time_total;
        sum_reduction += profiler->frame_history[i].draw_call_reduction;
        sum_cull += profiler->frame_history[i].overall_cull_rate;
    }
    
    uint32_t count = (profiler->current_frame_index - window_start + 1);
    profiler->stats.avg_cpu_time = sum_cpu / count;
    profiler->stats.avg_gpu_time = sum_gpu / count;
    profiler->stats.avg_draw_call_reduction = sum_reduction / count;
    profiler->stats.avg_cull_rate = sum_cull / count;
    profiler->stats.avg_frame_time = profiler->stats.avg_cpu_time + profiler->stats.avg_gpu_time;
    
    // Update peaks
    if (current->cpu_time_total > profiler->stats.peak_cpu_time) {
        profiler->stats.peak_cpu_time = current->cpu_time_total;
    }
    if (current->gpu_time_total > profiler->stats.peak_gpu_time) {
        profiler->stats.peak_gpu_time = current->gpu_time_total;
    }
    if (current->total_instances > profiler->stats.peak_instance_count) {
        profiler->stats.peak_instance_count = current->total_instances;
    }
    if (current->draw_calls_after > profiler->stats.peak_draw_calls) {
        profiler->stats.peak_draw_calls = current->draw_calls_after;
    }
    
    // Update totals
    profiler->stats.total_instances_rendered += current->visible_instances;
    profiler->stats.total_draw_calls_saved += 
        (current->draw_calls_before - current->draw_calls_after);
    
    // Move to next frame
    profiler->current_frame_index = (profiler->current_frame_index + 1) % profiler->history_capacity;
    if (profiler->history_count < profiler->history_capacity) {
        profiler->history_count++;
    }
}

void performance_profiler_begin_culling(performance_profiler_t* profiler) {
    if (profiler && profiler->enabled) {
        profiler->culling_start_time = performance_get_time_ms();
    }
}

void performance_profiler_end_culling(performance_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    double end_time = performance_get_time_ms();
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->cpu_time_culling = end_time - profiler->culling_start_time;
}

void performance_profiler_begin_batch_generation(performance_profiler_t* profiler) {
    if (profiler && profiler->enabled) {
        profiler->batch_start_time = performance_get_time_ms();
    }
}

void performance_profiler_end_batch_generation(performance_profiler_t* profiler) {
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    double end_time = performance_get_time_ms();
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->cpu_time_batch_generation = end_time - profiler->batch_start_time;
}

/* ============================================================================
 * METRICS RECORDING
 * ============================================================================ */

void performance_profiler_record_instances(
    performance_profiler_t* profiler,
    uint32_t total_instances,
    uint32_t visible_instances) {
    
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->total_instances = total_instances;
    current->visible_instances = visible_instances;
}

void performance_profiler_record_draw_calls(
    performance_profiler_t* profiler,
    uint32_t before_instancing,
    uint32_t after_instancing) {
    
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->draw_calls_before = before_instancing;
    current->draw_calls_after = after_instancing;
}

void performance_profiler_record_culling(
    performance_profiler_t* profiler,
    uint32_t frustum_culled,
    uint32_t distance_culled,
    uint32_t occlusion_culled,
    uint32_t total_tested) {
    
    if (!profiler || !profiler->enabled || total_tested == 0) {
        return;
    }
    
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->frustum_cull_rate = ((float)frustum_culled / total_tested) * 100.0f;
    current->distance_cull_rate = ((float)distance_culled / total_tested) * 100.0f;
    current->occlusion_cull_rate = ((float)occlusion_culled / total_tested) * 100.0f;
}

void performance_profiler_record_memory(
    performance_profiler_t* profiler,
    size_t instance_buffer_bytes,
    size_t visible_buffer_bytes,
    size_t command_buffer_bytes) {
    
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->instance_buffer_bytes = instance_buffer_bytes;
    current->visible_buffer_bytes = visible_buffer_bytes;
    current->command_buffer_bytes = command_buffer_bytes;
}

void performance_profiler_record_gpu_time(
    performance_profiler_t* profiler,
    double culling_time_ms,
    double rendering_time_ms) {
    
    if (!profiler || !profiler->enabled) {
        return;
    }
    
    frame_metrics_t* current = &profiler->frame_history[profiler->current_frame_index];
    current->gpu_time_culling = culling_time_ms;
    current->gpu_time_rendering = rendering_time_ms;
    current->gpu_time_total = culling_time_ms + rendering_time_ms;
    
    profiler->gpu_timing_enabled = true;
}

/* ============================================================================
 * STATISTICS & REPORTING
 * ============================================================================ */

frame_metrics_t performance_profiler_get_current_frame(
    const performance_profiler_t* profiler) {
    
    frame_metrics_t metrics = {0};
    if (profiler) {
        metrics = profiler->frame_history[profiler->current_frame_index];
    }
    return metrics;
}

performance_stats_t performance_profiler_get_stats(
    const performance_profiler_t* profiler) {
    
    performance_stats_t stats = {0};
    if (profiler) {
        stats = profiler->stats;
    }
    return stats;
}

frame_metrics_t performance_profiler_get_frame(
    const performance_profiler_t* profiler,
    uint32_t frame_offset) {
    
    frame_metrics_t metrics = {0};
    if (!profiler || frame_offset >= profiler->history_count) {
        return metrics;
    }
    
    uint32_t index = (profiler->current_frame_index + profiler->history_capacity - frame_offset) 
                     % profiler->history_capacity;
    return profiler->frame_history[index];
}

void performance_profiler_print_report(const performance_profiler_t* profiler) {
    if (!profiler) {
        return;
    }
    
    printf("\n=== Performance Profiler Report ===\n");
    printf("Frame Count: %u\n", profiler->stats.frame_count);
    printf("\n");
    
    printf("--- Timing (Averages) ---\n");
    printf("  CPU Time: %.2f ms\n", profiler->stats.avg_cpu_time);
    printf("  GPU Time: %.2f ms\n", profiler->stats.avg_gpu_time);
    printf("  Frame Time: %.2f ms (%.1f FPS)\n", 
           profiler->stats.avg_frame_time,
           1000.0 / profiler->stats.avg_frame_time);
    printf("\n");
    
    printf("--- Draw Call Reduction ---\n");
    printf("  Average Reduction: %.1f%%\n", profiler->stats.avg_draw_call_reduction);
    printf("  Total Calls Saved: %llu\n", 
           (unsigned long long)profiler->stats.total_draw_calls_saved);
    printf("\n");
    
    printf("--- Culling Efficiency ---\n");
    printf("  Average Cull Rate: %.1f%%\n", profiler->stats.avg_cull_rate);
    printf("  Total Instances Rendered: %llu\n", 
           (unsigned long long)profiler->stats.total_instances_rendered);
    printf("\n");
    
    printf("--- Peaks ---\n");
    printf("  Peak CPU Time: %.2f ms\n", profiler->stats.peak_cpu_time);
    printf("  Peak GPU Time: %.2f ms\n", profiler->stats.peak_gpu_time);
    printf("  Peak Instance Count: %u\n", profiler->stats.peak_instance_count);
    printf("  Peak Draw Calls: %u\n", profiler->stats.peak_draw_calls);
    printf("\n");
    
    // Current frame details
    frame_metrics_t current = performance_profiler_get_current_frame(profiler);
    printf("--- Current Frame ---\n");
    printf("  Instances: %u visible / %u total (%.1f%% culled)\n",
           current.visible_instances, current.total_instances, current.overall_cull_rate);
    printf("  Draw Calls: %u (from %u, %.1f%% reduction)\n",
           current.draw_calls_after, current.draw_calls_before, current.draw_call_reduction);
    printf("  Memory: %.2f MB instance + %.2f MB visible + %.2f MB commands\n",
           current.instance_buffer_bytes / (1024.0 * 1024.0),
           current.visible_buffer_bytes / (1024.0 * 1024.0),
           current.command_buffer_bytes / (1024.0 * 1024.0));
}

int performance_profiler_export_csv(
    const performance_profiler_t* profiler,
    const char* filename) {
    
    if (!profiler || !filename) {
        return -1;
    }
    
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[Profiler] Failed to open %s for writing\n", filename);
        return -1;
    }
    
    // Write CSV header
    fprintf(f, "Frame,CPU_Time,GPU_Time,Total_Time,");
    fprintf(f, "Total_Instances,Visible_Instances,Cull_Rate,");
    fprintf(f, "Draw_Before,Draw_After,Draw_Reduction,");
    fprintf(f, "Frustum_Cull,Distance_Cull,Occlusion_Cull\n");
    
    // Write frame data
    for (uint32_t i = 0; i < profiler->history_count; i++) {
        const frame_metrics_t* frame = &profiler->frame_history[i];
        
        fprintf(f, "%u,%.3f,%.3f,%.3f,",
                i, frame->cpu_time_total, frame->gpu_time_total,
                frame->cpu_time_total + frame->gpu_time_total);
        fprintf(f, "%u,%u,%.2f,",
                frame->total_instances, frame->visible_instances, frame->overall_cull_rate);
        fprintf(f, "%u,%u,%.2f,",
                frame->draw_calls_before, frame->draw_calls_after, frame->draw_call_reduction);
        fprintf(f, "%.2f,%.2f,%.2f\n",
                frame->frustum_cull_rate, frame->distance_cull_rate, frame->occlusion_cull_rate);
    }
    
    fclose(f);
    
    printf("[Profiler] Exported %u frames to %s\n", profiler->history_count, filename);
    return 0;
}
