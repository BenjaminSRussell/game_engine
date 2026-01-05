/*
 * lighting_volumetric_manager_01.h
 *
 * Header file for lighting_volumetric_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_VOLUMETRIC_MANAGER_01_H
#define LIGHTING_VOLUMETRIC_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_volumetric_manager_01 lighting_volumetric_manager_01_t;
typedef struct lighting_volumetric_manager_01_desc lighting_volumetric_manager_01_desc_t;
typedef struct lighting_volumetric_manager_01_stats lighting_volumetric_manager_01_stats_t;

/* Creation and destruction */
int lighting_volumetric_manager_01_create(lighting_volumetric_manager_01_t** out_ctx, const lighting_volumetric_manager_01_desc_t* desc);
int lighting_volumetric_manager_01_destroy(lighting_volumetric_manager_01_t* ctx);

/* Core operations */
int lighting_volumetric_manager_01_init(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_shutdown(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_update(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_create(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_destroy(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_get(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_set(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_reset(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_validate(lighting_volumetric_manager_01_t* ctx, void* params);
int lighting_volumetric_manager_01_flush(lighting_volumetric_manager_01_t* ctx, void* params);

/* Utility functions */
int lighting_volumetric_manager_01_get_stats(lighting_volumetric_manager_01_t* ctx);
int lighting_volumetric_manager_01_set_callback(lighting_volumetric_manager_01_t* ctx);
int lighting_volumetric_manager_01_get_memory_usage(lighting_volumetric_manager_01_t* ctx);
int lighting_volumetric_manager_01_optimize(lighting_volumetric_manager_01_t* ctx);
int lighting_volumetric_manager_01_debug_print(lighting_volumetric_manager_01_t* ctx);

/* Module functions */
int lighting_volumetric_manager_01_module_init(void);
int lighting_volumetric_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOLUMETRIC_MANAGER_01_H */
