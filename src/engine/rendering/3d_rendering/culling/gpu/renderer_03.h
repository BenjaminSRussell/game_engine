/*
 * culling_gpu_renderer_03.h
 *
 * Header file for culling_gpu_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_GPU_RENDERER_03_H
#define CULLING_GPU_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_gpu_renderer_03 culling_gpu_renderer_03_t;
typedef struct culling_gpu_renderer_03_desc culling_gpu_renderer_03_desc_t;
typedef struct culling_gpu_renderer_03_stats culling_gpu_renderer_03_stats_t;

/* Creation and destruction */
int culling_gpu_renderer_03_create(culling_gpu_renderer_03_t** out_ctx, const culling_gpu_renderer_03_desc_t* desc);
int culling_gpu_renderer_03_destroy(culling_gpu_renderer_03_t* ctx);

/* Core operations */
int culling_gpu_renderer_03_render(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_prepare(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_bind(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_draw(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_dispatch(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_submit_commands(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_build_commands(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_sort(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_batch(culling_gpu_renderer_03_t* ctx, void* params);
int culling_gpu_renderer_03_cull(culling_gpu_renderer_03_t* ctx, void* params);

/* Utility functions */
int culling_gpu_renderer_03_get_stats(culling_gpu_renderer_03_t* ctx);
int culling_gpu_renderer_03_set_callback(culling_gpu_renderer_03_t* ctx);
int culling_gpu_renderer_03_get_memory_usage(culling_gpu_renderer_03_t* ctx);
int culling_gpu_renderer_03_optimize(culling_gpu_renderer_03_t* ctx);
int culling_gpu_renderer_03_debug_print(culling_gpu_renderer_03_t* ctx);

/* Module functions */
int culling_gpu_renderer_03_module_init(void);
int culling_gpu_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_GPU_RENDERER_03_H */
