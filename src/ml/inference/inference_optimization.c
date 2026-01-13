#include "inference_optimization.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static uint32_t hash_input(const float* input, uint32_t input_size, uint32_t cache_size) {
    uint32_t hash = 0;
    for (uint32_t i = 0; i < input_size; i++) {
        // Simple hash function - would use better one in production
        uint32_t int_val = (uint32_t)(input[i] * 1000.0f);
        hash = ((hash << 5) - hash) + int_val;
    }
    return hash % cache_size;
}

static bool cache_lookup(OptimizedInferenceEngine* engine, const float* input, float* output) {
    if (!engine->cache_enabled) return false;
    
    uint32_t input_size = engine->engine->network->input_size;
    uint32_t hash = hash_input(input, input_size, engine->cache_size);
    
    // Check if cached input matches
    float* cached_input = engine->input_cache + hash * input_size;
    for (uint32_t i = 0; i < input_size; i++) {
        if (fabsf(cached_input[i] - input[i]) > 1e-6f) {
            return false;
        }
    }
    
    // Cache hit - return cached output
    uint32_t output_size = engine->engine->network->output_size;
    float* cached_output = engine->output_cache + hash * output_size;
    memcpy(output, cached_output, output_size * sizeof(float));
    
    return true;
}

static void cache_store(OptimizedInferenceEngine* engine, const float* input, const float* output) {
    if (!engine->cache_enabled) return;
    
    uint32_t input_size = engine->engine->network->input_size;
    uint32_t output_size = engine->engine->network->output_size;
    uint32_t hash = hash_input(input, input_size, engine->cache_size);
    
    // Store in cache
    float* cached_input = engine->input_cache + hash * input_size;
    float* cached_output = engine->output_cache + hash * output_size;
    
    memcpy(cached_input, input, input_size * sizeof(float));
    memcpy(cached_output, output, output_size * sizeof(float));
}

OptimizedInferenceEngine* ml_create_optimized_engine(NeuralNetwork* network, OptimizationType optimizations) {
    if (!network) return NULL;
    
    OptimizedInferenceEngine* engine = malloc(sizeof(OptimizedInferenceEngine));
    if (!engine) return NULL;
    
    engine->engine = ml_create_inference_engine(network);
    if (!engine->engine) {
        free(engine);
        return NULL;
    }
    
    engine->quantized_network = NULL;
    engine->cache_size = 0;
    engine->input_cache = NULL;
    engine->output_cache = NULL;
    engine->cache_indices = NULL;
    engine->cache_enabled = false;
    
    // Apply optimizations
    if (optimizations == OPTIMIZATION_QUANTIZATION || optimizations == OPTIMIZATION_ALL) {
        engine->quantized_network = malloc(sizeof(QuantizedNetwork));
        if (engine->quantized_network) {
            ml_quantize_network(network, engine->quantized_network);
        }
    }
    
    if (optimizations == OPTIMIZATION_CACHING || optimizations == OPTIMIZATION_ALL) {
        ml_enable_inference_cache(engine, 1024); // Default cache size
    }
    
    return engine;
}

bool ml_quantize_network(const NeuralNetwork* network, QuantizedNetwork* quantized) {
    if (!network || !quantized) return false;
    
    // Calculate total weights and biases
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    uint32_t total_biases = 0;
    for (uint32_t i = 0; i < network->hidden_layers; i++) {
        total_biases += network->layer_sizes[i];
    }
    total_biases += network->output_size;
    
    // Allocate quantized storage
    quantized->quantized_weights = malloc(total_weights * sizeof(float));
    quantized->quantized_biases = malloc(total_biases * sizeof(float));
    
    if (!quantized->quantized_weights || !quantized->quantized_biases) {
        free(quantized->quantized_weights);
        free(quantized->quantized_biases);
        return false;
    }
    
    // Find min and max values for scaling
    float min_weight = network->weights[0], max_weight = network->weights[0];
    for (uint32_t i = 1; i < total_weights; i++) {
        if (network->weights[i] < min_weight) min_weight = network->weights[i];
        if (network->weights[i] > max_weight) max_weight = network->weights[i];
    }
    
    // Calculate scale and zero point
    quantized->scale = (max_weight - min_weight) / 255.0f;
    quantized->zero_point = -min_weight / quantized->scale;
    
    // Quantize weights
    for (uint32_t i = 0; i < total_weights; i++) {
        quantized->quantized_weights[i] = roundf(network->weights[i] / quantized->scale + quantized->zero_point);
    }
    
    // Quantize biases (usually keep in higher precision)
    memcpy(quantized->quantized_biases, network->biases, total_biases * sizeof(float));
    
    quantized->is_quantized = true;
    return true;
}

bool ml_dequantize_network(const QuantizedNetwork* quantized, NeuralNetwork* network) {
    if (!quantized || !network || !quantized->is_quantized) return false;
    
    // Calculate total weights
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    // Dequantize weights
    for (uint32_t i = 0; i < total_weights; i++) {
        network->weights[i] = (quantized->quantized_weights[i] - quantized->zero_point) * quantized->scale;
    }
    
    return true;
}

