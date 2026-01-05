/*
 * culling_portal_system_02.h
 *
 * Header file for culling_portal_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_PORTAL_SYSTEM_02_H
#define CULLING_PORTAL_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_portal_system_02 culling_portal_system_02_t;
typedef struct culling_portal_system_02_desc culling_portal_system_02_desc_t;
typedef struct culling_portal_system_02_stats culling_portal_system_02_stats_t;

/* Creation and destruction */
int culling_portal_system_02_create(culling_portal_system_02_t** out_ctx, const culling_portal_system_02_desc_t* desc);
int culling_portal_system_02_destroy(culling_portal_system_02_t* ctx);

/* Core operations */
int culling_portal_system_02_create_system(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_destroy_system(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_tick(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_process(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_submit(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_execute(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_sync(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_query(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_configure(culling_portal_system_02_t* ctx, void* params);
int culling_portal_system_02_optimize(culling_portal_system_02_t* ctx, void* params);

/* Utility functions */
int culling_portal_system_02_get_stats(culling_portal_system_02_t* ctx);
int culling_portal_system_02_set_callback(culling_portal_system_02_t* ctx);
int culling_portal_system_02_get_memory_usage(culling_portal_system_02_t* ctx);
int culling_portal_system_02_optimize(culling_portal_system_02_t* ctx);
int culling_portal_system_02_debug_print(culling_portal_system_02_t* ctx);

/* Module functions */
int culling_portal_system_02_module_init(void);
int culling_portal_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_PORTAL_SYSTEM_02_H */
