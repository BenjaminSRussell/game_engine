/*
 * culling_prediction_renderer_03.h
 *
 * Header file for culling_prediction_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PREDICTION_RENDERER_03_H
#define CULLING_PREDICTION_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_prediction_renderer_03 culling_prediction_renderer_03_t;
typedef struct culling_prediction_renderer_03_desc culling_prediction_renderer_03_desc_t;
typedef struct culling_prediction_renderer_03_stats culling_prediction_renderer_03_stats_t;

/* Creation and destruction */
int culling_prediction_renderer_03_create(culling_prediction_renderer_03_t** out_ctx, const culling_prediction_renderer_03_desc_t* desc);
int culling_prediction_renderer_03_destroy(culling_prediction_renderer_03_t* ctx);

/* Core operations */
int culling_prediction_renderer_03_render(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_prepare(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_bind(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_draw(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_dispatch(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_submit_commands(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_build_commands(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_sort(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_batch(culling_prediction_renderer_03_t* ctx, void* params);
int culling_prediction_renderer_03_cull(culling_prediction_renderer_03_t* ctx, void* params);

/* Utility functions */
int culling_prediction_renderer_03_get_stats(culling_prediction_renderer_03_t* ctx);
int culling_prediction_renderer_03_set_callback(culling_prediction_renderer_03_t* ctx);
int culling_prediction_renderer_03_get_memory_usage(culling_prediction_renderer_03_t* ctx);
int culling_prediction_renderer_03_optimize(culling_prediction_renderer_03_t* ctx);
int culling_prediction_renderer_03_debug_print(culling_prediction_renderer_03_t* ctx);

/* Module functions */
int culling_prediction_renderer_03_module_init(void);
int culling_prediction_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PREDICTION_RENDERER_03_H */
