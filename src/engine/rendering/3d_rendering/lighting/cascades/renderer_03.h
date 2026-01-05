/*
 * lighting_cascades_renderer_03.h
 *
 * Header file for lighting_cascades_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADES_RENDERER_03_H
#define LIGHTING_CASCADES_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_cascades_renderer_03 lighting_cascades_renderer_03_t;
typedef struct lighting_cascades_renderer_03_desc lighting_cascades_renderer_03_desc_t;
typedef struct lighting_cascades_renderer_03_stats lighting_cascades_renderer_03_stats_t;

/* Creation and destruction */
int lighting_cascades_renderer_03_create(lighting_cascades_renderer_03_t** out_ctx, const lighting_cascades_renderer_03_desc_t* desc);
int lighting_cascades_renderer_03_destroy(lighting_cascades_renderer_03_t* ctx);

/* Core operations */
int lighting_cascades_renderer_03_render(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_prepare(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_bind(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_draw(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_dispatch(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_submit_commands(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_build_commands(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_sort(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_batch(lighting_cascades_renderer_03_t* ctx, void* params);
int lighting_cascades_renderer_03_cull(lighting_cascades_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_cascades_renderer_03_get_stats(lighting_cascades_renderer_03_t* ctx);
int lighting_cascades_renderer_03_set_callback(lighting_cascades_renderer_03_t* ctx);
int lighting_cascades_renderer_03_get_memory_usage(lighting_cascades_renderer_03_t* ctx);
int lighting_cascades_renderer_03_optimize(lighting_cascades_renderer_03_t* ctx);
int lighting_cascades_renderer_03_debug_print(lighting_cascades_renderer_03_t* ctx);

/* Module functions */
int lighting_cascades_renderer_03_module_init(void);
int lighting_cascades_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADES_RENDERER_03_H */
