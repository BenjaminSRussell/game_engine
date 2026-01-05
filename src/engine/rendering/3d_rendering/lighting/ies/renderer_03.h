/*
 * lighting_ies_renderer_03.h
 *
 * Header file for lighting_ies_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_IES_RENDERER_03_H
#define LIGHTING_IES_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_ies_renderer_03 lighting_ies_renderer_03_t;
typedef struct lighting_ies_renderer_03_desc lighting_ies_renderer_03_desc_t;
typedef struct lighting_ies_renderer_03_stats lighting_ies_renderer_03_stats_t;

/* Creation and destruction */
int lighting_ies_renderer_03_create(lighting_ies_renderer_03_t** out_ctx, const lighting_ies_renderer_03_desc_t* desc);
int lighting_ies_renderer_03_destroy(lighting_ies_renderer_03_t* ctx);

/* Core operations */
int lighting_ies_renderer_03_render(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_prepare(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_bind(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_draw(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_dispatch(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_submit_commands(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_build_commands(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_sort(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_batch(lighting_ies_renderer_03_t* ctx, void* params);
int lighting_ies_renderer_03_cull(lighting_ies_renderer_03_t* ctx, void* params);

/* Utility functions */
int lighting_ies_renderer_03_get_stats(lighting_ies_renderer_03_t* ctx);
int lighting_ies_renderer_03_set_callback(lighting_ies_renderer_03_t* ctx);
int lighting_ies_renderer_03_get_memory_usage(lighting_ies_renderer_03_t* ctx);
int lighting_ies_renderer_03_optimize(lighting_ies_renderer_03_t* ctx);
int lighting_ies_renderer_03_debug_print(lighting_ies_renderer_03_t* ctx);

/* Module functions */
int lighting_ies_renderer_03_module_init(void);
int lighting_ies_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_IES_RENDERER_03_H */
