/*
 * texture_residency_manager_01.h
 *
 * Header file for texture_residency_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_RESIDENCY_MANAGER_01_H
#define TEXTURE_RESIDENCY_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_residency_manager_01 texture_residency_manager_01_t;
typedef struct texture_residency_manager_01_desc texture_residency_manager_01_desc_t;
typedef struct texture_residency_manager_01_stats texture_residency_manager_01_stats_t;

/* Creation and destruction */
int texture_residency_manager_01_create(texture_residency_manager_01_t** out_ctx, const texture_residency_manager_01_desc_t* desc);
int texture_residency_manager_01_destroy(texture_residency_manager_01_t* ctx);

/* Core operations */
int texture_residency_manager_01_init(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_shutdown(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_update(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_create(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_destroy(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_get(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_set(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_reset(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_validate(texture_residency_manager_01_t* ctx, void* params);
int texture_residency_manager_01_flush(texture_residency_manager_01_t* ctx, void* params);

/* Utility functions */
int texture_residency_manager_01_get_stats(texture_residency_manager_01_t* ctx);
int texture_residency_manager_01_set_callback(texture_residency_manager_01_t* ctx);
int texture_residency_manager_01_get_memory_usage(texture_residency_manager_01_t* ctx);
int texture_residency_manager_01_optimize(texture_residency_manager_01_t* ctx);
int texture_residency_manager_01_debug_print(texture_residency_manager_01_t* ctx);

/* Module functions */
int texture_residency_manager_01_module_init(void);
int texture_residency_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_RESIDENCY_MANAGER_01_H */
