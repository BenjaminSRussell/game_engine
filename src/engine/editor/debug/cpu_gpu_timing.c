#include "editor/debug/cpu_gpu_timing.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <include/math/math.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

static double get_time_scale_factor(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase_info;
    static double scale = 0.0;
    
    if (scale == 0.0) {
        mach_timebase_info(&timebase_info);
        scale = (double)timebase_info.numer / (double)timebase_info.denom / 1000000.0; // Convert to milliseconds
    }
    
    return scale;
#else
    return 1.0;
#endif
}

static uint64_t get_current_time_ticks(void) {
#ifdef __APPLE__
    return mach_absolute_time();
#else
    // Fallback for other platforms
    return 0;
#endif
}

cpu_gpu_timing_t* cpu_gpu_timing_create(void) {
    cpu_gpu_timing_t* timing = (cpu_gpu_timing_t*)calloc(1, sizeof(cpu_gpu_timing_t));
    if (!timing) return NULL;
    
    // Initialize with 60fps target by default
    cpu_gpu_timing_set_budget(timing, 60.0f);
    
    return timing;
}

void cpu_gpu_timing_destroy(cpu_gpu_timing_t* timing) {
    if (timing) {
        free(timing);
    }
}

void cpu_gpu_timing_set_budget(cpu_gpu_timing_t* timing, float target_fps) {
    if (!timing || target_fps <= 0.0f) return;
    
    timing->budget.target_frame_time_ms = 1000.0f / target_fps;
    timing->budget.cpu_budget_ms = timing->budget.target_frame_time_ms * 0.6f; // 60% for CPU
    timing->budget.gpu_budget_ms = timing->budget.target_frame_time_ms * 0.8f; // 80% for GPU (overlap expected)
    timing->budget.stall_threshold_ms = 2.0f; // 2ms stall threshold
}

void cpu_gpu_timing_set_custom_budget(cpu_gpu_timing_t* timing, const perf_budget_t* budget) {
    if (!timing || !budget) return;
    timing->budget = *budget;
}

void cpu_gpu_timing_begin_cpu_frame(cpu_gpu_timing_t* timing) {
    if (!timing) return;
    timing->cpu_frame_start = get_current_time_ticks();
}

void cpu_gpu_timing_end_cpu_frame(cpu_gpu_timing_t* timing) {
    if (!timing) return;
    
    timing->cpu_frame_end = get_current_time_ticks();
    
    double scale = get_time_scale_factor();
    timing->cpu_frame_time_ms = (float)((timing->cpu_frame_end - timing->cpu_frame_start) * scale);
    
    // Update history
    timing->cpu_frame_times_ms[timing->frame_index] = timing->cpu_frame_time_ms;
    
    // Check CPU budget
    if (timing->cpu_frame_time_ms > timing->budget.cpu_budget_ms) {
        timing->budget_exceeded_cpu = true;
        
        char alert_msg[128];
        snprintf(alert_msg, sizeof(alert_msg), 
                 "CPU frame time %.2fms exceeds budget %.2fms", 
                 timing->cpu_frame_time_ms, 
                 timing->budget.cpu_budget_ms);
        cpu_gpu_timing_add_alert(timing, alert_msg, 0.7f);
    } else {
        timing->budget_exceeded_cpu = false;
    }
    
    // Update statistics
    float cpu_sum = 0.0f;
    float cpu_peak = 0.0f;
    for (int i = 0; i < TIMING_HISTORY_SIZE; i++) {
        cpu_sum += timing->cpu_frame_times_ms[i];
        if (timing->cpu_frame_times_ms[i] > cpu_peak) {
            cpu_peak = timing->cpu_frame_times_ms[i];
        }
    }
    timing->avg_cpu_time_ms = cpu_sum / TIMING_HISTORY_SIZE;
    timing->peak_cpu_time_ms = cpu_peak;
}

