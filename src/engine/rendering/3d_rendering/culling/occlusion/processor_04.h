/*
 * culling_occlusion_processor_04.h
 *
 * Header file for culling_occlusion_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_OCCLUSION_PROCESSOR_04_H
#define CULLING_OCCLUSION_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_occlusion_processor_04 culling_occlusion_processor_04_t;
typedef struct culling_occlusion_processor_04_desc culling_occlusion_processor_04_desc_t;
typedef struct culling_occlusion_processor_04_stats culling_occlusion_processor_04_stats_t;

/* Creation and destruction */
int culling_occlusion_processor_04_create(culling_occlusion_processor_04_t** out_ctx, const culling_occlusion_processor_04_desc_t* desc);
int culling_occlusion_processor_04_destroy(culling_occlusion_processor_04_t* ctx);

/* Core operations */
int culling_occlusion_processor_04_process_batch(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_process_single(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_transform(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_filter(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_aggregate(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_dispatch(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_finalize(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_validate_input(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_optimize_output(culling_occlusion_processor_04_t* ctx, void* params);
int culling_occlusion_processor_04_profile(culling_occlusion_processor_04_t* ctx, void* params);

/* Utility functions */
int culling_occlusion_processor_04_get_stats(culling_occlusion_processor_04_t* ctx);
int culling_occlusion_processor_04_set_callback(culling_occlusion_processor_04_t* ctx);
int culling_occlusion_processor_04_get_memory_usage(culling_occlusion_processor_04_t* ctx);
int culling_occlusion_processor_04_optimize(culling_occlusion_processor_04_t* ctx);
int culling_occlusion_processor_04_debug_print(culling_occlusion_processor_04_t* ctx);

/* Module functions */
int culling_occlusion_processor_04_module_init(void);
int culling_occlusion_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_OCCLUSION_PROCESSOR_04_H */
