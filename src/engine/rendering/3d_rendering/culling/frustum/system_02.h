/*
 * culling_frustum_system_02.h
 *
 * Header file for culling_frustum_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_FRUSTUM_SYSTEM_02_H
#define CULLING_FRUSTUM_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_frustum_system_02 culling_frustum_system_02_t;
typedef struct culling_frustum_system_02_desc culling_frustum_system_02_desc_t;
typedef struct culling_frustum_system_02_stats culling_frustum_system_02_stats_t;

/* Creation and destruction */
int culling_frustum_system_02_create(culling_frustum_system_02_t** out_ctx, const culling_frustum_system_02_desc_t* desc);
int culling_frustum_system_02_destroy(culling_frustum_system_02_t* ctx);

/* Core operations */
int culling_frustum_system_02_create_system(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_destroy_system(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_tick(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_process(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_submit(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_execute(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_sync(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_query(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_configure(culling_frustum_system_02_t* ctx, void* params);
int culling_frustum_system_02_optimize(culling_frustum_system_02_t* ctx, void* params);

/* Utility functions */
int culling_frustum_system_02_get_stats(culling_frustum_system_02_t* ctx);
int culling_frustum_system_02_set_callback(culling_frustum_system_02_t* ctx);
int culling_frustum_system_02_get_memory_usage(culling_frustum_system_02_t* ctx);
int culling_frustum_system_02_optimize(culling_frustum_system_02_t* ctx);
int culling_frustum_system_02_debug_print(culling_frustum_system_02_t* ctx);

/* Module functions */
int culling_frustum_system_02_module_init(void);
int culling_frustum_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_FRUSTUM_SYSTEM_02_H */
