/*
 * effects_fog_renderer_03.h
 *
 * Header file for effects_fog_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_FOG_RENDERER_03_H
#define EFFECTS_FOG_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_fog_renderer_03 effects_fog_renderer_03_t;
typedef struct effects_fog_renderer_03_desc effects_fog_renderer_03_desc_t;
typedef struct effects_fog_renderer_03_stats effects_fog_renderer_03_stats_t;

/* Creation and destruction */
int effects_fog_renderer_03_create(effects_fog_renderer_03_t** out_ctx, const effects_fog_renderer_03_desc_t* desc);
int effects_fog_renderer_03_destroy(effects_fog_renderer_03_t* ctx);

/* Core operations */
int effects_fog_renderer_03_render(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_prepare(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_bind(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_draw(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_dispatch(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_submit_commands(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_build_commands(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_sort(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_batch(effects_fog_renderer_03_t* ctx, void* params);
int effects_fog_renderer_03_cull(effects_fog_renderer_03_t* ctx, void* params);

/* Utility functions */
int effects_fog_renderer_03_get_stats(effects_fog_renderer_03_t* ctx);
int effects_fog_renderer_03_set_callback(effects_fog_renderer_03_t* ctx);
int effects_fog_renderer_03_get_memory_usage(effects_fog_renderer_03_t* ctx);
int effects_fog_renderer_03_optimize(effects_fog_renderer_03_t* ctx);
int effects_fog_renderer_03_debug_print(effects_fog_renderer_03_t* ctx);

/* Module functions */
int effects_fog_renderer_03_module_init(void);
int effects_fog_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_FOG_RENDERER_03_H */
