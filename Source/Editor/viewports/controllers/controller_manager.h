/*
 * viewports_controllers_manager_01.h
 *
 * Header file for viewports_controllers_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_CONTROLLERS_MANAGER_01_H
#define VIEWPORTS_CONTROLLERS_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_controllers_manager_01 viewports_controllers_manager_01_t;
typedef struct viewports_controllers_manager_01_desc viewports_controllers_manager_01_desc_t;
typedef struct viewports_controllers_manager_01_stats viewports_controllers_manager_01_stats_t;

/* Creation and destruction */
int viewports_controllers_manager_01_create(viewports_controllers_manager_01_t** out_ctx, const viewports_controllers_manager_01_desc_t* desc);
int viewports_controllers_manager_01_destroy(viewports_controllers_manager_01_t* ctx);

/* Core operations */
int viewports_controllers_manager_01_init(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_shutdown(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_update(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_create_legacy(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_destroy_legacy(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_get(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_set(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_reset(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_validate(viewports_controllers_manager_01_t* ctx, void* params);
int viewports_controllers_manager_01_flush(viewports_controllers_manager_01_t* ctx, void* params);

/* Utility functions */
int viewports_controllers_manager_01_get_stats(viewports_controllers_manager_01_t* ctx);
int viewports_controllers_manager_01_set_callback(viewports_controllers_manager_01_t* ctx);
int viewports_controllers_manager_01_get_memory_usage(viewports_controllers_manager_01_t* ctx);
int viewports_controllers_manager_01_optimize(viewports_controllers_manager_01_t* ctx);
int viewports_controllers_manager_01_debug_print(viewports_controllers_manager_01_t* ctx);

/* Module functions */
int viewports_controllers_manager_01_module_init(void);
int viewports_controllers_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_CONTROLLERS_MANAGER_01_H */
