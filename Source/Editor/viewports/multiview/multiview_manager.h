/*
 * viewports_multiview_manager_01.h
 *
 * Header file for viewports_multiview_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_MULTIVIEW_MANAGER_01_H
#define VIEWPORTS_MULTIVIEW_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_multiview_manager_01 viewports_multiview_manager_01_t;
typedef struct viewports_multiview_manager_01_desc viewports_multiview_manager_01_desc_t;
typedef struct viewports_multiview_manager_01_stats viewports_multiview_manager_01_stats_t;

/* Creation and destruction */
int viewports_multiview_manager_01_create(viewports_multiview_manager_01_t** out_ctx, const viewports_multiview_manager_01_desc_t* desc);
int viewports_multiview_manager_01_destroy(viewports_multiview_manager_01_t* ctx);

/* Core operations */
int viewports_multiview_manager_01_init(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_shutdown(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_update(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_create_legacy(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_destroy_legacy(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_get(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_set(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_reset(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_validate(viewports_multiview_manager_01_t* ctx, void* params);
int viewports_multiview_manager_01_flush(viewports_multiview_manager_01_t* ctx, void* params);

/* Utility functions */
int viewports_multiview_manager_01_get_stats(viewports_multiview_manager_01_t* ctx);
int viewports_multiview_manager_01_set_callback(viewports_multiview_manager_01_t* ctx);
int viewports_multiview_manager_01_get_memory_usage(viewports_multiview_manager_01_t* ctx);
int viewports_multiview_manager_01_optimize(viewports_multiview_manager_01_t* ctx);
int viewports_multiview_manager_01_debug_print(viewports_multiview_manager_01_t* ctx);

/* Module functions */
int viewports_multiview_manager_01_module_init(void);
int viewports_multiview_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_MULTIVIEW_MANAGER_01_H */
