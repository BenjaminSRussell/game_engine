/*
 * profiling_analysis_processor_04.h
 *
 * Header file for profiling_analysis_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_ANALYSIS_PROCESSOR_04_H
#define PROFILING_ANALYSIS_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_analysis_processor_04 profiling_analysis_processor_04_t;
typedef struct profiling_analysis_processor_04_desc profiling_analysis_processor_04_desc_t;
typedef struct profiling_analysis_processor_04_stats profiling_analysis_processor_04_stats_t;

/* Creation and destruction */
int profiling_analysis_processor_04_create(profiling_analysis_processor_04_t** out_ctx, const profiling_analysis_processor_04_desc_t* desc);
int profiling_analysis_processor_04_destroy(profiling_analysis_processor_04_t* ctx);

/* Core operations */
int profiling_analysis_processor_04_process_batch(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_process_single(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_transform(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_filter(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_aggregate(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_dispatch(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_finalize(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_validate_input(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_optimize_output(profiling_analysis_processor_04_t* ctx, void* params);
int profiling_analysis_processor_04_profile(profiling_analysis_processor_04_t* ctx, void* params);

/* Utility functions */
int profiling_analysis_processor_04_get_stats(profiling_analysis_processor_04_t* ctx);
int profiling_analysis_processor_04_set_callback(profiling_analysis_processor_04_t* ctx);
int profiling_analysis_processor_04_get_memory_usage(profiling_analysis_processor_04_t* ctx);
int profiling_analysis_processor_04_optimize(profiling_analysis_processor_04_t* ctx);
int profiling_analysis_processor_04_debug_print(profiling_analysis_processor_04_t* ctx);

/* Module functions */
int profiling_analysis_processor_04_module_init(void);
int profiling_analysis_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_ANALYSIS_PROCESSOR_04_H */
