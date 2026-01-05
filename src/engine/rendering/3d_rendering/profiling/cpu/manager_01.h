/*
 * profiling_cpu_manager_01.h
 *
 * Header file for profiling_cpu_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_CPU_MANAGER_01_H
#define PROFILING_CPU_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_cpu_manager_01 profiling_cpu_manager_01_t;
typedef struct profiling_cpu_manager_01_desc profiling_cpu_manager_01_desc_t;
typedef struct profiling_cpu_manager_01_stats profiling_cpu_manager_01_stats_t;

/* Creation and destruction */
int profiling_cpu_manager_01_create(profiling_cpu_manager_01_t** out_ctx, const profiling_cpu_manager_01_desc_t* desc);
int profiling_cpu_manager_01_destroy(profiling_cpu_manager_01_t* ctx);

/* Core operations */
int profiling_cpu_manager_01_init(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_shutdown(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_update(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_create(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_destroy(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_get(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_set(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_reset(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_validate(profiling_cpu_manager_01_t* ctx, void* params);
int profiling_cpu_manager_01_flush(profiling_cpu_manager_01_t* ctx, void* params);

/* Utility functions */
int profiling_cpu_manager_01_get_stats(profiling_cpu_manager_01_t* ctx);
int profiling_cpu_manager_01_set_callback(profiling_cpu_manager_01_t* ctx);
int profiling_cpu_manager_01_get_memory_usage(profiling_cpu_manager_01_t* ctx);
int profiling_cpu_manager_01_optimize(profiling_cpu_manager_01_t* ctx);
int profiling_cpu_manager_01_debug_print(profiling_cpu_manager_01_t* ctx);

/* Module functions */
int profiling_cpu_manager_01_module_init(void);
int profiling_cpu_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_CPU_MANAGER_01_H */