void cpu_gpu_timing_update_gpu_time(cpu_gpu_timing_t* timing, float gpu_time_ms) {
    if (!timing) return;
    
    timing->gpu_frame_time_ms = gpu_time_ms;
    timing->gpu_frame_times_ms[timing->frame_index] = gpu_time_ms;
    
    // Check GPU budget
    if (gpu_time_ms > timing->budget.gpu_budget_ms) {
        timing->budget_exceeded_gpu = true;
        
        char alert_msg[128];
        snprintf(alert_msg, sizeof(alert_msg), 
                 "GPU frame time %.2fms exceeds budget %.2fms", 
                 gpu_time_ms, 
                 timing->budget.gpu_budget_ms);
        cpu_gpu_timing_add_alert(timing, alert_msg, 0.8f);
    } else {
        timing->budget_exceeded_gpu = false;
    }
    
    // Update statistics
    float gpu_sum = 0.0f;
    float gpu_peak = 0.0f;
    for (int i = 0; i < TIMING_HISTORY_SIZE; i++) {
        gpu_sum += timing->gpu_frame_times_ms[i];
        if (timing->gpu_frame_times_ms[i] > gpu_peak) {
            gpu_peak = timing->gpu_frame_times_ms[i];
        }
    }
    timing->avg_gpu_time_ms = gpu_sum / TIMING_HISTORY_SIZE;
    timing->peak_gpu_time_ms = gpu_peak;
    
    // Advance frame index
    timing->frame_index = (timing->frame_index + 1) % TIMING_HISTORY_SIZE;
}

void cpu_gpu_timing_detect_stalls(cpu_gpu_timing_t* timing) {
    if (!timing) return;
    
    // Basic stall detection: if CPU and GPU times are both high and similar,
    // there's likely a synchronization stall
    float total_time = timing->cpu_frame_time_ms + timing->gpu_frame_time_ms;
    float expected_time = fmaxf(timing->cpu_frame_time_ms, timing->gpu_frame_time_ms);
    
    timing->cpu_gpu_stall_time_ms = total_time - expected_time;
    timing->stall_times_ms[timing->frame_index] = timing->cpu_gpu_stall_time_ms;
    
    if (timing->cpu_gpu_stall_time_ms > timing->budget.stall_threshold_ms) {
        char alert_msg[128];
        snprintf(alert_msg, sizeof(alert_msg), 
                 "CPU-GPU stall detected: %.2fms", 
                 timing->cpu_gpu_stall_time_ms);
        cpu_gpu_timing_add_alert(timing, alert_msg, 0.9f);
    }
    
    // Update average stall time
    float stall_sum = 0.0f;
    for (int i = 0; i < TIMING_HISTORY_SIZE; i++) {
        stall_sum += timing->stall_times_ms[i];
    }
    timing->avg_stall_time_ms = stall_sum / TIMING_HISTORY_SIZE;
}

void cpu_gpu_timing_add_alert(cpu_gpu_timing_t* timing, const char* message, float severity) {
    if (!timing || !message) return;
    if (timing->alert_count >= TIMING_MAX_ALERTS) return;
    
    perf_alert_t* alert = &timing->alerts[timing->alert_count++];
    strncpy(alert->message, message, sizeof(alert->message) - 1);
    alert->message[sizeof(alert->message) - 1] = '\0';
    alert->timestamp_ms = timing->cpu_frame_time_ms;
    alert->severity = severity;
    alert->active = true;
}

void cpu_gpu_timing_clear_alerts(cpu_gpu_timing_t* timing) {
    if (!timing) return;
    
    for (uint32_t i = 0; i < timing->alert_count; i++) {
        timing->alerts[i].active = false;
    }
    timing->alert_count = 0;
}

uint32_t cpu_gpu_timing_get_active_alerts(cpu_gpu_timing_t* timing, perf_alert_t* out_alerts, uint32_t max_alerts) {
    if (!timing || !out_alerts) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < timing->alert_count && count < max_alerts; i++) {
        if (timing->alerts[i].active) {
            out_alerts[count++] = timing->alerts[i];
        }
    }
    
    return count;
}

void cpu_gpu_timing_get_stats(cpu_gpu_timing_t* timing, float* out_cpu_avg, float* out_gpu_avg, float* out_stall_avg) {
    if (!timing) return;
    
    if (out_cpu_avg) *out_cpu_avg = timing->avg_cpu_time_ms;
    if (out_gpu_avg) *out_gpu_avg = timing->avg_gpu_time_ms;
    if (out_stall_avg) *out_stall_avg = timing->avg_stall_time_ms;
}

bool cpu_gpu_timing_is_budget_exceeded(cpu_gpu_timing_t* timing) {
    if (!timing) return false;
    return timing->budget_exceeded_cpu || timing->budget_exceeded_gpu;
}
