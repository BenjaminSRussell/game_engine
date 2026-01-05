/*
 * effects_beams_processor_04.h
 *
 * Header file for effects_beams_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_BEAMS_PROCESSOR_04_H
#define EFFECTS_BEAMS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_beams_processor_04 effects_beams_processor_04_t;
typedef struct effects_beams_processor_04_desc effects_beams_processor_04_desc_t;
typedef struct effects_beams_processor_04_stats effects_beams_processor_04_stats_t;

/* Creation and destruction */
int effects_beams_processor_04_create(effects_beams_processor_04_t** out_ctx, const effects_beams_processor_04_desc_t* desc);
int effects_beams_processor_04_destroy(effects_beams_processor_04_t* ctx);

/* Core operations */
int effects_beams_processor_04_process_batch(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_process_single(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_transform(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_filter(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_aggregate(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_dispatch(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_finalize(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_validate_input(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_optimize_output(effects_beams_processor_04_t* ctx, void* params);
int effects_beams_processor_04_profile(effects_beams_processor_04_t* ctx, void* params);

/* Utility functions */
int effects_beams_processor_04_get_stats(effects_beams_processor_04_t* ctx);
int effects_beams_processor_04_set_callback(effects_beams_processor_04_t* ctx);
int effects_beams_processor_04_get_memory_usage(effects_beams_processor_04_t* ctx);
int effects_beams_processor_04_optimize(effects_beams_processor_04_t* ctx);
int effects_beams_processor_04_debug_print(effects_beams_processor_04_t* ctx);

/* Module functions */
int effects_beams_processor_04_module_init(void);
int effects_beams_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_BEAMS_PROCESSOR_04_H */
