/*
 * rendering_clustered_system_02.h
 *
 * Header file for rendering_clustered_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_CLUSTERED_SYSTEM_02_H
#define RENDERING_CLUSTERED_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_clustered_system_02 rendering_clustered_system_02_t;
typedef struct rendering_clustered_system_02_desc rendering_clustered_system_02_desc_t;
typedef struct rendering_clustered_system_02_stats rendering_clustered_system_02_stats_t;

/* Creation and destruction */
int rendering_clustered_system_02_create(rendering_clustered_system_02_t** out_ctx, const rendering_clustered_system_02_desc_t* desc);
int rendering_clustered_system_02_destroy(rendering_clustered_system_02_t* ctx);

/* Core operations */
int rendering_clustered_system_02_create_system(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_destroy_system(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_tick(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_process(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_submit(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_execute(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_sync(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_query(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_configure(rendering_clustered_system_02_t* ctx, void* params);
int rendering_clustered_system_02_optimize(rendering_clustered_system_02_t* ctx, void* params);

/* Utility functions */
int rendering_clustered_system_02_get_stats(rendering_clustered_system_02_t* ctx);
int rendering_clustered_system_02_set_callback(rendering_clustered_system_02_t* ctx);
int rendering_clustered_system_02_get_memory_usage(rendering_clustered_system_02_t* ctx);
int rendering_clustered_system_02_optimize(rendering_clustered_system_02_t* ctx);
int rendering_clustered_system_02_debug_print(rendering_clustered_system_02_t* ctx);

/* Module functions */
int rendering_clustered_system_02_module_init(void);
int rendering_clustered_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_CLUSTERED_SYSTEM_02_H */
