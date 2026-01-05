/*
 * landscape_terrain_renderer_03.h
 *
 * Header file for landscape_terrain_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_TERRAIN_RENDERER_03_H
#define LANDSCAPE_TERRAIN_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_terrain_renderer_03 landscape_terrain_renderer_03_t;
typedef struct landscape_terrain_renderer_03_desc landscape_terrain_renderer_03_desc_t;
typedef struct landscape_terrain_renderer_03_stats landscape_terrain_renderer_03_stats_t;

/* Creation and destruction */
int landscape_terrain_renderer_03_create(landscape_terrain_renderer_03_t** out_ctx, const landscape_terrain_renderer_03_desc_t* desc);
int landscape_terrain_renderer_03_destroy(landscape_terrain_renderer_03_t* ctx);

/* Core operations */
int landscape_terrain_renderer_03_render(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_prepare(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_bind(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_draw(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_dispatch(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_submit_commands(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_build_commands(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_sort(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_batch(landscape_terrain_renderer_03_t* ctx, void* params);
int landscape_terrain_renderer_03_cull(landscape_terrain_renderer_03_t* ctx, void* params);

/* Utility functions */
int landscape_terrain_renderer_03_get_stats(landscape_terrain_renderer_03_t* ctx);
int landscape_terrain_renderer_03_set_callback(landscape_terrain_renderer_03_t* ctx);
int landscape_terrain_renderer_03_get_memory_usage(landscape_terrain_renderer_03_t* ctx);
int landscape_terrain_renderer_03_optimize(landscape_terrain_renderer_03_t* ctx);
int landscape_terrain_renderer_03_debug_print(landscape_terrain_renderer_03_t* ctx);

/* Module functions */
int landscape_terrain_renderer_03_module_init(void);
int landscape_terrain_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_TERRAIN_RENDERER_03_H */
