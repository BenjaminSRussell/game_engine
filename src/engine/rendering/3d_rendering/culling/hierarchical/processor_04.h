/*
 * culling_hierarchical_processor_04.h
 *
 * Header file for culling_hierarchical_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_HIERARCHICAL_PROCESSOR_04_H
#define CULLING_HIERARCHICAL_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_hierarchical_processor_04 culling_hierarchical_processor_04_t;
typedef struct culling_hierarchical_processor_04_desc culling_hierarchical_processor_04_desc_t;
typedef struct culling_hierarchical_processor_04_stats culling_hierarchical_processor_04_stats_t;

/* Creation and destruction */
int culling_hierarchical_processor_04_create(culling_hierarchical_processor_04_t** out_ctx, const culling_hierarchical_processor_04_desc_t* desc);
int culling_hierarchical_processor_04_destroy(culling_hierarchical_processor_04_t* ctx);

/* Core operations */
int culling_hierarchical_processor_04_process_batch(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_process_single(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_transform(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_filter(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_aggregate(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_dispatch(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_finalize(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_validate_input(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_optimize_output(culling_hierarchical_processor_04_t* ctx, void* params);
int culling_hierarchical_processor_04_profile(culling_hierarchical_processor_04_t* ctx, void* params);

/* Utility functions */
int culling_hierarchical_processor_04_get_stats(culling_hierarchical_processor_04_t* ctx);
int culling_hierarchical_processor_04_set_callback(culling_hierarchical_processor_04_t* ctx);
int culling_hierarchical_processor_04_get_memory_usage(culling_hierarchical_processor_04_t* ctx);
int culling_hierarchical_processor_04_optimize(culling_hierarchical_processor_04_t* ctx);
int culling_hierarchical_processor_04_debug_print(culling_hierarchical_processor_04_t* ctx);

/* Module functions */
int culling_hierarchical_processor_04_module_init(void);
int culling_hierarchical_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HIERARCHICAL_PROCESSOR_04_H */
