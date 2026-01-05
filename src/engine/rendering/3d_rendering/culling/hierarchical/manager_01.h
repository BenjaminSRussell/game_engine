/*
 * culling_hierarchical_manager_01.h
 *
 * Header file for culling_hierarchical_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_HIERARCHICAL_MANAGER_01_H
#define CULLING_HIERARCHICAL_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_hierarchical_manager_01 culling_hierarchical_manager_01_t;
typedef struct culling_hierarchical_manager_01_desc culling_hierarchical_manager_01_desc_t;
typedef struct culling_hierarchical_manager_01_stats culling_hierarchical_manager_01_stats_t;

/* Creation and destruction */
int culling_hierarchical_manager_01_create(culling_hierarchical_manager_01_t** out_ctx, const culling_hierarchical_manager_01_desc_t* desc);
int culling_hierarchical_manager_01_destroy(culling_hierarchical_manager_01_t* ctx);

/* Core operations */
int culling_hierarchical_manager_01_init(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_shutdown(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_update(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_create(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_destroy(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_get(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_set(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_reset(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_validate(culling_hierarchical_manager_01_t* ctx, void* params);
int culling_hierarchical_manager_01_flush(culling_hierarchical_manager_01_t* ctx, void* params);

/* Utility functions */
int culling_hierarchical_manager_01_get_stats(culling_hierarchical_manager_01_t* ctx);
int culling_hierarchical_manager_01_set_callback(culling_hierarchical_manager_01_t* ctx);
int culling_hierarchical_manager_01_get_memory_usage(culling_hierarchical_manager_01_t* ctx);
int culling_hierarchical_manager_01_optimize(culling_hierarchical_manager_01_t* ctx);
int culling_hierarchical_manager_01_debug_print(culling_hierarchical_manager_01_t* ctx);

/* Module functions */
int culling_hierarchical_manager_01_module_init(void);
int culling_hierarchical_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HIERARCHICAL_MANAGER_01_H */
