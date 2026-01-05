/*
 * rendering_gpu_driven_builder_05.h
 *
 * Header file for rendering_gpu_driven_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GPU_DRIVEN_BUILDER_05_H
#define RENDERING_GPU_DRIVEN_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_gpu_driven_builder_05 rendering_gpu_driven_builder_05_t;
typedef struct rendering_gpu_driven_builder_05_desc rendering_gpu_driven_builder_05_desc_t;
typedef struct rendering_gpu_driven_builder_05_stats rendering_gpu_driven_builder_05_stats_t;

/* Creation and destruction */
int rendering_gpu_driven_builder_05_create(rendering_gpu_driven_builder_05_t** out_ctx, const rendering_gpu_driven_builder_05_desc_t* desc);
int rendering_gpu_driven_builder_05_destroy(rendering_gpu_driven_builder_05_t* ctx);

/* Core operations */
int rendering_gpu_driven_builder_05_begin(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_end(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_add(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_remove(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_modify(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_finalize(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_validate(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_optimize(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_compile(rendering_gpu_driven_builder_05_t* ctx, void* params);
int rendering_gpu_driven_builder_05_link(rendering_gpu_driven_builder_05_t* ctx, void* params);

/* Utility functions */
int rendering_gpu_driven_builder_05_get_stats(rendering_gpu_driven_builder_05_t* ctx);
int rendering_gpu_driven_builder_05_set_callback(rendering_gpu_driven_builder_05_t* ctx);
int rendering_gpu_driven_builder_05_get_memory_usage(rendering_gpu_driven_builder_05_t* ctx);
int rendering_gpu_driven_builder_05_optimize(rendering_gpu_driven_builder_05_t* ctx);
int rendering_gpu_driven_builder_05_debug_print(rendering_gpu_driven_builder_05_t* ctx);

/* Module functions */
int rendering_gpu_driven_builder_05_module_init(void);
int rendering_gpu_driven_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_DRIVEN_BUILDER_05_H */
