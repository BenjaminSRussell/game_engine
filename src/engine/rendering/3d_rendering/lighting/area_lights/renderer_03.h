/*
 * lighting_area_lights_renderer_03.h
 *
 * Header file for lighting_area_lights_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_AREA_LIGHTS_RENDERER_03_H
#define LIGHTING_AREA_LIGHTS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_area_lights_renderer_03 lighting_area_lights_renderer_03_t;
typedef struct lighting_area_lights_renderer_03_desc lighting_area_lights_renderer_03_desc_t;
typedef struct lighting_area_lights_renderer_03_stats lighting_area_lights_renderer_03_stats_t;

/* Creation and destruction */
int lighting_area_lights_renderer_03_create(lighting_area_lights_renderer_03_t** out_ctx, const lighting_area_lights_renderer_03_desc_t* desc);
int lighting_area_lights_renderer_03_destroy(lighting_area_lights_renderer_03_t* ctx);

/* Core operations */
int lighting_area_lights_renderer_03_render(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_prepare(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_bind(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_draw(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_dispatch(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_submit_commands(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_build_commands(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_sort(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_batch(lighting_area_lights_renderer_03_t* ctx, void* params);
int lighting_area_lights_renderer_03_cull(lighting_area_lights_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_area_lights_renderer_03_get_stats(lighting_area_lights_renderer_03_t* ctx);
int lighting_area_lights_renderer_03_set_callback(lighting_area_lights_renderer_03_t* ctx);
int lighting_area_lights_renderer_03_get_memory_usage(lighting_area_lights_renderer_03_t* ctx);
int lighting_area_lights_renderer_03_optimize(lighting_area_lights_renderer_03_t* ctx);
int lighting_area_lights_renderer_03_debug_print(lighting_area_lights_renderer_03_t* ctx);

/* Module functions */
int lighting_area_lights_renderer_03_module_init(void);
int lighting_area_lights_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_AREA_LIGHTS_RENDERER_03_H */
