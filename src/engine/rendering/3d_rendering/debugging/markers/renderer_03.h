/*
 * debugging_markers_renderer_03.h
 *
 * Header file for debugging_markers_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef DEBUGGING_MARKERS_RENDERER_03_H
#define DEBUGGING_MARKERS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct debugging_markers_renderer_03 debugging_markers_renderer_03_t;
typedef struct debugging_markers_renderer_03_desc debugging_markers_renderer_03_desc_t;
typedef struct debugging_markers_renderer_03_stats debugging_markers_renderer_03_stats_t;

/* Creation and destruction */
int debugging_markers_renderer_03_create(debugging_markers_renderer_03_t** out_ctx, const debugging_markers_renderer_03_desc_t* desc);
int debugging_markers_renderer_03_destroy(debugging_markers_renderer_03_t* ctx);

/* Core operations */
int debugging_markers_renderer_03_render(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_prepare(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_bind(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_draw(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_dispatch(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_submit_commands(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_build_commands(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_sort(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_batch(debugging_markers_renderer_03_t* ctx, void* params);
int debugging_markers_renderer_03_cull(debugging_markers_renderer_03_t* ctx, void* params);

/* Utility functions */
int debugging_markers_renderer_03_get_stats(debugging_markers_renderer_03_t* ctx);
int debugging_markers_renderer_03_set_callback(debugging_markers_renderer_03_t* ctx);
int debugging_markers_renderer_03_get_memory_usage(debugging_markers_renderer_03_t* ctx);
int debugging_markers_renderer_03_optimize(debugging_markers_renderer_03_t* ctx);
int debugging_markers_renderer_03_debug_print(debugging_markers_renderer_03_t* ctx);

/* Module functions */
int debugging_markers_renderer_03_module_init(void);
int debugging_markers_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGING_MARKERS_RENDERER_03_H */
