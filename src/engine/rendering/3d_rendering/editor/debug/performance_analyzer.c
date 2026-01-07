#include "performance_analyzer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

performance_analyzer_t* performance_analyzer_create(void) {
    performance_analyzer_t* analyzer = (performance_analyzer_t*)calloc(1, sizeof(performance_analyzer_t));
    return analyzer;
}

void performance_analyzer_destroy(performance_analyzer_t* analyzer) {
    if (analyzer) {
        free(analyzer);
    }
}

void performance_analyzer_update(performance_analyzer_t* analyzer, const perf_metrics_t* metrics) {
    if (!analyzer || !metrics) return;
    
    analyzer->current_metrics = *metrics;
    
    // Update running averages
    float alpha = 0.1f; // Exponential moving average factor
    
    #define UPDATE_AVG(field) \
        analyzer->average_metrics.field = \
            analyzer->average_metrics.field * (1.0f - alpha) + \
            metrics->field * alpha
    
    UPDATE_AVG(gpu_utilization_percent);
    UPDATE_AVG(alu_utilization_percent);
    UPDATE_AVG(texture_utilization_percent);
    UPDATE_AVG(bandwidth_utilization_percent);
    UPDATE_AVG(texture_cache_hit_rate);
    UPDATE_AVG(l1_cache_hit_rate);
    UPDATE_AVG(l2_cache_hit_rate);
    
    #undef UPDATE_AVG
    
    analyzer->analysis_frame_count++;
}

static void add_hint(performance_analyzer_t* analyzer, const char* description, float impact_score) {
    if (!analyzer || analyzer->hint_count >= PERF_ANALYSIS_MAX_HINTS) return;
    
    optimization_hint_t* hint = &analyzer->hints[analyzer->hint_count++];
    strncpy(hint->description, description, sizeof(hint->description) - 1);
    hint->description[sizeof(hint->description) - 1] = '\0';
    hint->impact_score = impact_score;
    hint->actionable = true;
}

void performance_analyzer_run_analysis(performance_analyzer_t* analyzer) {
    if (!analyzer) return;
    
    performance_analyzer_clear_hints(analyzer);
    
    const perf_metrics_t* m = &analyzer->average_metrics;
    
    // Bottleneck detection
    float max_score = 0.0f;
    bottleneck_type_t detected = BOTTLENECK_NONE;
    
    // CPU-bound check
    if (m->gpu_utilization_percent < 70.0f) {
        float score = 100.0f - m->gpu_utilization_percent;
        if (score > max_score) {
            max_score = score;
            detected = BOTTLENECK_CPU_BOUND;
        }
    }
    
    // Memory bandwidth check
    if (m->bandwidth_utilization_percent > 85.0f) {
        float score = m->bandwidth_utilization_percent;
        if (score > max_score) {
            max_score = score;
            detected = BOTTLENECK_MEMORY_BANDWIDTH;
        }
    }
    
    // ALU bound check
    if (m->alu_utilization_percent > 90.0f) {
        float score = m->alu_utilization_percent;
        if (score > max_score) {
            max_score = score;
            detected = BOTTLENECK_ALU;
        }
    }
    
    analyzer->detected_bottleneck = detected;
    analyzer->bottleneck_confidence = max_score / 100.0f;
    
    // Generate optimization hints based on detected bottleneck
    switch (detected) {
        case BOTTLENECK_CPU_BOUND:
            add_hint(analyzer, "CPU-bound: Reduce draw calls through instancing or batching", 0.9f);
            add_hint(analyzer, "Consider using GPU-driven rendering for culling", 0.8f);
            break;
            
        case BOTTLENECK_MEMORY_BANDWIDTH:
            add_hint(analyzer, "High memory bandwidth usage: Reduce texture resolution or enable compression", 0.9f);
            add_hint(analyzer, "Consider using texture atlases to reduce memory access patterns", 0.7f);
            break;
            
        case BOTTLENECK_ALU:
            add_hint(analyzer, "ALU-bound: Simplify shader computations", 0.9f);
            add_hint(analyzer, "Consider precomputing complex calculations into lookup tables", 0.8f);
            break;
            
        default:
            break;
    }
    
    // Cache efficiency hints
    if (m->texture_cache_hit_rate < 0.85f) {
        add_hint(analyzer, "Low texture cache hit rate: Improve texture locality or reduce texture variety", 0.7f);
    }
    
    // Draw call hints
    if (analyzer->current_metrics.draw_calls_per_frame > 1000) {
        add_hint(analyzer, "High draw call count: Consider mesh merging or GPU instancing", 0.8f);
    }
}

