/*
 * shading_sheen_processor_04.h
 *
 * Header file for shading_sheen_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_SHEEN_PROCESSOR_04_H
#define SHADING_SHEEN_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_sheen_processor_04 shading_sheen_processor_04_t;
typedef struct shading_sheen_processor_04_desc shading_sheen_processor_04_desc_t;
typedef struct shading_sheen_processor_04_stats shading_sheen_processor_04_stats_t;

/* Creation and destruction */
int shading_sheen_processor_04_create(shading_sheen_processor_04_t** out_ctx, const shading_sheen_processor_04_desc_t* desc);
int shading_sheen_processor_04_destroy(shading_sheen_processor_04_t* ctx);

/* Core operations */
int shading_sheen_processor_04_process_batch(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_process_single(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_transform(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_filter(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_aggregate(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_dispatch(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_finalize(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_validate_input(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_optimize_output(shading_sheen_processor_04_t* ctx, void* params);
int shading_sheen_processor_04_profile(shading_sheen_processor_04_t* ctx, void* params);

/* Utility functions */
int shading_sheen_processor_04_get_stats(shading_sheen_processor_04_t* ctx);
int shading_sheen_processor_04_set_callback(shading_sheen_processor_04_t* ctx);
int shading_sheen_processor_04_get_memory_usage(shading_sheen_processor_04_t* ctx);
int shading_sheen_processor_04_optimize(shading_sheen_processor_04_t* ctx);
int shading_sheen_processor_04_debug_print(shading_sheen_processor_04_t* ctx);

/* Module functions */
int shading_sheen_processor_04_module_init(void);
int shading_sheen_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SHEEN_PROCESSOR_04_H */
