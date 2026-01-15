/*
 * viewports_multiview_system_02.h
 *
 * Header file for viewports_multiview_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_MULTIVIEW_SYSTEM_02_H
#define VIEWPORTS_MULTIVIEW_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_multiview_system_02 viewports_multiview_system_02_t;
typedef struct viewports_multiview_system_02_desc viewports_multiview_system_02_desc_t;
typedef struct viewports_multiview_system_02_stats viewports_multiview_system_02_stats_t;

/* Creation and destruction */
int viewports_multiview_system_02_create(viewports_multiview_system_02_t** out_ctx, const viewports_multiview_system_02_desc_t* desc);
int viewports_multiview_system_02_destroy(viewports_multiview_system_02_t* ctx);

/* Core operations */
int viewports_multiview_system_02_create_system(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_destroy_system(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_tick(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_process(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_submit(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_execute(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_sync(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_query(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_configure(viewports_multiview_system_02_t* ctx, void* params);
int viewports_multiview_system_02_optimize(viewports_multiview_system_02_t* ctx, void* params);

/* Utility functions */
int viewports_multiview_system_02_get_stats(viewports_multiview_system_02_t* ctx);
int viewports_multiview_system_02_set_callback(viewports_multiview_system_02_t* ctx);
int viewports_multiview_system_02_get_memory_usage(viewports_multiview_system_02_t* ctx);
int viewports_multiview_system_02_optimize(viewports_multiview_system_02_t* ctx);
int viewports_multiview_system_02_debug_print(viewports_multiview_system_02_t* ctx);

/* Module functions */
int viewports_multiview_system_02_module_init(void);
int viewports_multiview_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_MULTIVIEW_SYSTEM_02_H */