bottleneck_type_t performance_analyzer_get_bottleneck(performance_analyzer_t* analyzer, float* out_confidence) {
    if (!analyzer) return BOTTLENECK_NONE;
    
    if (out_confidence) {
        *out_confidence = analyzer->bottleneck_confidence;
    }
    
    return analyzer->detected_bottleneck;
}

const char* performance_analyzer_get_bottleneck_name(bottleneck_type_t type) {
    switch (type) {
        case BOTTLENECK_NONE: return "None";
        case BOTTLENECK_CPU_BOUND: return "CPU Bound";
        case BOTTLENECK_GPU_BOUND: return "GPU Bound";
        case BOTTLENECK_MEMORY_BANDWIDTH: return "Memory Bandwidth";
        case BOTTLENECK_ALU: return "ALU";
        case BOTTLENECK_TEXTURE_SAMPLING: return "Texture Sampling";
        case BOTTLENECK_VERTEX_PROCESSING: return "Vertex Processing";
        case BOTTLENECK_FRAGMENT_PROCESSING: return "Fragment Processing";
        case BOTTLENECK_SYNCHRONIZATION: return "Synchronization";
        default: return "Unknown";
    }
}

void performance_analyzer_analyze_bandwidth(performance_analyzer_t* analyzer) {
    if (!analyzer) return;
    
    // Bandwidth analysis is already done in run_analysis
    // This is a specialized entry point for deeper analysis
}

float performance_analyzer_get_bandwidth_pressure(performance_analyzer_t* analyzer) {
    if (!analyzer) return 0.0f;
    return analyzer->average_metrics.bandwidth_utilization_percent / 100.0f;
}

void performance_analyzer_analyze_alu(performance_analyzer_t* analyzer) {
    if (!analyzer) return;
    
    // ALU analysis is already done in run_analysis
}

bool performance_analyzer_is_alu_bound(performance_analyzer_t* analyzer) {
    if (!analyzer) return false;
    return analyzer->detected_bottleneck == BOTTLENECK_ALU;
}

void performance_analyzer_analyze_cache(performance_analyzer_t* analyzer) {
    if (!analyzer) return;
    
    // Cache analysis is already done in run_analysis
}

float performance_analyzer_get_cache_efficiency(performance_analyzer_t* analyzer) {
    if (!analyzer) return 0.0f;
    
    // Weighted average of cache hit rates
    float cache_efficiency = 
        analyzer->average_metrics.texture_cache_hit_rate * 0.5f +
        analyzer->average_metrics.l1_cache_hit_rate * 0.3f +
        analyzer->average_metrics.l2_cache_hit_rate * 0.2f;
    
    return cache_efficiency;
}

uint32_t performance_analyzer_get_hints(performance_analyzer_t* analyzer, optimization_hint_t* out_hints, uint32_t max_hints) {
    if (!analyzer || !out_hints) return 0;
    
    uint32_t count = analyzer->hint_count < max_hints ? analyzer->hint_count : max_hints;
    
    for (uint32_t i = 0; i < count; i++) {
        out_hints[i] = analyzer->hints[i];
    }
    
    return count;
}

void performance_analyzer_clear_hints(performance_analyzer_t* analyzer) {
    if (!analyzer) return;
    analyzer->hint_count = 0;
}
