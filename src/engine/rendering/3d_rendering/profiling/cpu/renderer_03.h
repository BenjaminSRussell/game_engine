/*
 * profiling_cpu_renderer_03.h
 *
 * Header file for profiling_cpu_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_CPU_RENDERER_03_H
#define PROFILING_CPU_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_cpu_renderer_03 profiling_cpu_renderer_03_t;
typedef struct profiling_cpu_renderer_03_desc profiling_cpu_renderer_03_desc_t;
typedef struct profiling_cpu_renderer_03_stats profiling_cpu_renderer_03_stats_t;

/* Creation and destruction */
int profiling_cpu_renderer_03_create(profiling_cpu_renderer_03_t** out_ctx, const profiling_cpu_renderer_03_desc_t* desc);
int profiling_cpu_renderer_03_destroy(profiling_cpu_renderer_03_t* ctx);

/* Core operations */
int profiling_cpu_renderer_03_render(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_prepare(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_bind(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_draw(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_dispatch(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_submit_commands(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_build_commands(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_sort(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_batch(profiling_cpu_renderer_03_t* ctx, void* params);
int profiling_cpu_renderer_03_cull(profiling_cpu_renderer_03_t* ctx, void* params);

/* Utility functions */
int profiling_cpu_renderer_03_get_stats(profiling_cpu_renderer_03_t* ctx);
int profiling_cpu_renderer_03_set_callback(profiling_cpu_renderer_03_t* ctx);
int profiling_cpu_renderer_03_get_memory_usage(profiling_cpu_renderer_03_t* ctx);
int profiling_cpu_renderer_03_optimize(profiling_cpu_renderer_03_t* ctx);
int profiling_cpu_renderer_03_debug_print(profiling_cpu_renderer_03_t* ctx);

/* Module functions */
int profiling_cpu_renderer_03_module_init(void);
int profiling_cpu_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_CPU_RENDERER_03_H */
