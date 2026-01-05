/*
 * shading_pbr_renderer_03.h
 *
 * Header file for shading_pbr_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_PBR_RENDERER_03_H
#define SHADING_PBR_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_pbr_renderer_03 shading_pbr_renderer_03_t;
typedef struct shading_pbr_renderer_03_desc shading_pbr_renderer_03_desc_t;
typedef struct shading_pbr_renderer_03_stats shading_pbr_renderer_03_stats_t;

/* Creation and destruction */
int shading_pbr_renderer_03_create(shading_pbr_renderer_03_t** out_ctx, const shading_pbr_renderer_03_desc_t* desc);
int shading_pbr_renderer_03_destroy(shading_pbr_renderer_03_t* ctx);

/* Core operations */
int shading_pbr_renderer_03_render(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_prepare(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_bind(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_draw(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_dispatch(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_submit_commands(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_build_commands(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_sort(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_batch(shading_pbr_renderer_03_t* ctx, void* params);
int shading_pbr_renderer_03_cull(shading_pbr_renderer_03_t* ctx, void* params);

/* Utility functions */
int shading_pbr_renderer_03_get_stats(shading_pbr_renderer_03_t* ctx);
int shading_pbr_renderer_03_set_callback(shading_pbr_renderer_03_t* ctx);
int shading_pbr_renderer_03_get_memory_usage(shading_pbr_renderer_03_t* ctx);
int shading_pbr_renderer_03_optimize(shading_pbr_renderer_03_t* ctx);
int shading_pbr_renderer_03_debug_print(shading_pbr_renderer_03_t* ctx);

/* Module functions */
int shading_pbr_renderer_03_module_init(void);
int shading_pbr_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_PBR_RENDERER_03_H */
