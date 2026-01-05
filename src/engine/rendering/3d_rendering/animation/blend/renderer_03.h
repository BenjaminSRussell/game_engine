/*
 * animation_blend_renderer_03.h
 *
 * Header file for animation_blend_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_BLEND_RENDERER_03_H
#define ANIMATION_BLEND_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_blend_renderer_03 animation_blend_renderer_03_t;
typedef struct animation_blend_renderer_03_desc animation_blend_renderer_03_desc_t;
typedef struct animation_blend_renderer_03_stats animation_blend_renderer_03_stats_t;

/* Creation and destruction */
int animation_blend_renderer_03_create(animation_blend_renderer_03_t** out_ctx, const animation_blend_renderer_03_desc_t* desc);
int animation_blend_renderer_03_destroy(animation_blend_renderer_03_t* ctx);

/* Core operations */
int animation_blend_renderer_03_render(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_prepare(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_bind(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_draw(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_dispatch(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_submit_commands(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_build_commands(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_sort(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_batch(animation_blend_renderer_03_t* ctx, void* params);
int animation_blend_renderer_03_cull(animation_blend_renderer_03_t* ctx, void* params);

/* Utility functions */
int animation_blend_renderer_03_get_stats(animation_blend_renderer_03_t* ctx);
int animation_blend_renderer_03_set_callback(animation_blend_renderer_03_t* ctx);
int animation_blend_renderer_03_get_memory_usage(animation_blend_renderer_03_t* ctx);
int animation_blend_renderer_03_optimize(animation_blend_renderer_03_t* ctx);
int animation_blend_renderer_03_debug_print(animation_blend_renderer_03_t* ctx);

/* Module functions */
int animation_blend_renderer_03_module_init(void);
int animation_blend_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_BLEND_RENDERER_03_H */
