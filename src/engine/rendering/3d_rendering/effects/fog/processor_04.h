/*
 * effects_fog_processor_04.h
 *
 * Header file for effects_fog_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_FOG_PROCESSOR_04_H
#define EFFECTS_FOG_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_fog_processor_04 effects_fog_processor_04_t;
typedef struct effects_fog_processor_04_desc effects_fog_processor_04_desc_t;
typedef struct effects_fog_processor_04_stats effects_fog_processor_04_stats_t;

/* Creation and destruction */
int effects_fog_processor_04_create(effects_fog_processor_04_t** out_ctx, const effects_fog_processor_04_desc_t* desc);
int effects_fog_processor_04_destroy(effects_fog_processor_04_t* ctx);

/* Core operations */
int effects_fog_processor_04_process_batch(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_process_single(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_transform(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_filter(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_aggregate(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_dispatch(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_finalize(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_validate_input(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_optimize_output(effects_fog_processor_04_t* ctx, void* params);
int effects_fog_processor_04_profile(effects_fog_processor_04_t* ctx, void* params);

/* Utility functions */
int effects_fog_processor_04_get_stats(effects_fog_processor_04_t* ctx);
int effects_fog_processor_04_set_callback(effects_fog_processor_04_t* ctx);
int effects_fog_processor_04_get_memory_usage(effects_fog_processor_04_t* ctx);
int effects_fog_processor_04_optimize(effects_fog_processor_04_t* ctx);
int effects_fog_processor_04_debug_print(effects_fog_processor_04_t* ctx);

/* Module functions */
int effects_fog_processor_04_module_init(void);
int effects_fog_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_FOG_PROCESSOR_04_H */
