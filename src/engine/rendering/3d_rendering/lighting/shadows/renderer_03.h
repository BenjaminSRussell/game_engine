/*
 * lighting_shadows_renderer_03.h
 *
 * Header file for lighting_shadows_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SHADOWS_RENDERER_03_H
#define LIGHTING_SHADOWS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_shadows_renderer_03 lighting_shadows_renderer_03_t;
typedef struct lighting_shadows_renderer_03_desc lighting_shadows_renderer_03_desc_t;
typedef struct lighting_shadows_renderer_03_stats lighting_shadows_renderer_03_stats_t;

/* Creation and destruction */
int lighting_shadows_renderer_03_create(lighting_shadows_renderer_03_t** out_ctx, const lighting_shadows_renderer_03_desc_t* desc);
int lighting_shadows_renderer_03_destroy(lighting_shadows_renderer_03_t* ctx);

/* Core operations */
int lighting_shadows_renderer_03_render(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_prepare(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_bind(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_draw(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_dispatch(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_submit_commands(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_build_commands(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_sort(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_batch(lighting_shadows_renderer_03_t* ctx, void* params);
int lighting_shadows_renderer_03_cull(lighting_shadows_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_shadows_renderer_03_get_stats(lighting_shadows_renderer_03_t* ctx);
int lighting_shadows_renderer_03_set_callback(lighting_shadows_renderer_03_t* ctx);
int lighting_shadows_renderer_03_get_memory_usage(lighting_shadows_renderer_03_t* ctx);
int lighting_shadows_renderer_03_optimize(lighting_shadows_renderer_03_t* ctx);
int lighting_shadows_renderer_03_debug_print(lighting_shadows_renderer_03_t* ctx);

/* Module functions */
int lighting_shadows_renderer_03_module_init(void);
int lighting_shadows_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SHADOWS_RENDERER_03_H */
