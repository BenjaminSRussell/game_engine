/*
 * framebuffer_tiled_renderer_03.h
 *
 * Header file for framebuffer_tiled_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef FRAMEBUFFER_TILED_RENDERER_03_H
#define FRAMEBUFFER_TILED_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct framebuffer_tiled_renderer_03 framebuffer_tiled_renderer_03_t;
typedef struct framebuffer_tiled_renderer_03_desc framebuffer_tiled_renderer_03_desc_t;
typedef struct framebuffer_tiled_renderer_03_stats framebuffer_tiled_renderer_03_stats_t;

/* Creation and destruction */
int framebuffer_tiled_renderer_03_create(framebuffer_tiled_renderer_03_t** out_ctx, const framebuffer_tiled_renderer_03_desc_t* desc);
int framebuffer_tiled_renderer_03_destroy(framebuffer_tiled_renderer_03_t* ctx);

/* Core operations */
int framebuffer_tiled_renderer_03_render(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_prepare(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_bind(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_draw(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_dispatch(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_submit_commands(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_build_commands(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_sort(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_batch(framebuffer_tiled_renderer_03_t* ctx, void* params);
int framebuffer_tiled_renderer_03_cull(framebuffer_tiled_renderer_03_t* ctx, void* params);

/* Utility functions */
int framebuffer_tiled_renderer_03_get_stats(framebuffer_tiled_renderer_03_t* ctx);
int framebuffer_tiled_renderer_03_set_callback(framebuffer_tiled_renderer_03_t* ctx);
int framebuffer_tiled_renderer_03_get_memory_usage(framebuffer_tiled_renderer_03_t* ctx);
int framebuffer_tiled_renderer_03_optimize(framebuffer_tiled_renderer_03_t* ctx);
int framebuffer_tiled_renderer_03_debug_print(framebuffer_tiled_renderer_03_t* ctx);

/* Module functions */
int framebuffer_tiled_renderer_03_module_init(void);
int framebuffer_tiled_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_TILED_RENDERER_03_H */
