/*
 * raytracing_ao_processor_04.h
 *
 * Header file for raytracing_ao_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_AO_PROCESSOR_04_H
#define RAYTRACING_AO_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_ao_processor_04 raytracing_ao_processor_04_t;
typedef struct raytracing_ao_processor_04_desc raytracing_ao_processor_04_desc_t;
typedef struct raytracing_ao_processor_04_stats raytracing_ao_processor_04_stats_t;

/* Creation and destruction */
int raytracing_ao_processor_04_create(raytracing_ao_processor_04_t** out_ctx, const raytracing_ao_processor_04_desc_t* desc);
int raytracing_ao_processor_04_destroy(raytracing_ao_processor_04_t* ctx);

/* Core operations */
int raytracing_ao_processor_04_process_batch(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_process_single(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_transform(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_filter(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_aggregate(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_dispatch(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_finalize(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_validate_input(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_optimize_output(raytracing_ao_processor_04_t* ctx, void* params);
int raytracing_ao_processor_04_profile(raytracing_ao_processor_04_t* ctx, void* params);

/* Utility functions */
int raytracing_ao_processor_04_get_stats(raytracing_ao_processor_04_t* ctx);
int raytracing_ao_processor_04_set_callback(raytracing_ao_processor_04_t* ctx);
int raytracing_ao_processor_04_get_memory_usage(raytracing_ao_processor_04_t* ctx);
int raytracing_ao_processor_04_optimize(raytracing_ao_processor_04_t* ctx);
int raytracing_ao_processor_04_debug_print(raytracing_ao_processor_04_t* ctx);

/* Module functions */
int raytracing_ao_processor_04_module_init(void);
int raytracing_ao_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_AO_PROCESSOR_04_H */
