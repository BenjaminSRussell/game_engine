/*
 * water_waves_manager_01.h
 *
 * Header file for water_waves_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_WAVES_MANAGER_01_H
#define WATER_WAVES_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_waves_manager_01 water_waves_manager_01_t;
typedef struct water_waves_manager_01_desc water_waves_manager_01_desc_t;
typedef struct water_waves_manager_01_stats water_waves_manager_01_stats_t;

/* Creation and destruction */
int water_waves_manager_01_create(water_waves_manager_01_t** out_ctx, const water_waves_manager_01_desc_t* desc);
int water_waves_manager_01_destroy(water_waves_manager_01_t* ctx);

/* Core operations */
int water_waves_manager_01_init(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_shutdown(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_update(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_create(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_destroy(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_get(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_set(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_reset(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_validate(water_waves_manager_01_t* ctx, void* params);
int water_waves_manager_01_flush(water_waves_manager_01_t* ctx, void* params);

/* Utility functions */
int water_waves_manager_01_get_stats(water_waves_manager_01_t* ctx);
int water_waves_manager_01_set_callback(water_waves_manager_01_t* ctx);
int water_waves_manager_01_get_memory_usage(water_waves_manager_01_t* ctx);
int water_waves_manager_01_optimize(water_waves_manager_01_t* ctx);
int water_waves_manager_01_debug_print(water_waves_manager_01_t* ctx);

/* Module functions */
int water_waves_manager_01_module_init(void);
int water_waves_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WAVES_MANAGER_01_H */
