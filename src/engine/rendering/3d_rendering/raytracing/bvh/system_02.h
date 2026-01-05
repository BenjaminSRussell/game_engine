/*
 * raytracing_bvh_system_02.h
 *
 * Header file for raytracing_bvh_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_BVH_SYSTEM_02_H
#define RAYTRACING_BVH_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_bvh_system_02 raytracing_bvh_system_02_t;
typedef struct raytracing_bvh_system_02_desc raytracing_bvh_system_02_desc_t;
typedef struct raytracing_bvh_system_02_stats raytracing_bvh_system_02_stats_t;

/* Creation and destruction */
int raytracing_bvh_system_02_create(raytracing_bvh_system_02_t** out_ctx, const raytracing_bvh_system_02_desc_t* desc);
int raytracing_bvh_system_02_destroy(raytracing_bvh_system_02_t* ctx);

/* Core operations */
int raytracing_bvh_system_02_create_system(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_destroy_system(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_tick(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_process(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_submit(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_execute(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_sync(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_query(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_configure(raytracing_bvh_system_02_t* ctx, void* params);
int raytracing_bvh_system_02_optimize(raytracing_bvh_system_02_t* ctx, void* params);

/* Utility functions */
int raytracing_bvh_system_02_get_stats(raytracing_bvh_system_02_t* ctx);
int raytracing_bvh_system_02_set_callback(raytracing_bvh_system_02_t* ctx);
int raytracing_bvh_system_02_get_memory_usage(raytracing_bvh_system_02_t* ctx);
int raytracing_bvh_system_02_optimize(raytracing_bvh_system_02_t* ctx);
int raytracing_bvh_system_02_debug_print(raytracing_bvh_system_02_t* ctx);

/* Module functions */
int raytracing_bvh_system_02_module_init(void);
int raytracing_bvh_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_BVH_SYSTEM_02_H */
