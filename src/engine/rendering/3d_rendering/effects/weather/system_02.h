/*
 * effects_weather_system_02.h
 *
 * Header file for effects_weather_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_WEATHER_SYSTEM_02_H
#define EFFECTS_WEATHER_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_weather_system_02 effects_weather_system_02_t;
typedef struct effects_weather_system_02_desc effects_weather_system_02_desc_t;
typedef struct effects_weather_system_02_stats effects_weather_system_02_stats_t;

/* Creation and destruction */
int effects_weather_system_02_create(effects_weather_system_02_t** out_ctx, const effects_weather_system_02_desc_t* desc);
int effects_weather_system_02_destroy(effects_weather_system_02_t* ctx);

/* Core operations */
int effects_weather_system_02_create_system(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_destroy_system(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_tick(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_process(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_submit(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_execute(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_sync(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_query(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_configure(effects_weather_system_02_t* ctx, void* params);
int effects_weather_system_02_optimize(effects_weather_system_02_t* ctx, void* params);

/* Utility functions */
int effects_weather_system_02_get_stats(effects_weather_system_02_t* ctx);
int effects_weather_system_02_set_callback(effects_weather_system_02_t* ctx);
int effects_weather_system_02_get_memory_usage(effects_weather_system_02_t* ctx);
int effects_weather_system_02_optimize(effects_weather_system_02_t* ctx);
int effects_weather_system_02_debug_print(effects_weather_system_02_t* ctx);

/* Module functions */
int effects_weather_system_02_module_init(void);
int effects_weather_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_WEATHER_SYSTEM_02_H */
