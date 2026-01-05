/*
 * editor_handles_renderer_03.h
 *
 * Header file for editor_handles_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_HANDLES_RENDERER_03_H
#define EDITOR_HANDLES_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_handles_renderer_03 editor_handles_renderer_03_t;
typedef struct editor_handles_renderer_03_desc editor_handles_renderer_03_desc_t;
typedef struct editor_handles_renderer_03_stats editor_handles_renderer_03_stats_t;

/* Creation and destruction */
int editor_handles_renderer_03_create(editor_handles_renderer_03_t** out_ctx, const editor_handles_renderer_03_desc_t* desc);
int editor_handles_renderer_03_destroy(editor_handles_renderer_03_t* ctx);

/* Core operations */
int editor_handles_renderer_03_render(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_prepare(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_bind(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_draw(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_dispatch(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_submit_commands(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_build_commands(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_sort(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_batch(editor_handles_renderer_03_t* ctx, void* params);
int editor_handles_renderer_03_cull(editor_handles_renderer_03_t* ctx, void* params);

/* Utility functions */
int editor_handles_renderer_03_get_stats(editor_handles_renderer_03_t* ctx);
int editor_handles_renderer_03_set_callback(editor_handles_renderer_03_t* ctx);
int editor_handles_renderer_03_get_memory_usage(editor_handles_renderer_03_t* ctx);
int editor_handles_renderer_03_optimize(editor_handles_renderer_03_t* ctx);
int editor_handles_renderer_03_debug_print(editor_handles_renderer_03_t* ctx);

/* Module functions */
int editor_handles_renderer_03_module_init(void);
int editor_handles_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_HANDLES_RENDERER_03_H */
