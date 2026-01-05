/*
 * geometry_batching_renderer_03.h
 *
 * Header file for geometry_batching_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_BATCHING_RENDERER_03_H
#define GEOMETRY_BATCHING_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_batching_renderer_03 geometry_batching_renderer_03_t;
typedef struct geometry_batching_renderer_03_desc geometry_batching_renderer_03_desc_t;
typedef struct geometry_batching_renderer_03_stats geometry_batching_renderer_03_stats_t;

/* Creation and destruction */
int geometry_batching_renderer_03_create(geometry_batching_renderer_03_t** out_ctx, const geometry_batching_renderer_03_desc_t* desc);
int geometry_batching_renderer_03_destroy(geometry_batching_renderer_03_t* ctx);

/* Core operations */
int geometry_batching_renderer_03_render(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_prepare(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_bind(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_draw(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_dispatch(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_submit_commands(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_build_commands(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_sort(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_batch(geometry_batching_renderer_03_t* ctx, void* params);
int geometry_batching_renderer_03_cull(geometry_batching_renderer_03_t* ctx, void* params);

/* Utility functions */
int geometry_batching_renderer_03_get_stats(geometry_batching_renderer_03_t* ctx);
int geometry_batching_renderer_03_set_callback(geometry_batching_renderer_03_t* ctx);
int geometry_batching_renderer_03_get_memory_usage(geometry_batching_renderer_03_t* ctx);
int geometry_batching_renderer_03_optimize(geometry_batching_renderer_03_t* ctx);
int geometry_batching_renderer_03_debug_print(geometry_batching_renderer_03_t* ctx);

/* Module functions */
int geometry_batching_renderer_03_module_init(void);
int geometry_batching_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_BATCHING_RENDERER_03_H */
