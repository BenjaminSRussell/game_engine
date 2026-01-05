/*
 * animation_blend_manager_01.h
 *
 * Header file for animation_blend_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_BLEND_MANAGER_01_H
#define ANIMATION_BLEND_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_blend_manager_01 animation_blend_manager_01_t;
typedef struct animation_blend_manager_01_desc animation_blend_manager_01_desc_t;
typedef struct animation_blend_manager_01_stats animation_blend_manager_01_stats_t;

/* Creation and destruction */
int animation_blend_manager_01_create(animation_blend_manager_01_t** out_ctx, const animation_blend_manager_01_desc_t* desc);
int animation_blend_manager_01_destroy(animation_blend_manager_01_t* ctx);

/* Core operations */
int animation_blend_manager_01_init(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_shutdown(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_update(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_create(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_destroy(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_get(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_set(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_reset(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_validate(animation_blend_manager_01_t* ctx, void* params);
int animation_blend_manager_01_flush(animation_blend_manager_01_t* ctx, void* params);

/* Utility functions */
int animation_blend_manager_01_get_stats(animation_blend_manager_01_t* ctx);
int animation_blend_manager_01_set_callback(animation_blend_manager_01_t* ctx);
int animation_blend_manager_01_get_memory_usage(animation_blend_manager_01_t* ctx);
int animation_blend_manager_01_optimize(animation_blend_manager_01_t* ctx);
int animation_blend_manager_01_debug_print(animation_blend_manager_01_t* ctx);

/* Module functions */
int animation_blend_manager_01_module_init(void);
int animation_blend_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_BLEND_MANAGER_01_H */
