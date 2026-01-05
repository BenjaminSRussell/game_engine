/*
 * profiling_counters_processor_04.h
 *
 * Header file for profiling_counters_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_COUNTERS_PROCESSOR_04_H
#define PROFILING_COUNTERS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_counters_processor_04 profiling_counters_processor_04_t;
typedef struct profiling_counters_processor_04_desc profiling_counters_processor_04_desc_t;
typedef struct profiling_counters_processor_04_stats profiling_counters_processor_04_stats_t;

/* Creation and destruction */
int profiling_counters_processor_04_create(profiling_counters_processor_04_t** out_ctx, const profiling_counters_processor_04_desc_t* desc);
int profiling_counters_processor_04_destroy(profiling_counters_processor_04_t* ctx);

/* Core operations */
int profiling_counters_processor_04_process_batch(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_process_single(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_transform(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_filter(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_aggregate(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_dispatch(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_finalize(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_validate_input(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_optimize_output(profiling_counters_processor_04_t* ctx, void* params);
int profiling_counters_processor_04_profile(profiling_counters_processor_04_t* ctx, void* params);

/* Utility functions */
int profiling_counters_processor_04_get_stats(profiling_counters_processor_04_t* ctx);
int profiling_counters_processor_04_set_callback(profiling_counters_processor_04_t* ctx);
int profiling_counters_processor_04_get_memory_usage(profiling_counters_processor_04_t* ctx);
int profiling_counters_processor_04_optimize(profiling_counters_processor_04_t* ctx);
int profiling_counters_processor_04_debug_print(profiling_counters_processor_04_t* ctx);

/* Module functions */
int profiling_counters_processor_04_module_init(void);
int profiling_counters_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_COUNTERS_PROCESSOR_04_H */
