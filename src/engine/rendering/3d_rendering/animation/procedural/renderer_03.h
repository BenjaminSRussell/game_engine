/*
 * animation_procedural_renderer_03.h
 *
 * Header file for animation_procedural_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_PROCEDURAL_RENDERER_03_H
#define ANIMATION_PROCEDURAL_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_procedural_renderer_03 animation_procedural_renderer_03_t;
typedef struct animation_procedural_renderer_03_desc animation_procedural_renderer_03_desc_t;
typedef struct animation_procedural_renderer_03_stats animation_procedural_renderer_03_stats_t;

/* Creation and destruction */
int animation_procedural_renderer_03_create(animation_procedural_renderer_03_t** out_ctx, const animation_procedural_renderer_03_desc_t* desc);
int animation_procedural_renderer_03_destroy(animation_procedural_renderer_03_t* ctx);

/* Core operations */
int animation_procedural_renderer_03_render(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_prepare(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_bind(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_draw(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_dispatch(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_submit_commands(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_build_commands(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_sort(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_batch(animation_procedural_renderer_03_t* ctx, void* params);
int animation_procedural_renderer_03_cull(animation_procedural_renderer_03_t* ctx, void* params);

/* Utility functions */
int animation_procedural_renderer_03_get_stats(animation_procedural_renderer_03_t* ctx);
int animation_procedural_renderer_03_set_callback(animation_procedural_renderer_03_t* ctx);
int animation_procedural_renderer_03_get_memory_usage(animation_procedural_renderer_03_t* ctx);
int animation_procedural_renderer_03_optimize(animation_procedural_renderer_03_t* ctx);
int animation_procedural_renderer_03_debug_print(animation_procedural_renderer_03_t* ctx);

/* Module functions */
int animation_procedural_renderer_03_module_init(void);
int animation_procedural_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_PROCEDURAL_RENDERER_03_H */
