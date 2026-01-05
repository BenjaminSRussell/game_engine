/*
 * landscape_streaming_renderer_03.h
 *
 * Header file for landscape_streaming_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_STREAMING_RENDERER_03_H
#define LANDSCAPE_STREAMING_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_streaming_renderer_03 landscape_streaming_renderer_03_t;
typedef struct landscape_streaming_renderer_03_desc landscape_streaming_renderer_03_desc_t;
typedef struct landscape_streaming_renderer_03_stats landscape_streaming_renderer_03_stats_t;

/* Creation and destruction */
int landscape_streaming_renderer_03_create(landscape_streaming_renderer_03_t** out_ctx, const landscape_streaming_renderer_03_desc_t* desc);
int landscape_streaming_renderer_03_destroy(landscape_streaming_renderer_03_t* ctx);

/* Core operations */
int landscape_streaming_renderer_03_render(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_prepare(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_bind(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_draw(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_dispatch(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_submit_commands(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_build_commands(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_sort(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_batch(landscape_streaming_renderer_03_t* ctx, void* params);
int landscape_streaming_renderer_03_cull(landscape_streaming_renderer_03_t* ctx, void* params);

/* Utility functions */
int landscape_streaming_renderer_03_get_stats(landscape_streaming_renderer_03_t* ctx);
int landscape_streaming_renderer_03_set_callback(landscape_streaming_renderer_03_t* ctx);
int landscape_streaming_renderer_03_get_memory_usage(landscape_streaming_renderer_03_t* ctx);
int landscape_streaming_renderer_03_optimize(landscape_streaming_renderer_03_t* ctx);
int landscape_streaming_renderer_03_debug_print(landscape_streaming_renderer_03_t* ctx);

/* Module functions */
int landscape_streaming_renderer_03_module_init(void);
int landscape_streaming_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_STREAMING_RENDERER_03_H */
