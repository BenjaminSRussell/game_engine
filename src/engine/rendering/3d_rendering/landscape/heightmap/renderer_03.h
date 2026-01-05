/*
 * landscape_heightmap_renderer_03.h
 *
 * Header file for landscape_heightmap_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_HEIGHTMAP_RENDERER_03_H
#define LANDSCAPE_HEIGHTMAP_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_heightmap_renderer_03 landscape_heightmap_renderer_03_t;
typedef struct landscape_heightmap_renderer_03_desc landscape_heightmap_renderer_03_desc_t;
typedef struct landscape_heightmap_renderer_03_stats landscape_heightmap_renderer_03_stats_t;

/* Creation and destruction */
int landscape_heightmap_renderer_03_create(landscape_heightmap_renderer_03_t** out_ctx, const landscape_heightmap_renderer_03_desc_t* desc);
int landscape_heightmap_renderer_03_destroy(landscape_heightmap_renderer_03_t* ctx);

/* Core operations */
int landscape_heightmap_renderer_03_render(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_prepare(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_bind(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_draw(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_dispatch(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_submit_commands(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_build_commands(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_sort(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_batch(landscape_heightmap_renderer_03_t* ctx, void* params);
int landscape_heightmap_renderer_03_cull(landscape_heightmap_renderer_03_t* ctx, void* params);

/* Utility functions */
int landscape_heightmap_renderer_03_get_stats(landscape_heightmap_renderer_03_t* ctx);
int landscape_heightmap_renderer_03_set_callback(landscape_heightmap_renderer_03_t* ctx);
int landscape_heightmap_renderer_03_get_memory_usage(landscape_heightmap_renderer_03_t* ctx);
int landscape_heightmap_renderer_03_optimize(landscape_heightmap_renderer_03_t* ctx);
int landscape_heightmap_renderer_03_debug_print(landscape_heightmap_renderer_03_t* ctx);

/* Module functions */
int landscape_heightmap_renderer_03_module_init(void);
int landscape_heightmap_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_HEIGHTMAP_RENDERER_03_H */
