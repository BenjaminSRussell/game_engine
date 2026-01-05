/*
 * shading_sheen_manager_01.h
 *
 * Header file for shading_sheen_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_SHEEN_MANAGER_01_H
#define SHADING_SHEEN_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_sheen_manager_01 shading_sheen_manager_01_t;
typedef struct shading_sheen_manager_01_desc shading_sheen_manager_01_desc_t;
typedef struct shading_sheen_manager_01_stats shading_sheen_manager_01_stats_t;

/* Creation and destruction */
int shading_sheen_manager_01_create(shading_sheen_manager_01_t** out_ctx, const shading_sheen_manager_01_desc_t* desc);
int shading_sheen_manager_01_destroy(shading_sheen_manager_01_t* ctx);

/* Core operations */
int shading_sheen_manager_01_init(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_shutdown(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_update(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_create(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_destroy(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_get(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_set(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_reset(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_validate(shading_sheen_manager_01_t* ctx, void* params);
int shading_sheen_manager_01_flush(shading_sheen_manager_01_t* ctx, void* params);

/* Utility functions */
int shading_sheen_manager_01_get_stats(shading_sheen_manager_01_t* ctx);
int shading_sheen_manager_01_set_callback(shading_sheen_manager_01_t* ctx);
int shading_sheen_manager_01_get_memory_usage(shading_sheen_manager_01_t* ctx);
int shading_sheen_manager_01_optimize(shading_sheen_manager_01_t* ctx);
int shading_sheen_manager_01_debug_print(shading_sheen_manager_01_t* ctx);

/* Module functions */
int shading_sheen_manager_01_module_init(void);
int shading_sheen_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SHEEN_MANAGER_01_H */
