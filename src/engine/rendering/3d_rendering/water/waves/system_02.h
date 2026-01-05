/*
 * water_waves_system_02.h
 *
 * Header file for water_waves_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_WAVES_SYSTEM_02_H
#define WATER_WAVES_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_waves_system_02 water_waves_system_02_t;
typedef struct water_waves_system_02_desc water_waves_system_02_desc_t;
typedef struct water_waves_system_02_stats water_waves_system_02_stats_t;

/* Creation and destruction */
int water_waves_system_02_create(water_waves_system_02_t** out_ctx, const water_waves_system_02_desc_t* desc);
int water_waves_system_02_destroy(water_waves_system_02_t* ctx);

/* Core operations */
int water_waves_system_02_create_system(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_destroy_system(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_tick(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_process(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_submit(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_execute(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_sync(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_query(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_configure(water_waves_system_02_t* ctx, void* params);
int water_waves_system_02_optimize(water_waves_system_02_t* ctx, void* params);

/* Utility functions */
int water_waves_system_02_get_stats(water_waves_system_02_t* ctx);
int water_waves_system_02_set_callback(water_waves_system_02_t* ctx);
int water_waves_system_02_get_memory_usage(water_waves_system_02_t* ctx);
int water_waves_system_02_optimize(water_waves_system_02_t* ctx);
int water_waves_system_02_debug_print(water_waves_system_02_t* ctx);

/* Module functions */
int water_waves_system_02_module_init(void);
int water_waves_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WAVES_SYSTEM_02_H */
