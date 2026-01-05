/*
 * shading_clearcoat_renderer_03.h
 *
 * Header file for shading_clearcoat_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_CLEARCOAT_RENDERER_03_H
#define SHADING_CLEARCOAT_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_clearcoat_renderer_03 shading_clearcoat_renderer_03_t;
typedef struct shading_clearcoat_renderer_03_desc shading_clearcoat_renderer_03_desc_t;
typedef struct shading_clearcoat_renderer_03_stats shading_clearcoat_renderer_03_stats_t;

/* Creation and destruction */
int shading_clearcoat_renderer_03_create(shading_clearcoat_renderer_03_t** out_ctx, const shading_clearcoat_renderer_03_desc_t* desc);
int shading_clearcoat_renderer_03_destroy(shading_clearcoat_renderer_03_t* ctx);

/* Core operations */
int shading_clearcoat_renderer_03_render(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_prepare(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_bind(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_draw(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_dispatch(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_submit_commands(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_build_commands(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_sort(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_batch(shading_clearcoat_renderer_03_t* ctx, void* params);
int shading_clearcoat_renderer_03_cull(shading_clearcoat_renderer_03_t* ctx, void* params);

/* Utility functions */
int shading_clearcoat_renderer_03_get_stats(shading_clearcoat_renderer_03_t* ctx);
int shading_clearcoat_renderer_03_set_callback(shading_clearcoat_renderer_03_t* ctx);
int shading_clearcoat_renderer_03_get_memory_usage(shading_clearcoat_renderer_03_t* ctx);
int shading_clearcoat_renderer_03_optimize(shading_clearcoat_renderer_03_t* ctx);
int shading_clearcoat_renderer_03_debug_print(shading_clearcoat_renderer_03_t* ctx);

/* Module functions */
int shading_clearcoat_renderer_03_module_init(void);
int shading_clearcoat_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CLEARCOAT_RENDERER_03_H */
