/*
 * rendering_forward_manager_01.h
 *
 * Header file for rendering_forward_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_MANAGER_01_H
#define RENDERING_FORWARD_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_forward_manager_01 rendering_forward_manager_01_t;
typedef struct rendering_forward_manager_01_desc rendering_forward_manager_01_desc_t;
typedef struct rendering_forward_manager_01_stats rendering_forward_manager_01_stats_t;

/* Creation and destruction */
int rendering_forward_manager_01_create(rendering_forward_manager_01_t** out_ctx, const rendering_forward_manager_01_desc_t* desc);
int rendering_forward_manager_01_destroy(rendering_forward_manager_01_t* ctx);

/* Core operations */
int rendering_forward_manager_01_init(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_shutdown(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_update(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_create(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_destroy(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_get(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_set(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_reset(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_validate(rendering_forward_manager_01_t* ctx, void* params);
int rendering_forward_manager_01_flush(rendering_forward_manager_01_t* ctx, void* params);

/* Utility functions */
int rendering_forward_manager_01_get_stats(rendering_forward_manager_01_t* ctx);
int rendering_forward_manager_01_set_callback(rendering_forward_manager_01_t* ctx);
int rendering_forward_manager_01_get_memory_usage(rendering_forward_manager_01_t* ctx);
int rendering_forward_manager_01_optimize(rendering_forward_manager_01_t* ctx);
int rendering_forward_manager_01_debug_print(rendering_forward_manager_01_t* ctx);

/* Module functions */
int rendering_forward_manager_01_module_init(void);
int rendering_forward_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_MANAGER_01_H */
