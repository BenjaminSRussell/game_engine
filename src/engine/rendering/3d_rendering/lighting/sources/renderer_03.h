/*
 * lighting_sources_renderer_03.h
 *
 * Header file for lighting_sources_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_SOURCES_RENDERER_03_H
#define LIGHTING_SOURCES_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_sources_renderer_03 lighting_sources_renderer_03_t;
typedef struct lighting_sources_renderer_03_desc lighting_sources_renderer_03_desc_t;
typedef struct lighting_sources_renderer_03_stats lighting_sources_renderer_03_stats_t;

/* Creation and destruction */
int lighting_sources_renderer_03_create(lighting_sources_renderer_03_t** out_ctx, const lighting_sources_renderer_03_desc_t* desc);
int lighting_sources_renderer_03_destroy(lighting_sources_renderer_03_t* ctx);

/* Core operations */
int lighting_sources_renderer_03_render(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_prepare(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_bind(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_draw(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_dispatch(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_submit_commands(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_build_commands(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_sort(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_batch(lighting_sources_renderer_03_t* ctx, void* params);
int lighting_sources_renderer_03_cull(lighting_sources_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_sources_renderer_03_get_stats(lighting_sources_renderer_03_t* ctx);
int lighting_sources_renderer_03_set_callback(lighting_sources_renderer_03_t* ctx);
int lighting_sources_renderer_03_get_memory_usage(lighting_sources_renderer_03_t* ctx);
int lighting_sources_renderer_03_optimize(lighting_sources_renderer_03_t* ctx);
int lighting_sources_renderer_03_debug_print(lighting_sources_renderer_03_t* ctx);

/* Module functions */
int lighting_sources_renderer_03_module_init(void);
int lighting_sources_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SOURCES_RENDERER_03_H */
