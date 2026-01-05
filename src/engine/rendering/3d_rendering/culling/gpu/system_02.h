/*
 * culling_gpu_system_02.h
 *
 * Header file for culling_gpu_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_GPU_SYSTEM_02_H
#define CULLING_GPU_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_gpu_system_02 culling_gpu_system_02_t;
typedef struct culling_gpu_system_02_desc culling_gpu_system_02_desc_t;
typedef struct culling_gpu_system_02_stats culling_gpu_system_02_stats_t;

/* Creation and destruction */
int culling_gpu_system_02_create(culling_gpu_system_02_t** out_ctx, const culling_gpu_system_02_desc_t* desc);
int culling_gpu_system_02_destroy(culling_gpu_system_02_t* ctx);

/* Core operations */
int culling_gpu_system_02_create_system(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_destroy_system(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_tick(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_process(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_submit(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_execute(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_sync(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_query(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_configure(culling_gpu_system_02_t* ctx, void* params);
int culling_gpu_system_02_optimize(culling_gpu_system_02_t* ctx, void* params);

/* Utility functions */
int culling_gpu_system_02_get_stats(culling_gpu_system_02_t* ctx);
int culling_gpu_system_02_set_callback(culling_gpu_system_02_t* ctx);
int culling_gpu_system_02_get_memory_usage(culling_gpu_system_02_t* ctx);
int culling_gpu_system_02_optimize(culling_gpu_system_02_t* ctx);
int culling_gpu_system_02_debug_print(culling_gpu_system_02_t* ctx);

/* Module functions */
int culling_gpu_system_02_module_init(void);
int culling_gpu_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_GPU_SYSTEM_02_H */
