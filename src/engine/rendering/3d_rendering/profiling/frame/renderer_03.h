/*
 * profiling_frame_renderer_03.h
 *
 * Header file for profiling_frame_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_FRAME_RENDERER_03_H
#define PROFILING_FRAME_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_frame_renderer_03 profiling_frame_renderer_03_t;
typedef struct profiling_frame_renderer_03_desc profiling_frame_renderer_03_desc_t;
typedef struct profiling_frame_renderer_03_stats profiling_frame_renderer_03_stats_t;

/* Creation and destruction */
int profiling_frame_renderer_03_create(profiling_frame_renderer_03_t** out_ctx, const profiling_frame_renderer_03_desc_t* desc);
int profiling_frame_renderer_03_destroy(profiling_frame_renderer_03_t* ctx);

/* Core operations */
int profiling_frame_renderer_03_render(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_prepare(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_bind(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_draw(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_dispatch(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_submit_commands(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_build_commands(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_sort(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_batch(profiling_frame_renderer_03_t* ctx, void* params);
int profiling_frame_renderer_03_cull(profiling_frame_renderer_03_t* ctx, void* params);

/* Utility functions */
int profiling_frame_renderer_03_get_stats(profiling_frame_renderer_03_t* ctx);
int profiling_frame_renderer_03_set_callback(profiling_frame_renderer_03_t* ctx);
int profiling_frame_renderer_03_get_memory_usage(profiling_frame_renderer_03_t* ctx);
int profiling_frame_renderer_03_optimize(profiling_frame_renderer_03_t* ctx);
int profiling_frame_renderer_03_debug_print(profiling_frame_renderer_03_t* ctx);

/* Module functions */
int profiling_frame_renderer_03_module_init(void);
int profiling_frame_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_FRAME_RENDERER_03_H */
