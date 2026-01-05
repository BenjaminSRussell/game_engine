/*
 * profiling_export_renderer_03.h
 *
 * Header file for profiling_export_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_EXPORT_RENDERER_03_H
#define PROFILING_EXPORT_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_export_renderer_03 profiling_export_renderer_03_t;
typedef struct profiling_export_renderer_03_desc profiling_export_renderer_03_desc_t;
typedef struct profiling_export_renderer_03_stats profiling_export_renderer_03_stats_t;

/* Creation and destruction */
int profiling_export_renderer_03_create(profiling_export_renderer_03_t** out_ctx, const profiling_export_renderer_03_desc_t* desc);
int profiling_export_renderer_03_destroy(profiling_export_renderer_03_t* ctx);

/* Core operations */
int profiling_export_renderer_03_render(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_prepare(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_bind(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_draw(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_dispatch(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_submit_commands(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_build_commands(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_sort(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_batch(profiling_export_renderer_03_t* ctx, void* params);
int profiling_export_renderer_03_cull(profiling_export_renderer_03_t* ctx, void* params);

/* Utility functions */
int profiling_export_renderer_03_get_stats(profiling_export_renderer_03_t* ctx);
int profiling_export_renderer_03_set_callback(profiling_export_renderer_03_t* ctx);
int profiling_export_renderer_03_get_memory_usage(profiling_export_renderer_03_t* ctx);
int profiling_export_renderer_03_optimize(profiling_export_renderer_03_t* ctx);
int profiling_export_renderer_03_debug_print(profiling_export_renderer_03_t* ctx);

/* Module functions */
int profiling_export_renderer_03_module_init(void);
int profiling_export_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_EXPORT_RENDERER_03_H */
