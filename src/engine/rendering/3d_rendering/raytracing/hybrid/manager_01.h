/*
 * raytracing_hybrid_manager_01.h
 *
 * Header file for raytracing_hybrid_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_HYBRID_MANAGER_01_H
#define RAYTRACING_HYBRID_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_hybrid_manager_01 raytracing_hybrid_manager_01_t;
typedef struct raytracing_hybrid_manager_01_desc raytracing_hybrid_manager_01_desc_t;
typedef struct raytracing_hybrid_manager_01_stats raytracing_hybrid_manager_01_stats_t;

/* Creation and destruction */
int raytracing_hybrid_manager_01_create(raytracing_hybrid_manager_01_t** out_ctx, const raytracing_hybrid_manager_01_desc_t* desc);
int raytracing_hybrid_manager_01_destroy(raytracing_hybrid_manager_01_t* ctx);

/* Core operations */
int raytracing_hybrid_manager_01_init(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_shutdown(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_update(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_create(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_destroy(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_get(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_set(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_reset(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_validate(raytracing_hybrid_manager_01_t* ctx, void* params);
int raytracing_hybrid_manager_01_flush(raytracing_hybrid_manager_01_t* ctx, void* params);

/* Utility functions */
int raytracing_hybrid_manager_01_get_stats(raytracing_hybrid_manager_01_t* ctx);
int raytracing_hybrid_manager_01_set_callback(raytracing_hybrid_manager_01_t* ctx);
int raytracing_hybrid_manager_01_get_memory_usage(raytracing_hybrid_manager_01_t* ctx);
int raytracing_hybrid_manager_01_optimize(raytracing_hybrid_manager_01_t* ctx);
int raytracing_hybrid_manager_01_debug_print(raytracing_hybrid_manager_01_t* ctx);

/* Module functions */
int raytracing_hybrid_manager_01_module_init(void);
int raytracing_hybrid_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_HYBRID_MANAGER_01_H */
