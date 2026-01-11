#ifndef PERFORMANCE_ANALYZER_H
#define PERFORMANCE_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PERF_ANALYSIS_MAX_HINTS 16

// Bottleneck types
typedef enum bottleneck_type {
    BOTTLENECK_NONE = 0,
    BOTTLENECK_CPU_BOUND,
    BOTTLENECK_GPU_BOUND,
    BOTTLENECK_MEMORY_BANDWIDTH,
    BOTTLENECK_ALU,
    BOTTLENECK_TEXTURE_SAMPLING,
    BOTTLENECK_VERTEX_PROCESSING,
    BOTTLENECK_FRAGMENT_PROCESSING,
    BOTTLENECK_SYNCHRONIZATION
} bottleneck_type_t;

// Performance metrics
typedef struct perf_metrics {
    // GPU utilization
    float gpu_utilization_percent;
    float alu_utilization_percent;
    float texture_utilization_percent;
    
    // Bandwidth
    uint64_t memory_bandwidth_bytes_per_sec;
    float bandwidth_utilization_percent;
    
    // Cache efficiency
    float texture_cache_hit_rate;
    float l1_cache_hit_rate;
    float l2_cache_hit_rate;
    
    // Draw call stats
    uint32_t draw_calls_per_frame;
    uint32_t triangles_per_frame;
    uint32_t vertices_per_frame;
    
} perf_metrics_t;

// Optimization hint
typedef struct optimization_hint {
    char description[256];
    float impact_score; // 0.0 to 1.0, higher = more impactful
    bool actionable;
} optimization_hint_t;

// Performance analyzer state
typedef struct performance_analyzer {
    perf_metrics_t current_metrics;
    perf_metrics_t average_metrics;
    
    bottleneck_type_t detected_bottleneck;
    float bottleneck_confidence; // 0.0 to 1.0
    
    optimization_hint_t hints[PERF_ANALYSIS_MAX_HINTS];
    uint32_t hint_count;
    
    // Analysis history
    uint32_t analysis_frame_count;
    
} performance_analyzer_t;

// Lifecycle
performance_analyzer_t* performance_analyzer_create(void);
void performance_analyzer_destroy(performance_analyzer_t* analyzer);

// Analysis
void performance_analyzer_update(performance_analyzer_t* analyzer, const perf_metrics_t* metrics);
void performance_analyzer_run_analysis(performance_analyzer_t* analyzer);

// Bottleneck identification
bottleneck_type_t performance_analyzer_get_bottleneck(performance_analyzer_t* analyzer, float* out_confidence);
const char* performance_analyzer_get_bottleneck_name(bottleneck_type_t type);

// Bandwidth analysis
void performance_analyzer_analyze_bandwidth(performance_analyzer_t* analyzer);
float performance_analyzer_get_bandwidth_pressure(performance_analyzer_t* analyzer);

// ALU utilization
void performance_analyzer_analyze_alu(performance_analyzer_t* analyzer);
bool performance_analyzer_is_alu_bound(performance_analyzer_t* analyzer);

// Cache efficiency
void performance_analyzer_analyze_cache(performance_analyzer_t* analyzer);
float performance_analyzer_get_cache_efficiency(performance_analyzer_t* analyzer);

// Optimization hints
uint32_t performance_analyzer_get_hints(performance_analyzer_t* analyzer, optimization_hint_t* out_hints, uint32_t max_hints);
void performance_analyzer_clear_hints(performance_analyzer_t* analyzer);

// Advanced analysis
void performance_analyzer_analyze_gpu_utilization(performance_analyzer_t* analyzer);
void performance_analyzer_analyze_cache_efficiency(performance_analyzer_t* analyzer);
void performance_analyzer_analyze_draw_calls(performance_analyzer_t* analyzer);
void performance_analyzer_analyze_bandwidth(performance_analyzer_t* analyzer);
void performance_analyzer_analyze_frame_time_stability(performance_analyzer_t* analyzer);
void performance_analyzer_analyze_memory_pressure(performance_analyzer_t* analyzer);
void performance_analyzer_identify_bottleneck(performance_analyzer_t* analyzer);
void performance_analyzer_calculate_advanced_metrics(performance_analyzer_t* analyzer);

// Reports and diagnostics
void performance_analyzer_get_performance_report(performance_analyzer_t* analyzer, char* buffer, u32 buffer_size);
float performance_analyzer_get_average_frame_time(performance_analyzer_t* analyzer);
uint32_t performance_analyzer_get_frame_spike_count(performance_analyzer_t* analyzer);
float performance_analyzer_get_render_pipeline_efficiency(performance_analyzer_t* analyzer);

#ifdef __cplusplus
}
#endif

#endif // PERFORMANCE_ANALYZER_H
