/*
 * rendering_gpu_driven_processor_04.h
 *
 * Header file for rendering_gpu_driven_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GPU_DRIVEN_PROCESSOR_04_H
#define RENDERING_GPU_DRIVEN_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_gpu_driven_processor_04 rendering_gpu_driven_processor_04_t;
typedef struct rendering_gpu_driven_processor_04_desc rendering_gpu_driven_processor_04_desc_t;
typedef struct rendering_gpu_driven_processor_04_stats rendering_gpu_driven_processor_04_stats_t;

/* Creation and destruction */
int rendering_gpu_driven_processor_04_create(rendering_gpu_driven_processor_04_t** out_ctx, const rendering_gpu_driven_processor_04_desc_t* desc);
int rendering_gpu_driven_processor_04_destroy(rendering_gpu_driven_processor_04_t* ctx);

/* Core operations */
int rendering_gpu_driven_processor_04_process_batch(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_process_single(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_transform(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_filter(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_aggregate(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_dispatch(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_finalize(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_validate_input(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_optimize_output(rendering_gpu_driven_processor_04_t* ctx, void* params);
int rendering_gpu_driven_processor_04_profile(rendering_gpu_driven_processor_04_t* ctx, void* params);

/* Utility functions */
int rendering_gpu_driven_processor_04_get_stats(rendering_gpu_driven_processor_04_t* ctx);
int rendering_gpu_driven_processor_04_set_callback(rendering_gpu_driven_processor_04_t* ctx);
int rendering_gpu_driven_processor_04_get_memory_usage(rendering_gpu_driven_processor_04_t* ctx);
int rendering_gpu_driven_processor_04_optimize(rendering_gpu_driven_processor_04_t* ctx);
int rendering_gpu_driven_processor_04_debug_print(rendering_gpu_driven_processor_04_t* ctx);

/* Module functions */
int rendering_gpu_driven_processor_04_module_init(void);
int rendering_gpu_driven_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_DRIVEN_PROCESSOR_04_H */