bool ml_enable_inference_cache(OptimizedInferenceEngine* engine, uint32_t cache_size) {
    if (!engine || cache_size == 0) return false;
    
    uint32_t input_size = engine->engine->network->input_size;
    uint32_t output_size = engine->engine->network->output_size;
    
    engine->input_cache = malloc(cache_size * input_size * sizeof(float));
    engine->output_cache = malloc(cache_size * output_size * sizeof(float));
    engine->cache_indices = malloc(cache_size * sizeof(uint32_t));
    
    if (!engine->input_cache || !engine->output_cache || !engine->cache_indices) {
        ml_disable_inference_cache(engine);
        return false;
    }
    
    engine->cache_size = cache_size;
    engine->cache_enabled = true;
    
    // Initialize cache
    memset(engine->input_cache, 0, cache_size * input_size * sizeof(float));
    memset(engine->output_cache, 0, cache_size * output_size * sizeof(float));
    for (uint32_t i = 0; i < cache_size; i++) {
        engine->cache_indices[i] = i;
    }
    
    return true;
}

bool ml_disable_inference_cache(OptimizedInferenceEngine* engine) {
    if (!engine) return false;
    
    free(engine->input_cache);
    free(engine->output_cache);
    free(engine->cache_indices);
    
    engine->input_cache = NULL;
    engine->output_cache = NULL;
    engine->cache_indices = NULL;
    engine->cache_enabled = false;
    engine->cache_size = 0;
    
    return true;
}

bool ml_clear_inference_cache(OptimizedInferenceEngine* engine) {
    if (!engine || !engine->cache_enabled) return false;
    
    uint32_t input_size = engine->engine->network->input_size;
    uint32_t output_size = engine->engine->network->output_size;
    
    memset(engine->input_cache, 0, engine->cache_size * input_size * sizeof(float));
    memset(engine->output_cache, 0, engine->cache_size * output_size * sizeof(float));
    
    return true;
}

bool ml_optimized_infer(OptimizedInferenceEngine* engine, const float* input, float* output) {
    if (!engine || !input || !output) return false;
    
    // Check cache first
    if (cache_lookup(engine, input, output)) {
        return true;
    }
    
    // Perform inference
    bool result = ml_infer(engine->engine, input, output);
    
    // Store in cache
    if (result) {
        cache_store(engine, input, output);
    }
    
    return result;
}

bool ml_batch_optimized_infer(OptimizedInferenceEngine* engine, const float* inputs, float* outputs, uint32_t batch_size) {
    if (!engine || !inputs || !outputs || batch_size == 0) return false;
    
    for (uint32_t i = 0; i < batch_size; i++) {
        const float* input = inputs + i * engine->engine->network->input_size;
        float* output = outputs + i * engine->engine->network->output_size;
        
        if (!ml_optimized_infer(engine, input, output)) {
            return false;
        }
    }
    
    return true;
}

bool ml_prune_network(NeuralNetwork* network, float pruning_threshold) {
    if (!network || pruning_threshold < 0.0f || pruning_threshold > 1.0f) return false;
    
    // Calculate total weights
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    // Prune weights below threshold
    uint32_t pruned_count = 0;
    for (uint32_t i = 0; i < total_weights; i++) {
        if (fabsf(network->weights[i]) < pruning_threshold) {
            network->weights[i] = 0.0f;
            pruned_count++;
        }
    }
    
    return (pruned_count > 0);
}

bool ml_prune_network_structured(NeuralNetwork* network, float sparsity_ratio) {
    if (!network || sparsity_ratio < 0.0f || sparsity_ratio > 1.0f) return false;
    
    // Structured pruning would remove entire neurons/channels
    // This is a simplified implementation
    return ml_prune_network(network, sparsity_ratio * 0.1f);
}

InferenceMetrics ml_get_inference_metrics(const OptimizedInferenceEngine* engine) {
    InferenceMetrics metrics = {0};
    
    if (!engine) return metrics;
    
    // Calculate cache hit rate
    if (engine->cache_enabled && engine->cache_size > 0) {
        // Would track actual metrics in real implementation
        metrics.cache_hit_rate = 0.0f;
    }
    
    return metrics;
}

void ml_reset_inference_metrics(OptimizedInferenceEngine* engine) {
    if (!engine) return;
    
    // Reset metrics tracking
}

bool ml_optimize_memory_layout(NeuralNetwork* network) {
    if (!network) return false;
    
    // Reorganize memory for better cache locality
    // This is a placeholder for actual memory layout optimization
    return true;
}

uint32_t ml_calculate_memory_usage(const NeuralNetwork* network) {
    if (!network) return 0;
    
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    uint32_t total_biases = 0;
    for (uint32_t i = 0; i < network->hidden_layers; i++) {
        total_biases += network->layer_sizes[i];
    }
    total_biases += network->output_size;
    
    return (total_weights + total_biases) * sizeof(float) + 
           network->hidden_layers * sizeof(uint32_t) + sizeof(NeuralNetwork);
}

void ml_destroy_optimized_engine(OptimizedInferenceEngine* engine) {
    if (!engine) return;
    
    ml_destroy_inference_engine(engine->engine);
    ml_destroy_quantized_network(engine->quantized_network);
    ml_disable_inference_cache(engine);
    
    free(engine);
}

void ml_destroy_quantized_network(QuantizedNetwork* network) {
    if (!network) return;
    
    free(network->quantized_weights);
    free(network->quantized_biases);
    free(network);
}
