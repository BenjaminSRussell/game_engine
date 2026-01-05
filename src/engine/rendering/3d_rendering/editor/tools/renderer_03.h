/*
 * editor_tools_renderer_03.h
 *
 * Header file for editor_tools_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EDITOR_TOOLS_RENDERER_03_H
#define EDITOR_TOOLS_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct editor_tools_renderer_03 editor_tools_renderer_03_t;
typedef struct editor_tools_renderer_03_desc editor_tools_renderer_03_desc_t;
typedef struct editor_tools_renderer_03_stats editor_tools_renderer_03_stats_t;

/* Creation and destruction */
int editor_tools_renderer_03_create(editor_tools_renderer_03_t** out_ctx, const editor_tools_renderer_03_desc_t* desc);
int editor_tools_renderer_03_destroy(editor_tools_renderer_03_t* ctx);

/* Core operations */
int editor_tools_renderer_03_render(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_prepare(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_bind(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_draw(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_dispatch(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_submit_commands(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_build_commands(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_sort(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_batch(editor_tools_renderer_03_t* ctx, void* params);
int editor_tools_renderer_03_cull(editor_tools_renderer_03_t* ctx, void* params);

/* Utility functions */
int editor_tools_renderer_03_get_stats(editor_tools_renderer_03_t* ctx);
int editor_tools_renderer_03_set_callback(editor_tools_renderer_03_t* ctx);
int editor_tools_renderer_03_get_memory_usage(editor_tools_renderer_03_t* ctx);
int editor_tools_renderer_03_optimize(editor_tools_renderer_03_t* ctx);
int editor_tools_renderer_03_debug_print(editor_tools_renderer_03_t* ctx);

/* Module functions */
int editor_tools_renderer_03_module_init(void);
int editor_tools_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_TOOLS_RENDERER_03_H */
