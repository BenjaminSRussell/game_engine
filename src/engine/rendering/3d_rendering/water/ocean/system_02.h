/*
 * water_ocean_system_02.h
 *
 * Header file for water_ocean_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_OCEAN_SYSTEM_02_H
#define WATER_OCEAN_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_ocean_system_02 water_ocean_system_02_t;
typedef struct water_ocean_system_02_desc water_ocean_system_02_desc_t;
typedef struct water_ocean_system_02_stats water_ocean_system_02_stats_t;

/* Creation and destruction */
int water_ocean_system_02_create(water_ocean_system_02_t** out_ctx, const water_ocean_system_02_desc_t* desc);
int water_ocean_system_02_destroy(water_ocean_system_02_t* ctx);

/* Core operations */
int water_ocean_system_02_create_system(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_destroy_system(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_tick(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_process(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_submit(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_execute(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_sync(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_query(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_configure(water_ocean_system_02_t* ctx, void* params);
int water_ocean_system_02_optimize(water_ocean_system_02_t* ctx, void* params);

/* Utility functions */
int water_ocean_system_02_get_stats(water_ocean_system_02_t* ctx);
int water_ocean_system_02_set_callback(water_ocean_system_02_t* ctx);
int water_ocean_system_02_get_memory_usage(water_ocean_system_02_t* ctx);
int water_ocean_system_02_optimize(water_ocean_system_02_t* ctx);
int water_ocean_system_02_debug_print(water_ocean_system_02_t* ctx);

/* Module functions */
int water_ocean_system_02_module_init(void);
int water_ocean_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_OCEAN_SYSTEM_02_H */
