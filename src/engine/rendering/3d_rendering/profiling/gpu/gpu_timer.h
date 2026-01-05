/*
 * gpu_timer.h
 * GPU timestamp queries
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_GPU_TIMER_H
#define PROFILING_GPU_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_gpu_timer_handle {
    uint32_t id;
} profiling_gpu_timer_handle_t;

typedef struct profiling_gpu_timer_desc {
    uint32_t flags;
    void* user_data;
} profiling_gpu_timer_desc_t;

typedef struct profiling_gpu_timer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_gpu_timer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_gpu_timer_init(void);
void profiling_gpu_timer_shutdown(void);

/* Lifecycle */
int profiling_gpu_timer_create(profiling_gpu_timer_handle_t* out_handle, const profiling_gpu_timer_desc_t* desc);
void profiling_gpu_timer_destroy(profiling_gpu_timer_handle_t handle);

/* Operations */
int profiling_gpu_timer_update(profiling_gpu_timer_handle_t handle, const void* data, size_t size);
bool profiling_gpu_timer_is_valid(profiling_gpu_timer_handle_t handle);
int profiling_gpu_timer_get_info(profiling_gpu_timer_handle_t handle, profiling_gpu_timer_info_t* out_info);
void profiling_gpu_timer_mark_dirty(profiling_gpu_timer_handle_t handle);
int profiling_gpu_timer_process_pending(void);

/* Statistics */
uint32_t profiling_gpu_timer_get_count(void);
size_t profiling_gpu_timer_get_memory_usage(void);
void profiling_gpu_timer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_GPU_TIMER_H */
