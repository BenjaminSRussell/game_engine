/*
 * profiling_frame_manager_01.h
 *
 * Header file for profiling_frame_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PROFILING_FRAME_MANAGER_01_H
#define PROFILING_FRAME_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct profiling_frame_manager_01 profiling_frame_manager_01_t;
typedef struct profiling_frame_manager_01_desc profiling_frame_manager_01_desc_t;
typedef struct profiling_frame_manager_01_stats profiling_frame_manager_01_stats_t;

/* Creation and destruction */
int profiling_frame_manager_01_create(profiling_frame_manager_01_t** out_ctx, const profiling_frame_manager_01_desc_t* desc);
int profiling_frame_manager_01_destroy(profiling_frame_manager_01_t* ctx);

/* Core operations */
int profiling_frame_manager_01_init(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_shutdown(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_update(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_create(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_destroy(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_get(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_set(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_reset(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_validate(profiling_frame_manager_01_t* ctx, void* params);
int profiling_frame_manager_01_flush(profiling_frame_manager_01_t* ctx, void* params);

/* Utility functions */
int profiling_frame_manager_01_get_stats(profiling_frame_manager_01_t* ctx);
int profiling_frame_manager_01_set_callback(profiling_frame_manager_01_t* ctx);
int profiling_frame_manager_01_get_memory_usage(profiling_frame_manager_01_t* ctx);
int profiling_frame_manager_01_optimize(profiling_frame_manager_01_t* ctx);
int profiling_frame_manager_01_debug_print(profiling_frame_manager_01_t* ctx);

/* Module functions */
int profiling_frame_manager_01_module_init(void);
int profiling_frame_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_FRAME_MANAGER_01_H */
