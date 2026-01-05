/*
 * shading_pbr_processor_04.h
 *
 * Header file for shading_pbr_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_PBR_PROCESSOR_04_H
#define SHADING_PBR_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_pbr_processor_04 shading_pbr_processor_04_t;
typedef struct shading_pbr_processor_04_desc shading_pbr_processor_04_desc_t;
typedef struct shading_pbr_processor_04_stats shading_pbr_processor_04_stats_t;

/* Creation and destruction */
int shading_pbr_processor_04_create(shading_pbr_processor_04_t** out_ctx, const shading_pbr_processor_04_desc_t* desc);
int shading_pbr_processor_04_destroy(shading_pbr_processor_04_t* ctx);

/* Core operations */
int shading_pbr_processor_04_process_batch(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_process_single(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_transform(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_filter(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_aggregate(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_dispatch(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_finalize(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_validate_input(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_optimize_output(shading_pbr_processor_04_t* ctx, void* params);
int shading_pbr_processor_04_profile(shading_pbr_processor_04_t* ctx, void* params);

/* Utility functions */
int shading_pbr_processor_04_get_stats(shading_pbr_processor_04_t* ctx);
int shading_pbr_processor_04_set_callback(shading_pbr_processor_04_t* ctx);
int shading_pbr_processor_04_get_memory_usage(shading_pbr_processor_04_t* ctx);
int shading_pbr_processor_04_optimize(shading_pbr_processor_04_t* ctx);
int shading_pbr_processor_04_debug_print(shading_pbr_processor_04_t* ctx);

/* Module functions */
int shading_pbr_processor_04_module_init(void);
int shading_pbr_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_PBR_PROCESSOR_04_H */
