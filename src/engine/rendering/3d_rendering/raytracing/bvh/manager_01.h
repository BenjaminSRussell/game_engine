/*
 * raytracing_bvh_manager_01.h
 *
 * Header file for raytracing_bvh_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_BVH_MANAGER_01_H
#define RAYTRACING_BVH_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_bvh_manager_01 raytracing_bvh_manager_01_t;
typedef struct raytracing_bvh_manager_01_desc raytracing_bvh_manager_01_desc_t;
typedef struct raytracing_bvh_manager_01_stats raytracing_bvh_manager_01_stats_t;

/* Creation and destruction */
int raytracing_bvh_manager_01_create(raytracing_bvh_manager_01_t** out_ctx, const raytracing_bvh_manager_01_desc_t* desc);
int raytracing_bvh_manager_01_destroy(raytracing_bvh_manager_01_t* ctx);

/* Core operations */
int raytracing_bvh_manager_01_init(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_shutdown(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_update(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_create(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_destroy(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_get(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_set(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_reset(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_validate(raytracing_bvh_manager_01_t* ctx, void* params);
int raytracing_bvh_manager_01_flush(raytracing_bvh_manager_01_t* ctx, void* params);

/* Utility functions */
int raytracing_bvh_manager_01_get_stats(raytracing_bvh_manager_01_t* ctx);
int raytracing_bvh_manager_01_set_callback(raytracing_bvh_manager_01_t* ctx);
int raytracing_bvh_manager_01_get_memory_usage(raytracing_bvh_manager_01_t* ctx);
int raytracing_bvh_manager_01_optimize(raytracing_bvh_manager_01_t* ctx);
int raytracing_bvh_manager_01_debug_print(raytracing_bvh_manager_01_t* ctx);

/* Module functions */
int raytracing_bvh_manager_01_module_init(void);
int raytracing_bvh_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_BVH_MANAGER_01_H */
