/*
 * culling_portal_manager_01.h
 *
 * Header file for culling_portal_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PORTAL_MANAGER_01_H
#define CULLING_PORTAL_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_portal_manager_01 culling_portal_manager_01_t;
typedef struct culling_portal_manager_01_desc culling_portal_manager_01_desc_t;
typedef struct culling_portal_manager_01_stats culling_portal_manager_01_stats_t;

/* Creation and destruction */
int culling_portal_manager_01_create(culling_portal_manager_01_t** out_ctx, const culling_portal_manager_01_desc_t* desc);
int culling_portal_manager_01_destroy(culling_portal_manager_01_t* ctx);

/* Core operations */
int culling_portal_manager_01_init(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_shutdown(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_update(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_create(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_destroy(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_get(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_set(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_reset(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_validate(culling_portal_manager_01_t* ctx, void* params);
int culling_portal_manager_01_flush(culling_portal_manager_01_t* ctx, void* params);

/* Utility functions */
int culling_portal_manager_01_get_stats(culling_portal_manager_01_t* ctx);
int culling_portal_manager_01_set_callback(culling_portal_manager_01_t* ctx);
int culling_portal_manager_01_get_memory_usage(culling_portal_manager_01_t* ctx);
int culling_portal_manager_01_optimize(culling_portal_manager_01_t* ctx);
int culling_portal_manager_01_debug_print(culling_portal_manager_01_t* ctx);

/* Module functions */
int culling_portal_manager_01_module_init(void);
int culling_portal_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PORTAL_MANAGER_01_H */
