/*
 * raytracing_denoise_processor_04.h
 *
 * Header file for raytracing_denoise_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_DENOISE_PROCESSOR_04_H
#define RAYTRACING_DENOISE_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_denoise_processor_04 raytracing_denoise_processor_04_t;
typedef struct raytracing_denoise_processor_04_desc raytracing_denoise_processor_04_desc_t;
typedef struct raytracing_denoise_processor_04_stats raytracing_denoise_processor_04_stats_t;

/* Creation and destruction */
int raytracing_denoise_processor_04_create(raytracing_denoise_processor_04_t** out_ctx, const raytracing_denoise_processor_04_desc_t* desc);
int raytracing_denoise_processor_04_destroy(raytracing_denoise_processor_04_t* ctx);

/* Core operations */
int raytracing_denoise_processor_04_process_batch(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_process_single(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_transform(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_filter(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_aggregate(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_dispatch(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_finalize(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_validate_input(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_optimize_output(raytracing_denoise_processor_04_t* ctx, void* params);
int raytracing_denoise_processor_04_profile(raytracing_denoise_processor_04_t* ctx, void* params);

/* Utility functions */
int raytracing_denoise_processor_04_get_stats(raytracing_denoise_processor_04_t* ctx);
int raytracing_denoise_processor_04_set_callback(raytracing_denoise_processor_04_t* ctx);
int raytracing_denoise_processor_04_get_memory_usage(raytracing_denoise_processor_04_t* ctx);
int raytracing_denoise_processor_04_optimize(raytracing_denoise_processor_04_t* ctx);
int raytracing_denoise_processor_04_debug_print(raytracing_denoise_processor_04_t* ctx);

/* Module functions */
int raytracing_denoise_processor_04_module_init(void);
int raytracing_denoise_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_DENOISE_PROCESSOR_04_H */
