/*
 * shading_specular_renderer_03.h
 *
 * Header file for shading_specular_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_SPECULAR_RENDERER_03_H
#define SHADING_SPECULAR_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_specular_renderer_03 shading_specular_renderer_03_t;
typedef struct shading_specular_renderer_03_desc shading_specular_renderer_03_desc_t;
typedef struct shading_specular_renderer_03_stats shading_specular_renderer_03_stats_t;

/* Creation and destruction */
int shading_specular_renderer_03_create(shading_specular_renderer_03_t** out_ctx, const shading_specular_renderer_03_desc_t* desc);
int shading_specular_renderer_03_destroy(shading_specular_renderer_03_t* ctx);

/* Core operations */
int shading_specular_renderer_03_render(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_prepare(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_bind(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_draw(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_dispatch(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_submit_commands(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_build_commands(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_sort(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_batch(shading_specular_renderer_03_t* ctx, void* params);
int shading_specular_renderer_03_cull(shading_specular_renderer_03_t* ctx, void* params);

/* Utility functions */
int shading_specular_renderer_03_get_stats(shading_specular_renderer_03_t* ctx);
int shading_specular_renderer_03_set_callback(shading_specular_renderer_03_t* ctx);
int shading_specular_renderer_03_get_memory_usage(shading_specular_renderer_03_t* ctx);
int shading_specular_renderer_03_optimize(shading_specular_renderer_03_t* ctx);
int shading_specular_renderer_03_debug_print(shading_specular_renderer_03_t* ctx);

/* Module functions */
int shading_specular_renderer_03_module_init(void);
int shading_specular_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SPECULAR_RENDERER_03_H */
