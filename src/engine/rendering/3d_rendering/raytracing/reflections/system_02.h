/*
 * raytracing_reflections_system_02.h
 *
 * Header file for raytracing_reflections_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_REFLECTIONS_SYSTEM_02_H
#define RAYTRACING_REFLECTIONS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct raytracing_reflections_system_02 raytracing_reflections_system_02_t;
typedef struct raytracing_reflections_system_02_desc raytracing_reflections_system_02_desc_t;
typedef struct raytracing_reflections_system_02_stats raytracing_reflections_system_02_stats_t;

/* Creation and destruction */
int raytracing_reflections_system_02_create(raytracing_reflections_system_02_t** out_ctx, const raytracing_reflections_system_02_desc_t* desc);
int raytracing_reflections_system_02_destroy(raytracing_reflections_system_02_t* ctx);

/* Core operations */
int raytracing_reflections_system_02_create_system(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_destroy_system(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_tick(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_process(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_submit(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_execute(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_sync(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_query(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_configure(raytracing_reflections_system_02_t* ctx, void* params);
int raytracing_reflections_system_02_optimize(raytracing_reflections_system_02_t* ctx, void* params);

/* Utility functions */
int raytracing_reflections_system_02_get_stats(raytracing_reflections_system_02_t* ctx);
int raytracing_reflections_system_02_set_callback(raytracing_reflections_system_02_t* ctx);
int raytracing_reflections_system_02_get_memory_usage(raytracing_reflections_system_02_t* ctx);
int raytracing_reflections_system_02_optimize(raytracing_reflections_system_02_t* ctx);
int raytracing_reflections_system_02_debug_print(raytracing_reflections_system_02_t* ctx);

/* Module functions */
int raytracing_reflections_system_02_module_init(void);
int raytracing_reflections_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_REFLECTIONS_SYSTEM_02_H */
