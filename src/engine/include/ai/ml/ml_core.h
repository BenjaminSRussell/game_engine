// include/engine/ml/ml_core.h
//
// Purpose: Core ML system integration with CoreML (macOS) and ONNX Runtime (cross-platform)
// This system provides high-performance machine learning inference for gaming applications
//
// Key Features:
// - CoreML integration for macOS (Neural Engine acceleration)
// - ONNX Runtime support for cross-platform compatibility
// - GPU inference pipeline with Metal/Vulkan ML
// - Tensor buffer management for efficient data flow
// - Model loading and caching system
// - Performance monitoring and optimization
//
// Performance Targets:
// - <2ms inference time for NPC behavior models
// - <5ms inference time for upscaling models
// - <1ms inference time for animation models
// - Memory usage <100MB for loaded models
// - GPU utilization >80% for batch inference
//
// Ownership: MLSystem owns all ML components and models
// Invariants: ML models must be loaded before inference, tensor buffers must be valid
//
#ifndef ML_CORE_H
#define ML_CORE_H

#include "include/common.h"
#include "include/core/performance.h"
#include "include/core/logger.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__APPLE__) && defined(__OBJC__)
#include <CoreML/CoreML.h>
#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>
#endif

// ============================================================================
// ML FRAMEWORK ENUMERATIONS
// ============================================================================

typedef enum {
    ML_FRAMEWORK_COREML = 0,        // macOS CoreML (preferred on Apple platforms)
    ML_FRAMEWORK_ONNX,              // ONNX Runtime (cross-platform)
    ML_FRAMEWORK_TENSORFLOW_LITE,    // TensorFlow Lite (mobile platforms)
    ML_FRAMEWORK_PYTORCH,           // PyTorch (development/testing)
    ML_FRAMEWORK_CUSTOM             // Custom implementation
} MLFramework;

typedef enum {
    ML_DEVICE_CPU = 0,
    ML_DEVICE_GPU,
    ML_DEVICE_NEURAL_ENGINE,        // Apple Neural Engine
    ML_DEVICE_AUTO                  // Automatically select best device
} MLDevice;

typedef enum {
    ML_DATA_TYPE_FLOAT32 = 0,
    ML_DATA_TYPE_FLOAT16,
    ML_DATA_TYPE_INT32,
    ML_DATA_TYPE_INT8,
    ML_DATA_TYPE_UINT8
} MLDataType;

typedef enum {
    ML_MODEL_TYPE_BEHAVIOR = 0,     // NPC behavior prediction
    ML_MODEL_TYPE_UPSCALING,        // Image super-resolution
    ML_MODEL_TYPE_ANIMATION,        // Animation synthesis
    ML_MODEL_TYPE_NAVIGATION,       // Pathfinding prediction
    ML_MODEL_TYPE_GENERATION,       // Procedural content generation
    ML_MODEL_TYPE_CUSTOM            // Custom model type
} MLModelType;

// ============================================================================
// TENSOR STRUCTURES
// ============================================================================

typedef struct {
    u32 *dimensions;
    u32 dimension_count;
    u64 total_elements;
    MLDataType data_type;
    size_t byte_size;
    void *data;
    char *name;
} MLTensor;

typedef struct {
    MLTensor *inputs;
    MLTensor *outputs;
    u32 input_count;
    u32 output_count;
    void *native_context;  // Framework-specific context
} MLInferenceContext;

// ============================================================================
// MODEL METADATA
// ============================================================================

typedef struct {
    char *name;
    char *version;
    char *description;
    MLModelType type;
    MLFramework framework;
    MLDevice preferred_device;
    u64 model_size_bytes;
    f32 expected_inference_time_ms;
    u32 max_batch_size;
    bool supports_dynamic_shapes;
    char *author;
    u64 creation_timestamp;
} MLModelMetadata;

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

typedef struct {
    u64 total_inferences;
    f64 total_inference_time;
    f64 average_inference_time;
    f64 min_inference_time;
    f64 max_inference_time;
    f32 gpu_utilization;
    f32 memory_usage_mb;
    u64 cache_hits;
    u64 cache_misses;
    f32 cache_hit_rate;
} MLPerformanceStats;

// ============================================================================
// CORE ML SYSTEM
// ============================================================================

typedef struct {
    // System configuration
    MLFramework default_framework;
    MLDevice default_device;
    u32 max_loaded_models;
    u32 max_concurrent_inferences;
    bool enable_gpu_acceleration;
    bool enable_performance_monitoring;
    bool enable_model_caching;
    
    // Framework-specific contexts
#ifdef __APPLE__
    void *coreml_context;
    void *metal_device;
    void *mps_command_queue;
#endif
    void *onnx_runtime;
    
    // Model management
    void **loaded_models;
    MLModelMetadata *model_metadata;
    u32 loaded_model_count;
    
    // Inference management
    MLInferenceContext *inference_contexts;
    u32 active_inference_count;
    
    // Performance monitoring
    MLPerformanceStats *performance_stats;
    Profiler *ml_profiler;
    
    // Memory management
    void *tensor_pool;
    size_t total_tensor_memory;
    size_t used_tensor_memory;
    
    bool initialized;
} MLSystem;

// ============================================================================
// PUBLIC API - SYSTEM MANAGEMENT
// ============================================================================

// System initialization and shutdown
MLSystem *ml_system_create(const MLFramework framework, const MLDevice device);
void ml_system_destroy(MLSystem *system);
bool ml_system_initialize(MLSystem *system);
void ml_system_shutdown(MLSystem *system);

// Configuration
void ml_system_set_default_framework(MLSystem *system, MLFramework framework);
void ml_system_set_default_device(MLSystem *system, MLDevice device);
void ml_system_enable_gpu_acceleration(MLSystem *system, bool enable);
void ml_system_enable_performance_monitoring(MLSystem *system, bool enable);

