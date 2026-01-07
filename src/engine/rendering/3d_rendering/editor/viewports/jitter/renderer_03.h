/*
 * viewports_jitter_renderer_03.h
 *
 * Header file for viewports_jitter_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_JITTER_RENDERER_03_H
#define VIEWPORTS_JITTER_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_jitter_renderer_03 viewports_jitter_renderer_03_t;
typedef struct viewports_jitter_renderer_03_desc viewports_jitter_renderer_03_desc_t;
typedef struct viewports_jitter_renderer_03_stats viewports_jitter_renderer_03_stats_t;

/* Creation and destruction */
int viewports_jitter_renderer_03_create(viewports_jitter_renderer_03_t** out_ctx, const viewports_jitter_renderer_03_desc_t* desc);
int viewports_jitter_renderer_03_destroy(viewports_jitter_renderer_03_t* ctx);

/* Core operations */
int viewports_jitter_renderer_03_render(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_prepare(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_bind(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_draw(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_dispatch(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_submit_commands(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_build_commands(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_sort(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_batch(viewports_jitter_renderer_03_t* ctx, void* params);
int viewports_jitter_renderer_03_cull(viewports_jitter_renderer_03_t* ctx, void* params);

/* Utility functions */
int viewports_jitter_renderer_03_get_stats(viewports_jitter_renderer_03_t* ctx);
int viewports_jitter_renderer_03_set_callback(viewports_jitter_renderer_03_t* ctx);
int viewports_jitter_renderer_03_get_memory_usage(viewports_jitter_renderer_03_t* ctx);
int viewports_jitter_renderer_03_optimize(viewports_jitter_renderer_03_t* ctx);
int viewports_jitter_renderer_03_debug_print(viewports_jitter_renderer_03_t* ctx);

/* Module functions */
int viewports_jitter_renderer_03_module_init(void);
int viewports_jitter_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_JITTER_RENDERER_03_H */
