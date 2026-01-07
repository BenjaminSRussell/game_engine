/*
 * viewports_projection_system_02.h
 *
 * Header file for viewports_projection_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef VIEWPORTS_PROJECTION_SYSTEM_02_H
#define VIEWPORTS_PROJECTION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct viewports_projection_system_02 viewports_projection_system_02_t;
typedef struct viewports_projection_system_02_desc viewports_projection_system_02_desc_t;
typedef struct viewports_projection_system_02_stats viewports_projection_system_02_stats_t;

/* Creation and destruction */
int viewports_projection_system_02_create(viewports_projection_system_02_t** out_ctx, const viewports_projection_system_02_desc_t* desc);
int viewports_projection_system_02_destroy(viewports_projection_system_02_t* ctx);

/* Core operations */
int viewports_projection_system_02_create_system(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_destroy_system(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_tick(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_process(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_submit(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_execute(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_sync(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_query(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_configure(viewports_projection_system_02_t* ctx, void* params);
int viewports_projection_system_02_optimize(viewports_projection_system_02_t* ctx, void* params);

/* Utility functions */
int viewports_projection_system_02_get_stats(viewports_projection_system_02_t* ctx);
int viewports_projection_system_02_set_callback(viewports_projection_system_02_t* ctx);
int viewports_projection_system_02_get_memory_usage(viewports_projection_system_02_t* ctx);
int viewports_projection_system_02_optimize_legacy(viewports_projection_system_02_t* ctx);
int viewports_projection_system_02_debug_print(viewports_projection_system_02_t* ctx);

/* Module functions */
int viewports_projection_system_02_module_init(void);
int viewports_projection_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIEWPORTS_PROJECTION_SYSTEM_02_H */
