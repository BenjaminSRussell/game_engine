/*
 * water_wetness_renderer_03.h
 *
 * Header file for water_wetness_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_WETNESS_RENDERER_03_H
#define WATER_WETNESS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_wetness_renderer_03 water_wetness_renderer_03_t;
typedef struct water_wetness_renderer_03_desc water_wetness_renderer_03_desc_t;
typedef struct water_wetness_renderer_03_stats water_wetness_renderer_03_stats_t;

/* Creation and destruction */
int water_wetness_renderer_03_create(water_wetness_renderer_03_t** out_ctx, const water_wetness_renderer_03_desc_t* desc);
int water_wetness_renderer_03_destroy(water_wetness_renderer_03_t* ctx);

/* Core operations */
int water_wetness_renderer_03_render(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_prepare(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_bind(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_draw(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_dispatch(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_submit_commands(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_build_commands(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_sort(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_batch(water_wetness_renderer_03_t* ctx, void* params);
int water_wetness_renderer_03_cull(water_wetness_renderer_03_t* ctx, void* params);

/* Utility functions */
int water_wetness_renderer_03_get_stats(water_wetness_renderer_03_t* ctx);
int water_wetness_renderer_03_set_callback(water_wetness_renderer_03_t* ctx);
int water_wetness_renderer_03_get_memory_usage(water_wetness_renderer_03_t* ctx);
int water_wetness_renderer_03_optimize(water_wetness_renderer_03_t* ctx);
int water_wetness_renderer_03_debug_print(water_wetness_renderer_03_t* ctx);

/* Module functions */
int water_wetness_renderer_03_module_init(void);
int water_wetness_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WETNESS_RENDERER_03_H */
