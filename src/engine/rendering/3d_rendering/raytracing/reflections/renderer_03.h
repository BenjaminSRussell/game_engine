/*
 * raytracing_reflections_renderer_03.h
 *
 * Header file for raytracing_reflections_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_REFLECTIONS_RENDERER_03_H
#define RAYTRACING_REFLECTIONS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_reflections_renderer_03 raytracing_reflections_renderer_03_t;
typedef struct raytracing_reflections_renderer_03_desc raytracing_reflections_renderer_03_desc_t;
typedef struct raytracing_reflections_renderer_03_stats raytracing_reflections_renderer_03_stats_t;

/* Creation and destruction */
int raytracing_reflections_renderer_03_create(raytracing_reflections_renderer_03_t** out_ctx, const raytracing_reflections_renderer_03_desc_t* desc);
int raytracing_reflections_renderer_03_destroy(raytracing_reflections_renderer_03_t* ctx);

/* Core operations */
int raytracing_reflections_renderer_03_render(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_prepare(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_bind(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_draw(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_dispatch(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_submit_commands(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_build_commands(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_sort(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_batch(raytracing_reflections_renderer_03_t* ctx, void* params);
int raytracing_reflections_renderer_03_cull(raytracing_reflections_renderer_03_t* ctx, void* params);

/* Utility functions */
int raytracing_reflections_renderer_03_get_stats(raytracing_reflections_renderer_03_t* ctx);
int raytracing_reflections_renderer_03_set_callback(raytracing_reflections_renderer_03_t* ctx);
int raytracing_reflections_renderer_03_get_memory_usage(raytracing_reflections_renderer_03_t* ctx);
int raytracing_reflections_renderer_03_optimize(raytracing_reflections_renderer_03_t* ctx);
int raytracing_reflections_renderer_03_debug_print(raytracing_reflections_renderer_03_t* ctx);

/* Module functions */
int raytracing_reflections_renderer_03_module_init(void);
int raytracing_reflections_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_REFLECTIONS_RENDERER_03_H */
