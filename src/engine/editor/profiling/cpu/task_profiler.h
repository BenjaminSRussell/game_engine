/*
 * task_profiler.h
 * Task/job profiling
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_TASK_PROFILER_H
#define PROFILING_TASK_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_task_profiler_handle {
    uint32_t id;
} profiling_task_profiler_handle_t;

typedef struct profiling_task_profiler_desc {
    uint32_t flags;
    void* user_data;
} profiling_task_profiler_desc_t;

typedef struct profiling_task_profiler_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_task_profiler_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_task_profiler_init(void);
void profiling_task_profiler_shutdown(void);

/* Lifecycle */
int profiling_task_profiler_create(profiling_task_profiler_handle_t* out_handle, const profiling_task_profiler_desc_t* desc);
void profiling_task_profiler_destroy(profiling_task_profiler_handle_t handle);

/* Operations */
int profiling_task_profiler_update(profiling_task_profiler_handle_t handle, const void* data, size_t size);
bool profiling_task_profiler_is_valid(profiling_task_profiler_handle_t handle);
int profiling_task_profiler_get_info(profiling_task_profiler_handle_t handle, profiling_task_profiler_info_t* out_info);
void profiling_task_profiler_mark_dirty(profiling_task_profiler_handle_t handle);
int profiling_task_profiler_process_pending(void);

/* Statistics */
uint32_t profiling_task_profiler_get_count(void);
size_t profiling_task_profiler_get_memory_usage(void);
void profiling_task_profiler_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_TASK_PROFILER_H */
