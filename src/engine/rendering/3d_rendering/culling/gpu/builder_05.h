/*
 * culling_gpu_builder_05.h
 *
 * Header file for culling_gpu_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_GPU_BUILDER_05_H
#define CULLING_GPU_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_gpu_builder_05 culling_gpu_builder_05_t;
typedef struct culling_gpu_builder_05_desc culling_gpu_builder_05_desc_t;
typedef struct culling_gpu_builder_05_stats culling_gpu_builder_05_stats_t;

/* Creation and destruction */
int culling_gpu_builder_05_create(culling_gpu_builder_05_t** out_ctx, const culling_gpu_builder_05_desc_t* desc);
int culling_gpu_builder_05_destroy(culling_gpu_builder_05_t* ctx);

/* Core operations */
int culling_gpu_builder_05_begin(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_end(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_add(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_remove(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_modify(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_finalize(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_validate(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_optimize(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_compile(culling_gpu_builder_05_t* ctx, void* params);
int culling_gpu_builder_05_link(culling_gpu_builder_05_t* ctx, void* params);

/* Utility functions */
int culling_gpu_builder_05_get_stats(culling_gpu_builder_05_t* ctx);
int culling_gpu_builder_05_set_callback(culling_gpu_builder_05_t* ctx);
int culling_gpu_builder_05_get_memory_usage(culling_gpu_builder_05_t* ctx);
int culling_gpu_builder_05_optimize(culling_gpu_builder_05_t* ctx);
int culling_gpu_builder_05_debug_print(culling_gpu_builder_05_t* ctx);

/* Module functions */
int culling_gpu_builder_05_module_init(void);
int culling_gpu_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_GPU_BUILDER_05_H */
