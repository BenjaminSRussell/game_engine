/*
 * profiling_analysis_system_02.h
 *
 * Header file for profiling_analysis_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_ANALYSIS_SYSTEM_02_H
#define PROFILING_ANALYSIS_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_analysis_system_02 profiling_analysis_system_02_t;
typedef struct profiling_analysis_system_02_desc profiling_analysis_system_02_desc_t;
typedef struct profiling_analysis_system_02_stats profiling_analysis_system_02_stats_t;

/* Creation and destruction */
int profiling_analysis_system_02_create(profiling_analysis_system_02_t** out_ctx, const profiling_analysis_system_02_desc_t* desc);
int profiling_analysis_system_02_destroy(profiling_analysis_system_02_t* ctx);

/* Core operations */
int profiling_analysis_system_02_create_system(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_destroy_system(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_tick(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_process(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_submit(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_execute(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_sync(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_query(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_configure(profiling_analysis_system_02_t* ctx, void* params);
int profiling_analysis_system_02_optimize(profiling_analysis_system_02_t* ctx, void* params);

/* Utility functions */
int profiling_analysis_system_02_get_stats(profiling_analysis_system_02_t* ctx);
int profiling_analysis_system_02_set_callback(profiling_analysis_system_02_t* ctx);
int profiling_analysis_system_02_get_memory_usage(profiling_analysis_system_02_t* ctx);
int profiling_analysis_system_02_optimize(profiling_analysis_system_02_t* ctx);
int profiling_analysis_system_02_debug_print(profiling_analysis_system_02_t* ctx);

/* Module functions */
int profiling_analysis_system_02_module_init(void);
int profiling_analysis_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_ANALYSIS_SYSTEM_02_H */
