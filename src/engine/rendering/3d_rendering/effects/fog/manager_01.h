/*
 * effects_fog_manager_01.h
 *
 * Header file for effects_fog_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_FOG_MANAGER_01_H
#define EFFECTS_FOG_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_fog_manager_01 effects_fog_manager_01_t;
typedef struct effects_fog_manager_01_desc effects_fog_manager_01_desc_t;
typedef struct effects_fog_manager_01_stats effects_fog_manager_01_stats_t;

/* Creation and destruction */
int effects_fog_manager_01_create(effects_fog_manager_01_t** out_ctx, const effects_fog_manager_01_desc_t* desc);
int effects_fog_manager_01_destroy(effects_fog_manager_01_t* ctx);

/* Core operations */
int effects_fog_manager_01_init(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_shutdown(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_update(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_create(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_destroy(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_get(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_set(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_reset(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_validate(effects_fog_manager_01_t* ctx, void* params);
int effects_fog_manager_01_flush(effects_fog_manager_01_t* ctx, void* params);

/* Utility functions */
int effects_fog_manager_01_get_stats(effects_fog_manager_01_t* ctx);
int effects_fog_manager_01_set_callback(effects_fog_manager_01_t* ctx);
int effects_fog_manager_01_get_memory_usage(effects_fog_manager_01_t* ctx);
int effects_fog_manager_01_optimize(effects_fog_manager_01_t* ctx);
int effects_fog_manager_01_debug_print(effects_fog_manager_01_t* ctx);

/* Module functions */
int effects_fog_manager_01_module_init(void);
int effects_fog_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_FOG_MANAGER_01_H */
