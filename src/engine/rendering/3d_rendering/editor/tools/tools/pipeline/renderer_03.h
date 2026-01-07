/*
 * tools_pipeline_renderer_03.h
 *
 * Header file for tools_pipeline_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_PIPELINE_RENDERER_03_H
#define TOOLS_PIPELINE_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_pipeline_renderer_03 tools_pipeline_renderer_03_t;
typedef struct tools_pipeline_renderer_03_desc tools_pipeline_renderer_03_desc_t;
typedef struct tools_pipeline_renderer_03_stats tools_pipeline_renderer_03_stats_t;

/* Creation and destruction */
int tools_pipeline_renderer_03_create(tools_pipeline_renderer_03_t** out_ctx, const tools_pipeline_renderer_03_desc_t* desc);
int tools_pipeline_renderer_03_destroy(tools_pipeline_renderer_03_t* ctx);

/* Core operations */
int tools_pipeline_renderer_03_render(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_prepare(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_bind(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_draw(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_dispatch(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_submit_commands(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_build_commands(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_sort(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_batch(tools_pipeline_renderer_03_t* ctx, void* params);
int tools_pipeline_renderer_03_cull(tools_pipeline_renderer_03_t* ctx, void* params);

/* Utility functions */
int tools_pipeline_renderer_03_get_stats(tools_pipeline_renderer_03_t* ctx);
int tools_pipeline_renderer_03_set_callback(tools_pipeline_renderer_03_t* ctx);
int tools_pipeline_renderer_03_get_memory_usage(tools_pipeline_renderer_03_t* ctx);
int tools_pipeline_renderer_03_optimize(tools_pipeline_renderer_03_t* ctx);
int tools_pipeline_renderer_03_debug_print(tools_pipeline_renderer_03_t* ctx);

/* Module functions */
int tools_pipeline_renderer_03_module_init(void);
int tools_pipeline_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_PIPELINE_RENDERER_03_H */
