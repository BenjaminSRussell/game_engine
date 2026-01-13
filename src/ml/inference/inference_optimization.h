#ifndef INFERENCE_OPTIMIZATION_H
#define INFERENCE_OPTIMIZATION_H

#include "neural_inference.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float* quantized_weights;
    float* quantized_biases;
    float scale;
    float zero_point;
    bool is_quantized;
} QuantizedNetwork;

typedef struct {
    InferenceEngine* engine;
    QuantizedNetwork* quantized_network;
    uint32_t cache_size;
    float* input_cache;
    float* output_cache;
    uint32_t* cache_indices;
    bool cache_enabled;
} OptimizedInferenceEngine;

typedef struct {
    uint32_t total_inferences;
    uint32_t cache_hits;
    uint32_t cache_misses;
    float average_inference_time;
    float cache_hit_rate;
} InferenceMetrics;

// Optimization techniques
typedef enum {
    OPTIMIZATION_NONE,
    OPTIMIZATION_QUANTIZATION,
    OPTIMIZATION_CACHING,
    OPTIMIZATION_BATCHING,
    OPTIMIZATION_PRUNING,
    OPTIMIZATION_ALL
} OptimizationType;

// Create optimized inference engine
OptimizedInferenceEngine* ml_create_optimized_engine(NeuralNetwork* network, OptimizationType optimizations);

// Quantization functions
bool ml_quantize_network(const NeuralNetwork* network, QuantizedNetwork* quantized);
bool ml_dequantize_network(const QuantizedNetwork* quantized, NeuralNetwork* network);

// Caching functions
bool ml_enable_inference_cache(OptimizedInferenceEngine* engine, uint32_t cache_size);
bool ml_disable_inference_cache(OptimizedInferenceEngine* engine);
bool ml_clear_inference_cache(OptimizedInferenceEngine* engine);

// Optimized inference
bool ml_optimized_infer(OptimizedInferenceEngine* engine, const float* input, float* output);
bool ml_batch_optimized_infer(OptimizedInferenceEngine* engine, const float* inputs, float* outputs, uint32_t batch_size);

// Network pruning
bool ml_prune_network(NeuralNetwork* network, float pruning_threshold);
bool ml_prune_network_structured(NeuralNetwork* network, float sparsity_ratio);

// Performance monitoring
InferenceMetrics ml_get_inference_metrics(const OptimizedInferenceEngine* engine);
void ml_reset_inference_metrics(OptimizedInferenceEngine* engine);

// Memory optimization
bool ml_optimize_memory_layout(NeuralNetwork* network);
uint32_t ml_calculate_memory_usage(const NeuralNetwork* network);

// Threading support
typedef struct {
    OptimizedInferenceEngine* engine;
    uint32_t thread_id;
    uint32_t num_threads;
    const float* inputs;
    float* outputs;
    uint32_t batch_size;
} InferenceThreadData;

bool ml_parallel_inference(OptimizedInferenceEngine* engine, const float* inputs, float* outputs, 
                          uint32_t batch_size, uint32_t num_threads);

// Cleanup
void ml_destroy_optimized_engine(OptimizedInferenceEngine* engine);
void ml_destroy_quantized_network(QuantizedNetwork* network);

#ifdef __cplusplus
}
#endif

#endif // INFERENCE_OPTIMIZATION_H
