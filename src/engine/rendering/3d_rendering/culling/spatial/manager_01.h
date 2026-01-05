/*
 * culling_spatial_manager_01.h
 *
 * Header file for culling_spatial_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_SPATIAL_MANAGER_01_H
#define CULLING_SPATIAL_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_spatial_manager_01 culling_spatial_manager_01_t;
typedef struct culling_spatial_manager_01_desc culling_spatial_manager_01_desc_t;
typedef struct culling_spatial_manager_01_stats culling_spatial_manager_01_stats_t;

/* Creation and destruction */
int culling_spatial_manager_01_create(culling_spatial_manager_01_t** out_ctx, const culling_spatial_manager_01_desc_t* desc);
int culling_spatial_manager_01_destroy(culling_spatial_manager_01_t* ctx);

/* Core operations */
int culling_spatial_manager_01_init(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_shutdown(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_update(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_create(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_destroy(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_get(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_set(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_reset(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_validate(culling_spatial_manager_01_t* ctx, void* params);
int culling_spatial_manager_01_flush(culling_spatial_manager_01_t* ctx, void* params);

/* Utility functions */
int culling_spatial_manager_01_get_stats(culling_spatial_manager_01_t* ctx);
int culling_spatial_manager_01_set_callback(culling_spatial_manager_01_t* ctx);
int culling_spatial_manager_01_get_memory_usage(culling_spatial_manager_01_t* ctx);
int culling_spatial_manager_01_optimize(culling_spatial_manager_01_t* ctx);
int culling_spatial_manager_01_debug_print(culling_spatial_manager_01_t* ctx);

/* Module functions */
int culling_spatial_manager_01_module_init(void);
int culling_spatial_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_SPATIAL_MANAGER_01_H */
