/*
 * viewports_split_screen_renderer_03.h
 *
 * Header file for viewports_split_screen_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_SPLIT_SCREEN_RENDERER_03_H
#define VIEWPORTS_SPLIT_SCREEN_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_split_screen_renderer_03 viewports_split_screen_renderer_03_t;
typedef struct viewports_split_screen_renderer_03_desc viewports_split_screen_renderer_03_desc_t;
typedef struct viewports_split_screen_renderer_03_stats viewports_split_screen_renderer_03_stats_t;

/* Creation and destruction */
int viewports_split_screen_renderer_03_create(viewports_split_screen_renderer_03_t** out_ctx, const viewports_split_screen_renderer_03_desc_t* desc);
int viewports_split_screen_renderer_03_destroy(viewports_split_screen_renderer_03_t* ctx);

/* Core operations */
int viewports_split_screen_renderer_03_render(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_prepare(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_bind(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_draw(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_dispatch(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_submit_commands(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_build_commands(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_sort(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_batch(viewports_split_screen_renderer_03_t* ctx, void* params);
int viewports_split_screen_renderer_03_cull(viewports_split_screen_renderer_03_t* ctx, void* params);

/* Utility functions */
int viewports_split_screen_renderer_03_get_stats(viewports_split_screen_renderer_03_t* ctx);
int viewports_split_screen_renderer_03_set_callback(viewports_split_screen_renderer_03_t* ctx);
int viewports_split_screen_renderer_03_get_memory_usage(viewports_split_screen_renderer_03_t* ctx);
int viewports_split_screen_renderer_03_optimize(viewports_split_screen_renderer_03_t* ctx);
int viewports_split_screen_renderer_03_debug_print(viewports_split_screen_renderer_03_t* ctx);

/* Module functions */
int viewports_split_screen_renderer_03_module_init(void);
int viewports_split_screen_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_SPLIT_SCREEN_RENDERER_03_H */
