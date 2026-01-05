/*
 * culling_portal_processor_04.h
 *
 * Header file for culling_portal_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PORTAL_PROCESSOR_04_H
#define CULLING_PORTAL_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_portal_processor_04 culling_portal_processor_04_t;
typedef struct culling_portal_processor_04_desc culling_portal_processor_04_desc_t;
typedef struct culling_portal_processor_04_stats culling_portal_processor_04_stats_t;

/* Creation and destruction */
int culling_portal_processor_04_create(culling_portal_processor_04_t** out_ctx, const culling_portal_processor_04_desc_t* desc);
int culling_portal_processor_04_destroy(culling_portal_processor_04_t* ctx);

/* Core operations */
int culling_portal_processor_04_process_batch(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_process_single(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_transform(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_filter(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_aggregate(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_dispatch(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_finalize(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_validate_input(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_optimize_output(culling_portal_processor_04_t* ctx, void* params);
int culling_portal_processor_04_profile(culling_portal_processor_04_t* ctx, void* params);

/* Utility functions */
int culling_portal_processor_04_get_stats(culling_portal_processor_04_t* ctx);
int culling_portal_processor_04_set_callback(culling_portal_processor_04_t* ctx);
int culling_portal_processor_04_get_memory_usage(culling_portal_processor_04_t* ctx);
int culling_portal_processor_04_optimize(culling_portal_processor_04_t* ctx);
int culling_portal_processor_04_debug_print(culling_portal_processor_04_t* ctx);

/* Module functions */
int culling_portal_processor_04_module_init(void);
int culling_portal_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PORTAL_PROCESSOR_04_H */
