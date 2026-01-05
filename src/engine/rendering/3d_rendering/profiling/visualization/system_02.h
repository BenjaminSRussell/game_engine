/*
 * profiling_visualization_system_02.h
 *
 * Header file for profiling_visualization_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_VISUALIZATION_SYSTEM_02_H
#define PROFILING_VISUALIZATION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_visualization_system_02 profiling_visualization_system_02_t;
typedef struct profiling_visualization_system_02_desc profiling_visualization_system_02_desc_t;
typedef struct profiling_visualization_system_02_stats profiling_visualization_system_02_stats_t;

/* Creation and destruction */
int profiling_visualization_system_02_create(profiling_visualization_system_02_t** out_ctx, const profiling_visualization_system_02_desc_t* desc);
int profiling_visualization_system_02_destroy(profiling_visualization_system_02_t* ctx);

/* Core operations */
int profiling_visualization_system_02_create_system(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_destroy_system(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_tick(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_process(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_submit(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_execute(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_sync(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_query(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_configure(profiling_visualization_system_02_t* ctx, void* params);
int profiling_visualization_system_02_optimize(profiling_visualization_system_02_t* ctx, void* params);

/* Utility functions */
int profiling_visualization_system_02_get_stats(profiling_visualization_system_02_t* ctx);
int profiling_visualization_system_02_set_callback(profiling_visualization_system_02_t* ctx);
int profiling_visualization_system_02_get_memory_usage(profiling_visualization_system_02_t* ctx);
int profiling_visualization_system_02_optimize(profiling_visualization_system_02_t* ctx);
int profiling_visualization_system_02_debug_print(profiling_visualization_system_02_t* ctx);

/* Module functions */
int profiling_visualization_system_02_module_init(void);
int profiling_visualization_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_VISUALIZATION_SYSTEM_02_H */
