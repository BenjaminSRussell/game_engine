/*
 * perf_overlay.h
 * Performance overlay
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_PERF_OVERLAY_H
#define PROFILING_PERF_OVERLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_perf_overlay_handle {
    uint32_t id;
} profiling_perf_overlay_handle_t;

typedef struct profiling_perf_overlay_desc {
    uint32_t flags;
    void* user_data;
} profiling_perf_overlay_desc_t;

typedef struct profiling_perf_overlay_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_perf_overlay_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_perf_overlay_init(void);
void profiling_perf_overlay_shutdown(void);

/* Lifecycle */
int profiling_perf_overlay_create(profiling_perf_overlay_handle_t* out_handle, const profiling_perf_overlay_desc_t* desc);
void profiling_perf_overlay_destroy(profiling_perf_overlay_handle_t handle);

/* Operations */
int profiling_perf_overlay_update(profiling_perf_overlay_handle_t handle, const void* data, size_t size);
bool profiling_perf_overlay_is_valid(profiling_perf_overlay_handle_t handle);
int profiling_perf_overlay_get_info(profiling_perf_overlay_handle_t handle, profiling_perf_overlay_info_t* out_info);
void profiling_perf_overlay_mark_dirty(profiling_perf_overlay_handle_t handle);
int profiling_perf_overlay_process_pending(void);

/* Statistics */
uint32_t profiling_perf_overlay_get_count(void);
size_t profiling_perf_overlay_get_memory_usage(void);
void profiling_perf_overlay_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_PERF_OVERLAY_H */
