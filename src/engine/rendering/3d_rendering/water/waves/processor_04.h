/*
 * water_waves_processor_04.h
 *
 * Header file for water_waves_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef WATER_WAVES_PROCESSOR_04_H
#define WATER_WAVES_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct water_waves_processor_04 water_waves_processor_04_t;
typedef struct water_waves_processor_04_desc water_waves_processor_04_desc_t;
typedef struct water_waves_processor_04_stats water_waves_processor_04_stats_t;

/* Creation and destruction */
int water_waves_processor_04_create(water_waves_processor_04_t** out_ctx, const water_waves_processor_04_desc_t* desc);
int water_waves_processor_04_destroy(water_waves_processor_04_t* ctx);

/* Core operations */
int water_waves_processor_04_process_batch(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_process_single(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_transform(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_filter(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_aggregate(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_dispatch(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_finalize(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_validate_input(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_optimize_output(water_waves_processor_04_t* ctx, void* params);
int water_waves_processor_04_profile(water_waves_processor_04_t* ctx, void* params);

/* Utility functions */
int water_waves_processor_04_get_stats(water_waves_processor_04_t* ctx);
int water_waves_processor_04_set_callback(water_waves_processor_04_t* ctx);
int water_waves_processor_04_get_memory_usage(water_waves_processor_04_t* ctx);
int water_waves_processor_04_optimize(water_waves_processor_04_t* ctx);
int water_waves_processor_04_debug_print(water_waves_processor_04_t* ctx);

/* Module functions */
int water_waves_processor_04_module_init(void);
int water_waves_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WAVES_PROCESSOR_04_H */