// ============================================================================
// PUBLIC API - MODEL MANAGEMENT
// ============================================================================

// Model loading and unloading
void *ml_load_model(MLSystem *system, const char *model_path, const MLModelMetadata *metadata);
bool ml_unload_model(MLSystem *system, void *model);
void *ml_get_model(MLSystem *system, const char *model_name);

// Model information
MLModelMetadata *ml_get_model_metadata(MLSystem *system, void *model);
bool ml_validate_model(MLSystem *system, void *model);
u64 ml_get_model_memory_usage(MLSystem *system, void *model);

// ============================================================================
// PUBLIC API - TENSOR OPERATIONS
// ============================================================================

// Tensor creation and management
MLTensor *ml_create_tensor(MLSystem *system, const u32 *dimensions, u32 dimension_count, MLDataType data_type);
void ml_destroy_tensor(MLSystem *system, MLTensor *tensor);
bool ml_resize_tensor(MLSystem *system, MLTensor *tensor, const u32 *new_dimensions, u32 new_dimension_count);

// Tensor data operations
bool ml_copy_tensor_data(MLTensor *dst, const MLTensor *src);
bool ml_set_tensor_data(MLTensor *tensor, const void *data, size_t data_size);
void *ml_get_tensor_data(const MLTensor *tensor);
size_t ml_get_tensor_size(const MLTensor *tensor);

// ============================================================================
// PUBLIC API - INFERENCE
// ============================================================================

// Inference execution
MLInferenceContext *ml_create_inference_context(MLSystem *system, void *model);
void ml_destroy_inference_context(MLSystem *system, MLInferenceContext *context);
bool ml_run_inference(MLSystem *system, MLInferenceContext *context);
bool ml_run_inference_async(MLSystem *system, MLInferenceContext *context, void (*callback)(MLInferenceContext *, void *), void *user_data);

// Batch inference
bool ml_run_batch_inference(MLSystem *system, void *model, const MLTensor *input_batch, MLTensor *output_batch, u32 batch_size);

// ============================================================================
// PUBLIC API - PERFORMANCE MONITORING
// ============================================================================

// Performance statistics
MLPerformanceStats *ml_get_performance_stats(MLSystem *system);
void ml_reset_performance_stats(MLSystem *system);
void ml_print_performance_report(MLSystem *system);

// Profiling
void ml_start_profiling(MLSystem *system);
void ml_stop_profiling(MLSystem *system);
void ml_dump_profiling_data(MLSystem *system, const char *output_path);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Device capabilities
bool ml_is_device_available(MLSystem *system, MLDevice device);
const char *ml_get_device_name(MLDevice device);
u32 ml_get_device_memory_size(MLSystem *system, MLDevice device);

// Framework information
const char *ml_get_framework_name(MLFramework framework);
bool ml_is_framework_available(MLFramework framework);

// Error handling
typedef enum {
    ML_ERROR_NONE = 0,
    ML_ERROR_INVALID_PARAMETER,
    ML_ERROR_MODEL_NOT_FOUND,
    ML_ERROR_MODEL_LOAD_FAILED,
    ML_ERROR_INFERENCE_FAILED,
    ML_ERROR_OUT_OF_MEMORY,
    ML_ERROR_DEVICE_UNAVAILABLE,
    ML_ERROR_TENSOR_MISMATCH,
    ML_ERROR_FRAMEWORK_ERROR
} MLError;

const char *ml_get_error_string(MLError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Tensor creation macros
#define ML_CREATE_TENSOR_1D(system, dim0, type) \
    ml_create_tensor(system, (u32[]){dim0}, 1, type)

#define ML_CREATE_TENSOR_2D(system, dim0, dim1, type) \
    ml_create_tensor(system, (u32[]){dim0, dim1}, 2, type)

#define ML_CREATE_TENSOR_3D(system, dim0, dim1, dim2, type) \
    ml_create_tensor(system, (u32[]){dim0, dim1, dim2}, 3, type)

#define ML_CREATE_TENSOR_4D(system, dim0, dim1, dim2, dim3, type) \
    ml_create_tensor(system, (u32[]){dim0, dim1, dim2, dim3}, 4, type)

// Error checking macros
#define ML_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR(LOG_CAT_AI, "ML Error: %s", ml_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define ML_CHECK_NULL_PARAM_PTR(param) \
    do { \
        if ((param) == NULL) { \
            LOG_ERROR(LOG_CAT_AI, "ML Error: %s", ml_get_error_string(ML_ERROR_INVALID_PARAMETER)); \
            return NULL; \
        } \
    } while(0)

#define ML_CHECK_NULL_PARAM_BOOL(param) \
    do { \
        if ((param) == NULL) { \
            LOG_ERROR(LOG_CAT_AI, "ML Error: %s", ml_get_error_string(ML_ERROR_INVALID_PARAMETER)); \
            return false; \
        } \
    } while(0)

#define ML_CHECK_ERROR_PTR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR(LOG_CAT_AI, "ML Error: %s", ml_get_error_string(error)); \
            return NULL; \
        } \
    } while(0)

// Performance macros
#define ML_START_TIMER(system) \
    clock_t start_time = clock()

#define ML_END_TIMER(system, stats_ptr) \
    do { \
        clock_t end_time = clock(); \
        f64 duration = ((f64)(end_time - start_time)) / CLOCKS_PER_SEC; \
        if (stats_ptr) { \
            stats_ptr->total_inference_time += duration; \
            stats_ptr->total_inferences++; \
            stats_ptr->average_inference_time = stats_ptr->total_inference_time / stats_ptr->total_inferences; \
        } \
    } while(0)

#endif // ML_CORE_H
