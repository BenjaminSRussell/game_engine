/*
 * shading_pbr_manager_01.h
 *
 * Header file for shading_pbr_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_PBR_MANAGER_01_H
#define SHADING_PBR_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_pbr_manager_01 shading_pbr_manager_01_t;
typedef struct shading_pbr_manager_01_desc shading_pbr_manager_01_desc_t;
typedef struct shading_pbr_manager_01_stats shading_pbr_manager_01_stats_t;

/* Creation and destruction */
int shading_pbr_manager_01_create(shading_pbr_manager_01_t** out_ctx, const shading_pbr_manager_01_desc_t* desc);
int shading_pbr_manager_01_destroy(shading_pbr_manager_01_t* ctx);

/* Core operations */
int shading_pbr_manager_01_init(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_shutdown(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_update(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_create(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_destroy(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_get(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_set(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_reset(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_validate(shading_pbr_manager_01_t* ctx, void* params);
int shading_pbr_manager_01_flush(shading_pbr_manager_01_t* ctx, void* params);

/* Utility functions */
int shading_pbr_manager_01_get_stats(shading_pbr_manager_01_t* ctx);
int shading_pbr_manager_01_set_callback(shading_pbr_manager_01_t* ctx);
int shading_pbr_manager_01_get_memory_usage(shading_pbr_manager_01_t* ctx);
int shading_pbr_manager_01_optimize(shading_pbr_manager_01_t* ctx);
int shading_pbr_manager_01_debug_print(shading_pbr_manager_01_t* ctx);

/* Module functions */
int shading_pbr_manager_01_module_init(void);
int shading_pbr_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_PBR_MANAGER_01_H */
