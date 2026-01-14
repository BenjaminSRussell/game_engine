#ifndef GPU_ACCELERATION_H
#define GPU_ACCELERATION_H

#include "neural_inference.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* device_context;
    void* command_queue;
    void* memory_buffer;
    bool is_initialized;
    char device_name[256];
    uint32_t compute_units;
    uint64_t memory_size;
} GPUContext;

typedef struct {
    GPUContext* context;
    void* weight_buffer;
    void* bias_buffer;
    void* input_buffer;
    void* output_buffer;
    void* intermediate_buffer;
    uint32_t buffer_size;
    bool buffers_allocated;
} GPUNetworkBuffers;

typedef struct {
    GPUContext* gpu_context;
    NeuralNetwork* network;
    GPUNetworkBuffers* buffers;
    void* compute_shader;
    bool shader_compiled;
} GPUInferenceEngine;

typedef struct {
    uint32_t kernel_launches;
    uint32_t memory_transfers;
    float gpu_time_ms;
    float cpu_to_gpu_time_ms;
    float gpu_to_cpu_time_ms;
    float compute_time_ms;
} GPUPerformanceMetrics;

// GPU initialization and management
GPUContext* ml_init_gpu_context();
bool ml_is_gpu_available();
bool ml_select_gpu_device(uint32_t device_id);
void ml_destroy_gpu_context(GPUContext* context);

// Network buffer management
GPUNetworkBuffers* ml_create_gpu_buffers(GPUContext* context, const NeuralNetwork* network);
bool ml_upload_network_to_gpu(GPUNetworkBuffers* buffers, const NeuralNetwork* network);
bool ml_download_network_from_gpu(GPUNetworkBuffers* buffers, NeuralNetwork* network);
void ml_destroy_gpu_buffers(GPUNetworkBuffers* buffers);

// GPU inference engine
GPUInferenceEngine* ml_create_gpu_inference_engine(NeuralNetwork* network);
bool ml_gpu_infer(GPUInferenceEngine* engine, const float* input, float* output);
bool ml_gpu_batch_infer(GPUInferenceEngine* engine, const float* inputs, float* outputs, uint32_t batch_size);

// Shader compilation and management
bool ml_compile_compute_shader(GPUInferenceEngine* engine, const char* shader_source);
bool ml_load_precompiled_shader(GPUInferenceEngine* engine, const char* shader_path);
void ml_destroy_compute_shader(void* shader);

// Memory management
bool ml_allocate_gpu_memory(GPUContext* context, void** buffer, uint64_t size);
bool ml_free_gpu_memory(GPUContext* context, void* buffer);
bool ml_copy_to_gpu(GPUContext* context, const void* host_data, void* gpu_data, uint64_t size);
bool ml_copy_from_gpu(GPUContext* context, const void* gpu_data, void* host_data, uint64_t size);

// Performance monitoring
GPUPerformanceMetrics ml_get_gpu_performance_metrics(const GPUInferenceEngine* engine);
void ml_reset_gpu_performance_metrics(GPUInferenceEngine* engine);
bool ml_enable_gpu_profiling(GPUInferenceEngine* engine);
bool ml_disable_gpu_profiling(GPUInferenceEngine* engine);

// Multi-GPU support
typedef struct {
    GPUContext* contexts;
    uint32_t device_count;
    uint32_t active_device;
} MultiGPUContext;

MultiGPUContext* ml_init_multi_gpu_context();
bool ml_set_active_gpu_device(MultiGPUContext* multi_context, uint32_t device_id);
bool ml_distribute_inference_across_gpus(MultiGPUContext* multi_context, const float* inputs, 
                                        float* outputs, uint32_t batch_size);
void ml_destroy_multi_gpu_context(MultiGPUContext* multi_context);

// GPU-specific optimizations
bool ml_enable_tensor_cores(GPUInferenceEngine* engine);
bool ml_optimize_for_gpu_architecture(GPUInferenceEngine* engine, const char* architecture);
bool ml_use_mixed_precision(GPUInferenceEngine* engine);

// Error handling
typedef enum {
    GPU_SUCCESS,
    GPU_ERROR_OUT_OF_MEMORY,
    GPU_ERROR_DEVICE_LOST,
    GPU_ERROR_COMPILATION_FAILED,
    GPU_ERROR_INVALID_OPERATION,
    GPU_ERROR_CONTEXT_LOST
} GPUResult;

const char* ml_get_gpu_error_string(GPUResult result);

// Cleanup
void ml_destroy_gpu_inference_engine(GPUInferenceEngine* engine);

#ifdef __cplusplus
}
#endif

#endif // GPU_ACCELERATION_H
