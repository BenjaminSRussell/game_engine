/*
 * framebuffer_multiview_renderer_03.h
 *
 * Header file for framebuffer_multiview_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_MULTIVIEW_RENDERER_03_H
#define FRAMEBUFFER_MULTIVIEW_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_multiview_renderer_03 framebuffer_multiview_renderer_03_t;
typedef struct framebuffer_multiview_renderer_03_desc framebuffer_multiview_renderer_03_desc_t;
typedef struct framebuffer_multiview_renderer_03_stats framebuffer_multiview_renderer_03_stats_t;

/* Creation and destruction */
int framebuffer_multiview_renderer_03_create(framebuffer_multiview_renderer_03_t** out_ctx, const framebuffer_multiview_renderer_03_desc_t* desc);
int framebuffer_multiview_renderer_03_destroy(framebuffer_multiview_renderer_03_t* ctx);

/* Core operations */
int framebuffer_multiview_renderer_03_render(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_prepare(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_bind(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_draw(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_dispatch(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_submit_commands(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_build_commands(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_sort(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_batch(framebuffer_multiview_renderer_03_t* ctx, void* params);
int framebuffer_multiview_renderer_03_cull(framebuffer_multiview_renderer_03_t* ctx, void* params);

/* Utility functions */
int framebuffer_multiview_renderer_03_get_stats(framebuffer_multiview_renderer_03_t* ctx);
int framebuffer_multiview_renderer_03_set_callback(framebuffer_multiview_renderer_03_t* ctx);
int framebuffer_multiview_renderer_03_get_memory_usage(framebuffer_multiview_renderer_03_t* ctx);
int framebuffer_multiview_renderer_03_optimize(framebuffer_multiview_renderer_03_t* ctx);
int framebuffer_multiview_renderer_03_debug_print(framebuffer_multiview_renderer_03_t* ctx);

/* Module functions */
int framebuffer_multiview_renderer_03_module_init(void);
int framebuffer_multiview_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_MULTIVIEW_RENDERER_03_H */
