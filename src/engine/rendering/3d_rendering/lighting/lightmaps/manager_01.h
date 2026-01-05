/*
 * lighting_lightmaps_manager_01.h
 *
 * Header file for lighting_lightmaps_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_LIGHTMAPS_MANAGER_01_H
#define LIGHTING_LIGHTMAPS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_lightmaps_manager_01 lighting_lightmaps_manager_01_t;
typedef struct lighting_lightmaps_manager_01_desc lighting_lightmaps_manager_01_desc_t;
typedef struct lighting_lightmaps_manager_01_stats lighting_lightmaps_manager_01_stats_t;

/* Creation and destruction */
int lighting_lightmaps_manager_01_create(lighting_lightmaps_manager_01_t** out_ctx, const lighting_lightmaps_manager_01_desc_t* desc);
int lighting_lightmaps_manager_01_destroy(lighting_lightmaps_manager_01_t* ctx);

/* Core operations */
int lighting_lightmaps_manager_01_init(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_shutdown(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_update(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_create(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_destroy(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_get(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_set(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_reset(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_validate(lighting_lightmaps_manager_01_t* ctx, void* params);
int lighting_lightmaps_manager_01_flush(lighting_lightmaps_manager_01_t* ctx, void* params);

/* Utility functions */
int lighting_lightmaps_manager_01_get_stats(lighting_lightmaps_manager_01_t* ctx);
int lighting_lightmaps_manager_01_set_callback(lighting_lightmaps_manager_01_t* ctx);
int lighting_lightmaps_manager_01_get_memory_usage(lighting_lightmaps_manager_01_t* ctx);
int lighting_lightmaps_manager_01_optimize(lighting_lightmaps_manager_01_t* ctx);
int lighting_lightmaps_manager_01_debug_print(lighting_lightmaps_manager_01_t* ctx);

/* Module functions */
int lighting_lightmaps_manager_01_module_init(void);
int lighting_lightmaps_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_LIGHTMAPS_MANAGER_01_H */
