/*
 * viewports_controllers_renderer_03.h
 *
 * Header file for viewports_controllers_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_CONTROLLERS_RENDERER_03_H
#define VIEWPORTS_CONTROLLERS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_controllers_renderer_03 viewports_controllers_renderer_03_t;
typedef struct viewports_controllers_renderer_03_desc viewports_controllers_renderer_03_desc_t;
typedef struct viewports_controllers_renderer_03_stats viewports_controllers_renderer_03_stats_t;

/* Creation and destruction */
int viewports_controllers_renderer_03_create(viewports_controllers_renderer_03_t** out_ctx, const viewports_controllers_renderer_03_desc_t* desc);
int viewports_controllers_renderer_03_destroy(viewports_controllers_renderer_03_t* ctx);

/* Core operations */
int viewports_controllers_renderer_03_render(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_prepare(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_bind(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_draw(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_dispatch(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_submit_commands(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_build_commands(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_sort(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_batch(viewports_controllers_renderer_03_t* ctx, void* params);
int viewports_controllers_renderer_03_cull(viewports_controllers_renderer_03_t* ctx, void* params);

/* Utility functions */
int viewports_controllers_renderer_03_get_stats(viewports_controllers_renderer_03_t* ctx);
int viewports_controllers_renderer_03_set_callback(viewports_controllers_renderer_03_t* ctx);
int viewports_controllers_renderer_03_get_memory_usage(viewports_controllers_renderer_03_t* ctx);
int viewports_controllers_renderer_03_optimize(viewports_controllers_renderer_03_t* ctx);
int viewports_controllers_renderer_03_debug_print(viewports_controllers_renderer_03_t* ctx);

/* Module functions */
int viewports_controllers_renderer_03_module_init(void);
int viewports_controllers_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_CONTROLLERS_RENDERER_03_H */
