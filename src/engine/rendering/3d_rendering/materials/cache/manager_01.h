/*
 * materials_cache_manager_01.h
 *
 * Header file for materials_cache_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_CACHE_MANAGER_01_H
#define MATERIALS_CACHE_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_cache_manager_01 materials_cache_manager_01_t;
typedef struct materials_cache_manager_01_desc materials_cache_manager_01_desc_t;
typedef struct materials_cache_manager_01_stats materials_cache_manager_01_stats_t;

/* Creation and destruction */
int materials_cache_manager_01_create(materials_cache_manager_01_t** out_ctx, const materials_cache_manager_01_desc_t* desc);
int materials_cache_manager_01_destroy(materials_cache_manager_01_t* ctx);

/* Core operations */
int materials_cache_manager_01_init(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_shutdown(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_update(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_create(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_destroy(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_get(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_set(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_reset(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_validate(materials_cache_manager_01_t* ctx, void* params);
int materials_cache_manager_01_flush(materials_cache_manager_01_t* ctx, void* params);

/* Utility functions */
int materials_cache_manager_01_get_stats(materials_cache_manager_01_t* ctx);
int materials_cache_manager_01_set_callback(materials_cache_manager_01_t* ctx);
int materials_cache_manager_01_get_memory_usage(materials_cache_manager_01_t* ctx);
int materials_cache_manager_01_optimize(materials_cache_manager_01_t* ctx);
int materials_cache_manager_01_debug_print(materials_cache_manager_01_t* ctx);

/* Module functions */
int materials_cache_manager_01_module_init(void);
int materials_cache_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_CACHE_MANAGER_01_H */
