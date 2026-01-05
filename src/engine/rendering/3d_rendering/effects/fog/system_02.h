/*
 * effects_fog_system_02.h
 *
 * Header file for effects_fog_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_FOG_SYSTEM_02_H
#define EFFECTS_FOG_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_fog_system_02 effects_fog_system_02_t;
typedef struct effects_fog_system_02_desc effects_fog_system_02_desc_t;
typedef struct effects_fog_system_02_stats effects_fog_system_02_stats_t;

/* Creation and destruction */
int effects_fog_system_02_create(effects_fog_system_02_t** out_ctx, const effects_fog_system_02_desc_t* desc);
int effects_fog_system_02_destroy(effects_fog_system_02_t* ctx);

/* Core operations */
int effects_fog_system_02_create_system(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_destroy_system(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_tick(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_process(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_submit(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_execute(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_sync(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_query(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_configure(effects_fog_system_02_t* ctx, void* params);
int effects_fog_system_02_optimize(effects_fog_system_02_t* ctx, void* params);

/* Utility functions */
int effects_fog_system_02_get_stats(effects_fog_system_02_t* ctx);
int effects_fog_system_02_set_callback(effects_fog_system_02_t* ctx);
int effects_fog_system_02_get_memory_usage(effects_fog_system_02_t* ctx);
int effects_fog_system_02_optimize(effects_fog_system_02_t* ctx);
int effects_fog_system_02_debug_print(effects_fog_system_02_t* ctx);

/* Module functions */
int effects_fog_system_02_module_init(void);
int effects_fog_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_FOG_SYSTEM_02_H */
