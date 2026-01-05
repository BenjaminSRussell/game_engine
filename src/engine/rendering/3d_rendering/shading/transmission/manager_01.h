/*
 * shading_transmission_manager_01.h
 *
 * Header file for shading_transmission_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_TRANSMISSION_MANAGER_01_H
#define SHADING_TRANSMISSION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_transmission_manager_01 shading_transmission_manager_01_t;
typedef struct shading_transmission_manager_01_desc shading_transmission_manager_01_desc_t;
typedef struct shading_transmission_manager_01_stats shading_transmission_manager_01_stats_t;

/* Creation and destruction */
int shading_transmission_manager_01_create(shading_transmission_manager_01_t** out_ctx, const shading_transmission_manager_01_desc_t* desc);
int shading_transmission_manager_01_destroy(shading_transmission_manager_01_t* ctx);

/* Core operations */
int shading_transmission_manager_01_init(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_shutdown(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_update(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_create(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_destroy(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_get(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_set(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_reset(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_validate(shading_transmission_manager_01_t* ctx, void* params);
int shading_transmission_manager_01_flush(shading_transmission_manager_01_t* ctx, void* params);

/* Utility functions */
int shading_transmission_manager_01_get_stats(shading_transmission_manager_01_t* ctx);
int shading_transmission_manager_01_set_callback(shading_transmission_manager_01_t* ctx);
int shading_transmission_manager_01_get_memory_usage(shading_transmission_manager_01_t* ctx);
int shading_transmission_manager_01_optimize(shading_transmission_manager_01_t* ctx);
int shading_transmission_manager_01_debug_print(shading_transmission_manager_01_t* ctx);

/* Module functions */
int shading_transmission_manager_01_module_init(void);
int shading_transmission_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_TRANSMISSION_MANAGER_01_H */
