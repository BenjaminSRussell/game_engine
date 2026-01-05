/*
 * shading_iridescence_renderer_03.h
 *
 * Header file for shading_iridescence_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_IRIDESCENCE_RENDERER_03_H
#define SHADING_IRIDESCENCE_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_iridescence_renderer_03 shading_iridescence_renderer_03_t;
typedef struct shading_iridescence_renderer_03_desc shading_iridescence_renderer_03_desc_t;
typedef struct shading_iridescence_renderer_03_stats shading_iridescence_renderer_03_stats_t;

/* Creation and destruction */
int shading_iridescence_renderer_03_create(shading_iridescence_renderer_03_t** out_ctx, const shading_iridescence_renderer_03_desc_t* desc);
int shading_iridescence_renderer_03_destroy(shading_iridescence_renderer_03_t* ctx);

/* Core operations */
int shading_iridescence_renderer_03_render(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_prepare(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_bind(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_draw(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_dispatch(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_submit_commands(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_build_commands(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_sort(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_batch(shading_iridescence_renderer_03_t* ctx, void* params);
int shading_iridescence_renderer_03_cull(shading_iridescence_renderer_03_t* ctx, void* params);

/* Utility functions */
int shading_iridescence_renderer_03_get_stats(shading_iridescence_renderer_03_t* ctx);
int shading_iridescence_renderer_03_set_callback(shading_iridescence_renderer_03_t* ctx);
int shading_iridescence_renderer_03_get_memory_usage(shading_iridescence_renderer_03_t* ctx);
int shading_iridescence_renderer_03_optimize(shading_iridescence_renderer_03_t* ctx);
int shading_iridescence_renderer_03_debug_print(shading_iridescence_renderer_03_t* ctx);

/* Module functions */
int shading_iridescence_renderer_03_module_init(void);
int shading_iridescence_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_IRIDESCENCE_RENDERER_03_H */
