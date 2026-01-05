/*
 * rendering_visibility_system_02.h
 *
 * Header file for rendering_visibility_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_VISIBILITY_SYSTEM_02_H
#define RENDERING_VISIBILITY_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_visibility_system_02 rendering_visibility_system_02_t;
typedef struct rendering_visibility_system_02_desc rendering_visibility_system_02_desc_t;
typedef struct rendering_visibility_system_02_stats rendering_visibility_system_02_stats_t;

/* Creation and destruction */
int rendering_visibility_system_02_create(rendering_visibility_system_02_t** out_ctx, const rendering_visibility_system_02_desc_t* desc);
int rendering_visibility_system_02_destroy(rendering_visibility_system_02_t* ctx);

/* Core operations */
int rendering_visibility_system_02_create_system(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_destroy_system(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_tick(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_process(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_submit(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_execute(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_sync(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_query(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_configure(rendering_visibility_system_02_t* ctx, void* params);
int rendering_visibility_system_02_optimize(rendering_visibility_system_02_t* ctx, void* params);

/* Utility functions */
int rendering_visibility_system_02_get_stats(rendering_visibility_system_02_t* ctx);
int rendering_visibility_system_02_set_callback(rendering_visibility_system_02_t* ctx);
int rendering_visibility_system_02_get_memory_usage(rendering_visibility_system_02_t* ctx);
int rendering_visibility_system_02_optimize(rendering_visibility_system_02_t* ctx);
int rendering_visibility_system_02_debug_print(rendering_visibility_system_02_t* ctx);

/* Module functions */
int rendering_visibility_system_02_module_init(void);
int rendering_visibility_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_VISIBILITY_SYSTEM_02_H */
