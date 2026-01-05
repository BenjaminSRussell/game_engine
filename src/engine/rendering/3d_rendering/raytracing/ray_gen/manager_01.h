/*
 * raytracing_ray_gen_manager_01.h
 *
 * Header file for raytracing_ray_gen_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RAY_GEN_MANAGER_01_H
#define RAYTRACING_RAY_GEN_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_ray_gen_manager_01 raytracing_ray_gen_manager_01_t;
typedef struct raytracing_ray_gen_manager_01_desc raytracing_ray_gen_manager_01_desc_t;
typedef struct raytracing_ray_gen_manager_01_stats raytracing_ray_gen_manager_01_stats_t;

/* Creation and destruction */
int raytracing_ray_gen_manager_01_create(raytracing_ray_gen_manager_01_t** out_ctx, const raytracing_ray_gen_manager_01_desc_t* desc);
int raytracing_ray_gen_manager_01_destroy(raytracing_ray_gen_manager_01_t* ctx);

/* Core operations */
int raytracing_ray_gen_manager_01_init(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_shutdown(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_update(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_create(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_destroy(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_get(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_set(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_reset(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_validate(raytracing_ray_gen_manager_01_t* ctx, void* params);
int raytracing_ray_gen_manager_01_flush(raytracing_ray_gen_manager_01_t* ctx, void* params);

/* Utility functions */
int raytracing_ray_gen_manager_01_get_stats(raytracing_ray_gen_manager_01_t* ctx);
int raytracing_ray_gen_manager_01_set_callback(raytracing_ray_gen_manager_01_t* ctx);
int raytracing_ray_gen_manager_01_get_memory_usage(raytracing_ray_gen_manager_01_t* ctx);
int raytracing_ray_gen_manager_01_optimize(raytracing_ray_gen_manager_01_t* ctx);
int raytracing_ray_gen_manager_01_debug_print(raytracing_ray_gen_manager_01_t* ctx);

/* Module functions */
int raytracing_ray_gen_manager_01_module_init(void);
int raytracing_ray_gen_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RAY_GEN_MANAGER_01_H */
