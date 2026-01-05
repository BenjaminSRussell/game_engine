/*
 * culling_frustum_renderer_03.h
 *
 * Header file for culling_frustum_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_FRUSTUM_RENDERER_03_H
#define CULLING_FRUSTUM_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_frustum_renderer_03 culling_frustum_renderer_03_t;
typedef struct culling_frustum_renderer_03_desc culling_frustum_renderer_03_desc_t;
typedef struct culling_frustum_renderer_03_stats culling_frustum_renderer_03_stats_t;

/* Creation and destruction */
int culling_frustum_renderer_03_create(culling_frustum_renderer_03_t** out_ctx, const culling_frustum_renderer_03_desc_t* desc);
int culling_frustum_renderer_03_destroy(culling_frustum_renderer_03_t* ctx);

/* Core operations */
int culling_frustum_renderer_03_render(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_prepare(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_bind(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_draw(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_dispatch(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_submit_commands(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_build_commands(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_sort(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_batch(culling_frustum_renderer_03_t* ctx, void* params);
int culling_frustum_renderer_03_cull(culling_frustum_renderer_03_t* ctx, void* params);

/* Utility functions */
int culling_frustum_renderer_03_get_stats(culling_frustum_renderer_03_t* ctx);
int culling_frustum_renderer_03_set_callback(culling_frustum_renderer_03_t* ctx);
int culling_frustum_renderer_03_get_memory_usage(culling_frustum_renderer_03_t* ctx);
int culling_frustum_renderer_03_optimize(culling_frustum_renderer_03_t* ctx);
int culling_frustum_renderer_03_debug_print(culling_frustum_renderer_03_t* ctx);

/* Module functions */
int culling_frustum_renderer_03_module_init(void);
int culling_frustum_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_FRUSTUM_RENDERER_03_H */
