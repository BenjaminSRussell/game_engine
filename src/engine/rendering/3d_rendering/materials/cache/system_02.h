/*
 * materials_cache_system_02.h
 *
 * Header file for materials_cache_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_CACHE_SYSTEM_02_H
#define MATERIALS_CACHE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_cache_system_02 materials_cache_system_02_t;
typedef struct materials_cache_system_02_desc materials_cache_system_02_desc_t;
typedef struct materials_cache_system_02_stats materials_cache_system_02_stats_t;

/* Creation and destruction */
int materials_cache_system_02_create(materials_cache_system_02_t** out_ctx, const materials_cache_system_02_desc_t* desc);
int materials_cache_system_02_destroy(materials_cache_system_02_t* ctx);

/* Core operations */
int materials_cache_system_02_create_system(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_destroy_system(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_tick(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_process(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_submit(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_execute(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_sync(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_query(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_configure(materials_cache_system_02_t* ctx, void* params);
int materials_cache_system_02_optimize(materials_cache_system_02_t* ctx, void* params);

/* Utility functions */
int materials_cache_system_02_get_stats(materials_cache_system_02_t* ctx);
int materials_cache_system_02_set_callback(materials_cache_system_02_t* ctx);
int materials_cache_system_02_get_memory_usage(materials_cache_system_02_t* ctx);
int materials_cache_system_02_optimize(materials_cache_system_02_t* ctx);
int materials_cache_system_02_debug_print(materials_cache_system_02_t* ctx);

/* Module functions */
int materials_cache_system_02_module_init(void);
int materials_cache_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_CACHE_SYSTEM_02_H */
