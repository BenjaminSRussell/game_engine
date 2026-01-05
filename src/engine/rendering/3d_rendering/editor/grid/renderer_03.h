/*
 * editor_grid_renderer_03.h
 *
 * Header file for editor_grid_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_GRID_RENDERER_03_H
#define EDITOR_GRID_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_grid_renderer_03 editor_grid_renderer_03_t;
typedef struct editor_grid_renderer_03_desc editor_grid_renderer_03_desc_t;
typedef struct editor_grid_renderer_03_stats editor_grid_renderer_03_stats_t;

/* Creation and destruction */
int editor_grid_renderer_03_create(editor_grid_renderer_03_t** out_ctx, const editor_grid_renderer_03_desc_t* desc);
int editor_grid_renderer_03_destroy(editor_grid_renderer_03_t* ctx);

/* Core operations */
int editor_grid_renderer_03_render(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_prepare(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_bind(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_draw(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_dispatch(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_submit_commands(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_build_commands(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_sort(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_batch(editor_grid_renderer_03_t* ctx, void* params);
int editor_grid_renderer_03_cull(editor_grid_renderer_03_t* ctx, void* params);

/* Utility functions */
int editor_grid_renderer_03_get_stats(editor_grid_renderer_03_t* ctx);
int editor_grid_renderer_03_set_callback(editor_grid_renderer_03_t* ctx);
int editor_grid_renderer_03_get_memory_usage(editor_grid_renderer_03_t* ctx);
int editor_grid_renderer_03_optimize(editor_grid_renderer_03_t* ctx);
int editor_grid_renderer_03_debug_print(editor_grid_renderer_03_t* ctx);

/* Module functions */
int editor_grid_renderer_03_module_init(void);
int editor_grid_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GRID_RENDERER_03_H */
