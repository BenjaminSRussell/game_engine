/*
 * materials_layered_renderer_03.h
 *
 * Header file for materials_layered_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_LAYERED_RENDERER_03_H
#define MATERIALS_LAYERED_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_layered_renderer_03 materials_layered_renderer_03_t;
typedef struct materials_layered_renderer_03_desc materials_layered_renderer_03_desc_t;
typedef struct materials_layered_renderer_03_stats materials_layered_renderer_03_stats_t;

/* Creation and destruction */
int materials_layered_renderer_03_create(materials_layered_renderer_03_t** out_ctx, const materials_layered_renderer_03_desc_t* desc);
int materials_layered_renderer_03_destroy(materials_layered_renderer_03_t* ctx);

/* Core operations */
int materials_layered_renderer_03_render(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_prepare(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_bind(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_draw(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_dispatch(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_submit_commands(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_build_commands(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_sort(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_batch(materials_layered_renderer_03_t* ctx, void* params);
int materials_layered_renderer_03_cull(materials_layered_renderer_03_t* ctx, void* params);

/* Utility functions */
int materials_layered_renderer_03_get_stats(materials_layered_renderer_03_t* ctx);
int materials_layered_renderer_03_set_callback(materials_layered_renderer_03_t* ctx);
int materials_layered_renderer_03_get_memory_usage(materials_layered_renderer_03_t* ctx);
int materials_layered_renderer_03_optimize(materials_layered_renderer_03_t* ctx);
int materials_layered_renderer_03_debug_print(materials_layered_renderer_03_t* ctx);

/* Module functions */
int materials_layered_renderer_03_module_init(void);
int materials_layered_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_LAYERED_RENDERER_03_H */
