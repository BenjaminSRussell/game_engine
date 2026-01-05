/*
 * geometry_instancing_manager_01.h
 *
 * Header file for geometry_instancing_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCING_MANAGER_01_H
#define GEOMETRY_INSTANCING_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_instancing_manager_01 geometry_instancing_manager_01_t;
typedef struct geometry_instancing_manager_01_desc geometry_instancing_manager_01_desc_t;
typedef struct geometry_instancing_manager_01_stats geometry_instancing_manager_01_stats_t;

/* Creation and destruction */
int geometry_instancing_manager_01_create(geometry_instancing_manager_01_t** out_ctx, const geometry_instancing_manager_01_desc_t* desc);
int geometry_instancing_manager_01_destroy(geometry_instancing_manager_01_t* ctx);

/* Core operations */
int geometry_instancing_manager_01_init(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_shutdown(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_update(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_create(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_destroy(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_get(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_set(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_reset(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_validate(geometry_instancing_manager_01_t* ctx, void* params);
int geometry_instancing_manager_01_flush(geometry_instancing_manager_01_t* ctx, void* params);

/* Utility functions */
int geometry_instancing_manager_01_get_stats(geometry_instancing_manager_01_t* ctx);
int geometry_instancing_manager_01_set_callback(geometry_instancing_manager_01_t* ctx);
int geometry_instancing_manager_01_get_memory_usage(geometry_instancing_manager_01_t* ctx);
int geometry_instancing_manager_01_optimize(geometry_instancing_manager_01_t* ctx);
int geometry_instancing_manager_01_debug_print(geometry_instancing_manager_01_t* ctx);

/* Module functions */
int geometry_instancing_manager_01_module_init(void);
int geometry_instancing_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCING_MANAGER_01_H */
