/*
 * effects_vfx_renderer_03.h
 *
 * Header file for effects_vfx_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_RENDERER_03_H
#define EFFECTS_VFX_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_vfx_renderer_03 effects_vfx_renderer_03_t;
typedef struct effects_vfx_renderer_03_desc effects_vfx_renderer_03_desc_t;
typedef struct effects_vfx_renderer_03_stats effects_vfx_renderer_03_stats_t;

/* Creation and destruction */
int effects_vfx_renderer_03_create(effects_vfx_renderer_03_t** out_ctx, const effects_vfx_renderer_03_desc_t* desc);
int effects_vfx_renderer_03_destroy(effects_vfx_renderer_03_t* ctx);

/* Core operations */
int effects_vfx_renderer_03_render(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_prepare(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_bind(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_draw(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_dispatch(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_submit_commands(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_build_commands(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_sort(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_batch(effects_vfx_renderer_03_t* ctx, void* params);
int effects_vfx_renderer_03_cull(effects_vfx_renderer_03_t* ctx, void* params);

/* Utility functions */
int effects_vfx_renderer_03_get_stats(effects_vfx_renderer_03_t* ctx);
int effects_vfx_renderer_03_set_callback(effects_vfx_renderer_03_t* ctx);
int effects_vfx_renderer_03_get_memory_usage(effects_vfx_renderer_03_t* ctx);
int effects_vfx_renderer_03_optimize(effects_vfx_renderer_03_t* ctx);
int effects_vfx_renderer_03_debug_print(effects_vfx_renderer_03_t* ctx);

/* Module functions */
int effects_vfx_renderer_03_module_init(void);
int effects_vfx_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_RENDERER_03_H */
