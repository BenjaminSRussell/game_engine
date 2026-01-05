/*
 * rendering_gpu_driven_renderer_03.h
 *
 * Header file for rendering_gpu_driven_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GPU_DRIVEN_RENDERER_03_H
#define RENDERING_GPU_DRIVEN_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_gpu_driven_renderer_03 rendering_gpu_driven_renderer_03_t;
typedef struct rendering_gpu_driven_renderer_03_desc rendering_gpu_driven_renderer_03_desc_t;
typedef struct rendering_gpu_driven_renderer_03_stats rendering_gpu_driven_renderer_03_stats_t;

/* Creation and destruction */
int rendering_gpu_driven_renderer_03_create(rendering_gpu_driven_renderer_03_t** out_ctx, const rendering_gpu_driven_renderer_03_desc_t* desc);
int rendering_gpu_driven_renderer_03_destroy(rendering_gpu_driven_renderer_03_t* ctx);

/* Core operations */
int rendering_gpu_driven_renderer_03_render(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_prepare(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_bind(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_draw(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_dispatch(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_submit_commands(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_build_commands(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_sort(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_batch(rendering_gpu_driven_renderer_03_t* ctx, void* params);
int rendering_gpu_driven_renderer_03_cull(rendering_gpu_driven_renderer_03_t* ctx, void* params);

/* Utility functions */
int rendering_gpu_driven_renderer_03_get_stats(rendering_gpu_driven_renderer_03_t* ctx);
int rendering_gpu_driven_renderer_03_set_callback(rendering_gpu_driven_renderer_03_t* ctx);
int rendering_gpu_driven_renderer_03_get_memory_usage(rendering_gpu_driven_renderer_03_t* ctx);
int rendering_gpu_driven_renderer_03_optimize(rendering_gpu_driven_renderer_03_t* ctx);
int rendering_gpu_driven_renderer_03_debug_print(rendering_gpu_driven_renderer_03_t* ctx);

/* Module functions */
int rendering_gpu_driven_renderer_03_module_init(void);
int rendering_gpu_driven_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_DRIVEN_RENDERER_03_H */
