/*
 * cache_profiler.h
 * Cache hit profiling
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_CACHE_PROFILER_H
#define PROFILING_CACHE_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_cache_profiler_handle {
    uint32_t id;
} profiling_cache_profiler_handle_t;

typedef struct profiling_cache_profiler_desc {
    uint32_t flags;
    void* user_data;
} profiling_cache_profiler_desc_t;

typedef struct profiling_cache_profiler_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_cache_profiler_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_cache_profiler_init(void);
void profiling_cache_profiler_shutdown(void);

/* Lifecycle */
int profiling_cache_profiler_create(profiling_cache_profiler_handle_t* out_handle, const profiling_cache_profiler_desc_t* desc);
void profiling_cache_profiler_destroy(profiling_cache_profiler_handle_t handle);

/* Operations */
int profiling_cache_profiler_update(profiling_cache_profiler_handle_t handle, const void* data, size_t size);
bool profiling_cache_profiler_is_valid(profiling_cache_profiler_handle_t handle);
int profiling_cache_profiler_get_info(profiling_cache_profiler_handle_t handle, profiling_cache_profiler_info_t* out_info);
void profiling_cache_profiler_mark_dirty(profiling_cache_profiler_handle_t handle);
int profiling_cache_profiler_process_pending(void);

/* Statistics */
uint32_t profiling_cache_profiler_get_count(void);
size_t profiling_cache_profiler_get_memory_usage(void);
void profiling_cache_profiler_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_CACHE_PROFILER_H */
