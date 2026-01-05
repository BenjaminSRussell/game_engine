/*
 * core_resource_manager_01.h
 *
 * Header file for core_resource_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_RESOURCE_MANAGER_01_H
#define CORE_RESOURCE_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_resource_manager_01 core_resource_manager_01_t;
typedef struct core_resource_manager_01_desc core_resource_manager_01_desc_t;
typedef struct core_resource_manager_01_stats core_resource_manager_01_stats_t;

/* Creation and destruction */
int core_resource_manager_01_create(core_resource_manager_01_t** out_ctx, const core_resource_manager_01_desc_t* desc);
int core_resource_manager_01_destroy(core_resource_manager_01_t* ctx);

/* Core operations */
int core_resource_manager_01_init(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_shutdown(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_update(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_create(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_destroy(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_get(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_set(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_reset(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_validate(core_resource_manager_01_t* ctx, void* params);
int core_resource_manager_01_flush(core_resource_manager_01_t* ctx, void* params);

/* Utility functions */
int core_resource_manager_01_get_stats(core_resource_manager_01_t* ctx);
int core_resource_manager_01_set_callback(core_resource_manager_01_t* ctx);
int core_resource_manager_01_get_memory_usage(core_resource_manager_01_t* ctx);
int core_resource_manager_01_optimize(core_resource_manager_01_t* ctx);
int core_resource_manager_01_debug_print(core_resource_manager_01_t* ctx);

/* Module functions */
int core_resource_manager_01_module_init(void);
int core_resource_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RESOURCE_MANAGER_01_H */
