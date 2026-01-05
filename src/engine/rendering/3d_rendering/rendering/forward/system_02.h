/*
 * rendering_forward_system_02.h
 *
 * Header file for rendering_forward_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_SYSTEM_02_H
#define RENDERING_FORWARD_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_forward_system_02 rendering_forward_system_02_t;
typedef struct rendering_forward_system_02_desc rendering_forward_system_02_desc_t;
typedef struct rendering_forward_system_02_stats rendering_forward_system_02_stats_t;

/* Creation and destruction */
int rendering_forward_system_02_create(rendering_forward_system_02_t** out_ctx, const rendering_forward_system_02_desc_t* desc);
int rendering_forward_system_02_destroy(rendering_forward_system_02_t* ctx);

/* Core operations */
int rendering_forward_system_02_create_system(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_destroy_system(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_tick(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_process(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_submit(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_execute(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_sync(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_query(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_configure(rendering_forward_system_02_t* ctx, void* params);
int rendering_forward_system_02_optimize(rendering_forward_system_02_t* ctx, void* params);

/* Utility functions */
int rendering_forward_system_02_get_stats(rendering_forward_system_02_t* ctx);
int rendering_forward_system_02_set_callback(rendering_forward_system_02_t* ctx);
int rendering_forward_system_02_get_memory_usage(rendering_forward_system_02_t* ctx);
int rendering_forward_system_02_optimize(rendering_forward_system_02_t* ctx);
int rendering_forward_system_02_debug_print(rendering_forward_system_02_t* ctx);

/* Module functions */
int rendering_forward_system_02_module_init(void);
int rendering_forward_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_SYSTEM_02_H */
