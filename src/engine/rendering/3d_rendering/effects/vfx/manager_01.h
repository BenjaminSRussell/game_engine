/*
 * effects_vfx_manager_01.h
 *
 * Header file for effects_vfx_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_MANAGER_01_H
#define EFFECTS_VFX_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_vfx_manager_01 effects_vfx_manager_01_t;
typedef struct effects_vfx_manager_01_desc effects_vfx_manager_01_desc_t;
typedef struct effects_vfx_manager_01_stats effects_vfx_manager_01_stats_t;

/* Creation and destruction */
int effects_vfx_manager_01_create(effects_vfx_manager_01_t** out_ctx, const effects_vfx_manager_01_desc_t* desc);
int effects_vfx_manager_01_destroy(effects_vfx_manager_01_t* ctx);

/* Core operations */
int effects_vfx_manager_01_init(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_shutdown(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_update(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_create(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_destroy(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_get(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_set(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_reset(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_validate(effects_vfx_manager_01_t* ctx, void* params);
int effects_vfx_manager_01_flush(effects_vfx_manager_01_t* ctx, void* params);

/* Utility functions */
int effects_vfx_manager_01_get_stats(effects_vfx_manager_01_t* ctx);
int effects_vfx_manager_01_set_callback(effects_vfx_manager_01_t* ctx);
int effects_vfx_manager_01_get_memory_usage(effects_vfx_manager_01_t* ctx);
int effects_vfx_manager_01_optimize(effects_vfx_manager_01_t* ctx);
int effects_vfx_manager_01_debug_print(effects_vfx_manager_01_t* ctx);

/* Module functions */
int effects_vfx_manager_01_module_init(void);
int effects_vfx_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_MANAGER_01_H */
