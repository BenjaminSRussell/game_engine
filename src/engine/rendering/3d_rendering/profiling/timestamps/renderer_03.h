/*
 * profiling_timestamps_renderer_03.h
 *
 * Header file for profiling_timestamps_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_TIMESTAMPS_RENDERER_03_H
#define PROFILING_TIMESTAMPS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_timestamps_renderer_03 profiling_timestamps_renderer_03_t;
typedef struct profiling_timestamps_renderer_03_desc profiling_timestamps_renderer_03_desc_t;
typedef struct profiling_timestamps_renderer_03_stats profiling_timestamps_renderer_03_stats_t;

/* Creation and destruction */
int profiling_timestamps_renderer_03_create(profiling_timestamps_renderer_03_t** out_ctx, const profiling_timestamps_renderer_03_desc_t* desc);
int profiling_timestamps_renderer_03_destroy(profiling_timestamps_renderer_03_t* ctx);

/* Core operations */
int profiling_timestamps_renderer_03_render(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_prepare(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_bind(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_draw(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_dispatch(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_submit_commands(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_build_commands(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_sort(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_batch(profiling_timestamps_renderer_03_t* ctx, void* params);
int profiling_timestamps_renderer_03_cull(profiling_timestamps_renderer_03_t* ctx, void* params);

/* Utility functions */
int profiling_timestamps_renderer_03_get_stats(profiling_timestamps_renderer_03_t* ctx);
int profiling_timestamps_renderer_03_set_callback(profiling_timestamps_renderer_03_t* ctx);
int profiling_timestamps_renderer_03_get_memory_usage(profiling_timestamps_renderer_03_t* ctx);
int profiling_timestamps_renderer_03_optimize(profiling_timestamps_renderer_03_t* ctx);
int profiling_timestamps_renderer_03_debug_print(profiling_timestamps_renderer_03_t* ctx);

/* Module functions */
int profiling_timestamps_renderer_03_module_init(void);
int profiling_timestamps_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_TIMESTAMPS_RENDERER_03_H */
