/*
 * culling_frustum_builder_05.h
 *
 * Header file for culling_frustum_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_FRUSTUM_BUILDER_05_H
#define CULLING_FRUSTUM_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_frustum_builder_05 culling_frustum_builder_05_t;
typedef struct culling_frustum_builder_05_desc culling_frustum_builder_05_desc_t;
typedef struct culling_frustum_builder_05_stats culling_frustum_builder_05_stats_t;

/* Creation and destruction */
int culling_frustum_builder_05_create(culling_frustum_builder_05_t** out_ctx, const culling_frustum_builder_05_desc_t* desc);
int culling_frustum_builder_05_destroy(culling_frustum_builder_05_t* ctx);

/* Core operations */
int culling_frustum_builder_05_begin(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_end(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_add(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_remove(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_modify(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_finalize(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_validate(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_optimize(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_compile(culling_frustum_builder_05_t* ctx, void* params);
int culling_frustum_builder_05_link(culling_frustum_builder_05_t* ctx, void* params);

/* Utility functions */
int culling_frustum_builder_05_get_stats(culling_frustum_builder_05_t* ctx);
int culling_frustum_builder_05_set_callback(culling_frustum_builder_05_t* ctx);
int culling_frustum_builder_05_get_memory_usage(culling_frustum_builder_05_t* ctx);
int culling_frustum_builder_05_optimize(culling_frustum_builder_05_t* ctx);
int culling_frustum_builder_05_debug_print(culling_frustum_builder_05_t* ctx);

/* Module functions */
int culling_frustum_builder_05_module_init(void);
int culling_frustum_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_FRUSTUM_BUILDER_05_H */
