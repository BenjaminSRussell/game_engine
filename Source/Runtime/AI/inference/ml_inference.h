#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ml_inference_context_t ml_inference_context_t;
typedef struct ml_model_t ml_model_t;
typedef struct ml_tensor_t ml_tensor_t;

typedef struct {
    float* data;
    uint32_t* shape;
    uint32_t shape_size;
    uint32_t total_elements;
    char* name;
} ml_tensor_desc_t;

typedef struct {
    char* model_path;
    char** input_names;
    char** output_names;
    uint32_t input_count;
    uint32_t output_count;
    bool use_gpu;
    uint32_t gpu_device_id;
} ml_model_desc_t;

typedef struct {
    uint64_t inference_time_ns;
    uint64_t preprocessing_time_ns;
    uint64_t postprocessing_time_ns;
    uint32_t input_size_bytes;
    uint32_t output_size_bytes;
    float memory_usage_mb;
    bool gpu_accelerated;
} ml_inference_stats_t;

typedef enum {
    ML_SUCCESS = 0,
    ML_ERROR_INVALID_PARAM = -1,
    ML_ERROR_OUT_OF_MEMORY = -2,
    ML_ERROR_MODEL_LOAD_FAILED = -3,
    ML_ERROR_INFERENCE_FAILED = -4,
    ML_ERROR_NOT_INITIALIZED = -5,
    ML_ERROR_TENSOR_MISMATCH = -6,
    ML_ERROR_UNSUPPORTED_FORMAT = -7
} ml_error_t;

ml_error_t ml_inference_init(ml_inference_context_t** context);
void ml_inference_shutdown(ml_inference_context_t* context);

ml_error_t ml_model_load(ml_inference_context_t* context, const ml_model_desc_t* desc, ml_model_t** model);
ml_error_t ml_model_unload(ml_inference_context_t* context, ml_model_t* model);

ml_error_t ml_tensor_create(ml_inference_context_t* context, const ml_tensor_desc_t* desc, ml_tensor_t** tensor);
ml_error_t ml_tensor_destroy(ml_inference_context_t* context, ml_tensor_t* tensor);
ml_error_t ml_tensor_set_data(ml_tensor_t* tensor, const float* data);
ml_error_t ml_tensor_get_data(ml_tensor_t* tensor, float** data);

ml_error_t ml_inference_run(ml_inference_context_t* context, 
                           ml_model_t* model,
                           ml_tensor_t** inputs,
                           uint32_t input_count,
                           ml_tensor_t** outputs,
                           uint32_t output_count);

ml_error_t ml_inference_run_async(ml_inference_context_t* context,
                                 ml_model_t* model,
                                 ml_tensor_t** inputs,
                                 uint32_t input_count,
                                 ml_tensor_t** outputs,
                                 uint32_t output_count);

ml_error_t ml_inference_wait(ml_inference_context_t* context, uint64_t timeout_ms);

ml_error_t ml_inference_get_stats(ml_inference_context_t* context, ml_inference_stats_t* stats);
ml_error_t ml_inference_reset_stats(ml_inference_context_t* context);

ml_error_t ml_inference_set_optimization_level(ml_inference_context_t* context, int level);
ml_error_t ml_inference_enable_profiling(ml_inference_context_t* context, bool enabled);

#ifdef __cplusplus
}
#endif

#endif
