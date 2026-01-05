/*
 * profiling_memory_processor_04.h
 *
 * Header file for profiling_memory_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_MEMORY_PROCESSOR_04_H
#define PROFILING_MEMORY_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_memory_processor_04 profiling_memory_processor_04_t;
typedef struct profiling_memory_processor_04_desc profiling_memory_processor_04_desc_t;
typedef struct profiling_memory_processor_04_stats profiling_memory_processor_04_stats_t;

/* Creation and destruction */
int profiling_memory_processor_04_create(profiling_memory_processor_04_t** out_ctx, const profiling_memory_processor_04_desc_t* desc);
int profiling_memory_processor_04_destroy(profiling_memory_processor_04_t* ctx);

/* Core operations */
int profiling_memory_processor_04_process_batch(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_process_single(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_transform(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_filter(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_aggregate(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_dispatch(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_finalize(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_validate_input(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_optimize_output(profiling_memory_processor_04_t* ctx, void* params);
int profiling_memory_processor_04_profile(profiling_memory_processor_04_t* ctx, void* params);

/* Utility functions */
int profiling_memory_processor_04_get_stats(profiling_memory_processor_04_t* ctx);
int profiling_memory_processor_04_set_callback(profiling_memory_processor_04_t* ctx);
int profiling_memory_processor_04_get_memory_usage(profiling_memory_processor_04_t* ctx);
int profiling_memory_processor_04_optimize(profiling_memory_processor_04_t* ctx);
int profiling_memory_processor_04_debug_print(profiling_memory_processor_04_t* ctx);

/* Module functions */
int profiling_memory_processor_04_module_init(void);
int profiling_memory_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_MEMORY_PROCESSOR_04_H */
