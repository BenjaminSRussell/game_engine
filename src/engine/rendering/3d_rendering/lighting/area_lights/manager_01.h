/*
 * lighting_area_lights_manager_01.h
 *
 * Header file for lighting_area_lights_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_AREA_LIGHTS_MANAGER_01_H
#define LIGHTING_AREA_LIGHTS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_area_lights_manager_01 lighting_area_lights_manager_01_t;
typedef struct lighting_area_lights_manager_01_desc lighting_area_lights_manager_01_desc_t;
typedef struct lighting_area_lights_manager_01_stats lighting_area_lights_manager_01_stats_t;

/* Creation and destruction */
int lighting_area_lights_manager_01_create(lighting_area_lights_manager_01_t** out_ctx, const lighting_area_lights_manager_01_desc_t* desc);
int lighting_area_lights_manager_01_destroy(lighting_area_lights_manager_01_t* ctx);

/* Core operations */
int lighting_area_lights_manager_01_init(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_shutdown(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_update(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_create(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_destroy(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_get(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_set(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_reset(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_validate(lighting_area_lights_manager_01_t* ctx, void* params);
int lighting_area_lights_manager_01_flush(lighting_area_lights_manager_01_t* ctx, void* params);

/* Utility functions */
int lighting_area_lights_manager_01_get_stats(lighting_area_lights_manager_01_t* ctx);
int lighting_area_lights_manager_01_set_callback(lighting_area_lights_manager_01_t* ctx);
int lighting_area_lights_manager_01_get_memory_usage(lighting_area_lights_manager_01_t* ctx);
int lighting_area_lights_manager_01_optimize(lighting_area_lights_manager_01_t* ctx);
int lighting_area_lights_manager_01_debug_print(lighting_area_lights_manager_01_t* ctx);

/* Module functions */
int lighting_area_lights_manager_01_module_init(void);
int lighting_area_lights_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_AREA_LIGHTS_MANAGER_01_H */
