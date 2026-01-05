/*
 * lighting_cascades_manager_01.h
 *
 * Header file for lighting_cascades_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADES_MANAGER_01_H
#define LIGHTING_CASCADES_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_cascades_manager_01 lighting_cascades_manager_01_t;
typedef struct lighting_cascades_manager_01_desc lighting_cascades_manager_01_desc_t;
typedef struct lighting_cascades_manager_01_stats lighting_cascades_manager_01_stats_t;

/* Creation and destruction */
int lighting_cascades_manager_01_create(lighting_cascades_manager_01_t** out_ctx, const lighting_cascades_manager_01_desc_t* desc);
int lighting_cascades_manager_01_destroy(lighting_cascades_manager_01_t* ctx);

/* Core operations */
int lighting_cascades_manager_01_init(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_shutdown(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_update(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_create(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_destroy(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_get(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_set(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_reset(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_validate(lighting_cascades_manager_01_t* ctx, void* params);
int lighting_cascades_manager_01_flush(lighting_cascades_manager_01_t* ctx, void* params);

/* Utility functions */
int lighting_cascades_manager_01_get_stats(lighting_cascades_manager_01_t* ctx);
int lighting_cascades_manager_01_set_callback(lighting_cascades_manager_01_t* ctx);
int lighting_cascades_manager_01_get_memory_usage(lighting_cascades_manager_01_t* ctx);
int lighting_cascades_manager_01_optimize(lighting_cascades_manager_01_t* ctx);
int lighting_cascades_manager_01_debug_print(lighting_cascades_manager_01_t* ctx);

/* Module functions */
int lighting_cascades_manager_01_module_init(void);
int lighting_cascades_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADES_MANAGER_01_H */
