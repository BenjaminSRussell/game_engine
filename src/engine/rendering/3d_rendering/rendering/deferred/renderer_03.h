/*
 * rendering_deferred_renderer_03.h
 *
 * Header file for rendering_deferred_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_DEFERRED_RENDERER_03_H
#define RENDERING_DEFERRED_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_deferred_renderer_03 rendering_deferred_renderer_03_t;
typedef struct rendering_deferred_renderer_03_desc rendering_deferred_renderer_03_desc_t;
typedef struct rendering_deferred_renderer_03_stats rendering_deferred_renderer_03_stats_t;

/* Creation and destruction */
int rendering_deferred_renderer_03_create(rendering_deferred_renderer_03_t** out_ctx, const rendering_deferred_renderer_03_desc_t* desc);
int rendering_deferred_renderer_03_destroy(rendering_deferred_renderer_03_t* ctx);

/* Core operations */
int rendering_deferred_renderer_03_render(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_prepare(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_bind(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_draw(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_dispatch(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_submit_commands(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_build_commands(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_sort(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_batch(rendering_deferred_renderer_03_t* ctx, void* params);
int rendering_deferred_renderer_03_cull(rendering_deferred_renderer_03_t* ctx, void* params);

/* Utility functions */
int rendering_deferred_renderer_03_get_stats(rendering_deferred_renderer_03_t* ctx);
int rendering_deferred_renderer_03_set_callback(rendering_deferred_renderer_03_t* ctx);
int rendering_deferred_renderer_03_get_memory_usage(rendering_deferred_renderer_03_t* ctx);
int rendering_deferred_renderer_03_optimize(rendering_deferred_renderer_03_t* ctx);
int rendering_deferred_renderer_03_debug_print(rendering_deferred_renderer_03_t* ctx);

/* Module functions */
int rendering_deferred_renderer_03_module_init(void);
int rendering_deferred_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_DEFERRED_RENDERER_03_H */
