#ifndef CPU_GPU_TIMING_H
#define CPU_GPU_TIMING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIMING_HISTORY_SIZE 120
#define TIMING_MAX_ALERTS 32

// Performance budget thresholds
typedef struct perf_budget {
    float target_frame_time_ms;    // Target frame time (e.g., 16.67ms for 60fps)
    float cpu_budget_ms;            // CPU budget
    float gpu_budget_ms;            // GPU budget
    float stall_threshold_ms;       // Threshold for detecting CPU-GPU stalls
} perf_budget_t;

// Performance alert
typedef struct perf_alert {
    char message[128];
    float timestamp_ms;
    float severity; // 0.0 to 1.0
    bool active;
} perf_alert_t;

// CPU-GPU timing state
typedef struct cpu_gpu_timing {
    // CPU timing
    uint64_t cpu_frame_start;
    uint64_t cpu_frame_end;
    float cpu_frame_time_ms;
    float cpu_frame_times_ms[TIMING_HISTORY_SIZE];
    
    // GPU timing
    float gpu_frame_time_ms;
    float gpu_frame_times_ms[TIMING_HISTORY_SIZE];
    
    // Synchronization
    float cpu_gpu_stall_time_ms;
    float stall_times_ms[TIMING_HISTORY_SIZE];
    
    // Budget tracking
    perf_budget_t budget;
    bool budget_exceeded_cpu;
    bool budget_exceeded_gpu;
    
    // Alerts
    perf_alert_t alerts[TIMING_MAX_ALERTS];
    uint32_t alert_count;
    
    // Frame index
    uint32_t frame_index;
    
    // Statistics
    float avg_cpu_time_ms;
    float avg_gpu_time_ms;
    float avg_stall_time_ms;
    float peak_cpu_time_ms;
    float peak_gpu_time_ms;
    
} cpu_gpu_timing_t;

// Lifecycle
cpu_gpu_timing_t* cpu_gpu_timing_create(void);
void cpu_gpu_timing_destroy(cpu_gpu_timing_t* timing);

// Budget configuration
void cpu_gpu_timing_set_budget(cpu_gpu_timing_t* timing, float target_fps);
void cpu_gpu_timing_set_custom_budget(cpu_gpu_timing_t* timing, const perf_budget_t* budget);

// Frame timing
void cpu_gpu_timing_begin_cpu_frame(cpu_gpu_timing_t* timing);
void cpu_gpu_timing_end_cpu_frame(cpu_gpu_timing_t* timing);
void cpu_gpu_timing_update_gpu_time(cpu_gpu_timing_t* timing, float gpu_time_ms);

// Stall detection
void cpu_gpu_timing_detect_stalls(cpu_gpu_timing_t* timing);

// Alert management
void cpu_gpu_timing_add_alert(cpu_gpu_timing_t* timing, const char* message, float severity);
void cpu_gpu_timing_clear_alerts(cpu_gpu_timing_t* timing);
uint32_t cpu_gpu_timing_get_active_alerts(cpu_gpu_timing_t* timing, perf_alert_t* out_alerts, uint32_t max_alerts);

// Statistics
void cpu_gpu_timing_get_stats(cpu_gpu_timing_t* timing, float* out_cpu_avg, float* out_gpu_avg, float* out_stall_avg);
bool cpu_gpu_timing_is_budget_exceeded(cpu_gpu_timing_t* timing);

#ifdef __cplusplus
}
#endif

#endif // CPU_GPU_TIMING_H
