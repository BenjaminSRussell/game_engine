/*
 * water_ocean_manager_01.h
 *
 * Header file for water_ocean_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_OCEAN_MANAGER_01_H
#define WATER_OCEAN_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_ocean_manager_01 water_ocean_manager_01_t;
typedef struct water_ocean_manager_01_desc water_ocean_manager_01_desc_t;
typedef struct water_ocean_manager_01_stats water_ocean_manager_01_stats_t;

/* Creation and destruction */
int water_ocean_manager_01_create(water_ocean_manager_01_t** out_ctx, const water_ocean_manager_01_desc_t* desc);
int water_ocean_manager_01_destroy(water_ocean_manager_01_t* ctx);

/* Core operations */
int water_ocean_manager_01_init(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_shutdown(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_update(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_create(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_destroy(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_get(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_set(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_reset(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_validate(water_ocean_manager_01_t* ctx, void* params);
int water_ocean_manager_01_flush(water_ocean_manager_01_t* ctx, void* params);

/* Utility functions */
int water_ocean_manager_01_get_stats(water_ocean_manager_01_t* ctx);
int water_ocean_manager_01_set_callback(water_ocean_manager_01_t* ctx);
int water_ocean_manager_01_get_memory_usage(water_ocean_manager_01_t* ctx);
int water_ocean_manager_01_optimize(water_ocean_manager_01_t* ctx);
int water_ocean_manager_01_debug_print(water_ocean_manager_01_t* ctx);

/* Module functions */
int water_ocean_manager_01_module_init(void);
int water_ocean_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_OCEAN_MANAGER_01_H */
