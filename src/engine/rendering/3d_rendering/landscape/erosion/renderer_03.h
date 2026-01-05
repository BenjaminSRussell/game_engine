/*
 * landscape_erosion_renderer_03.h
 *
 * Header file for landscape_erosion_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_EROSION_RENDERER_03_H
#define LANDSCAPE_EROSION_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_erosion_renderer_03 landscape_erosion_renderer_03_t;
typedef struct landscape_erosion_renderer_03_desc landscape_erosion_renderer_03_desc_t;
typedef struct landscape_erosion_renderer_03_stats landscape_erosion_renderer_03_stats_t;

/* Creation and destruction */
int landscape_erosion_renderer_03_create(landscape_erosion_renderer_03_t** out_ctx, const landscape_erosion_renderer_03_desc_t* desc);
int landscape_erosion_renderer_03_destroy(landscape_erosion_renderer_03_t* ctx);

/* Core operations */
int landscape_erosion_renderer_03_render(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_prepare(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_bind(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_draw(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_dispatch(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_submit_commands(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_build_commands(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_sort(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_batch(landscape_erosion_renderer_03_t* ctx, void* params);
int landscape_erosion_renderer_03_cull(landscape_erosion_renderer_03_t* ctx, void* params);

/* Utility functions */
int landscape_erosion_renderer_03_get_stats(landscape_erosion_renderer_03_t* ctx);
int landscape_erosion_renderer_03_set_callback(landscape_erosion_renderer_03_t* ctx);
int landscape_erosion_renderer_03_get_memory_usage(landscape_erosion_renderer_03_t* ctx);
int landscape_erosion_renderer_03_optimize(landscape_erosion_renderer_03_t* ctx);
int landscape_erosion_renderer_03_debug_print(landscape_erosion_renderer_03_t* ctx);

/* Module functions */
int landscape_erosion_renderer_03_module_init(void);
int landscape_erosion_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_EROSION_RENDERER_03_H */
