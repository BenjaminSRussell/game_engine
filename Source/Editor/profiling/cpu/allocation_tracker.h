/*
 * allocation_tracker.h
 * Allocation tracking
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_ALLOCATION_TRACKER_H
#define PROFILING_ALLOCATION_TRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_allocation_tracker_handle {
    uint32_t id;
} profiling_allocation_tracker_handle_t;

typedef struct profiling_allocation_tracker_desc {
    uint32_t flags;
    void* user_data;
} profiling_allocation_tracker_desc_t;

typedef struct profiling_allocation_tracker_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_allocation_tracker_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_allocation_tracker_init(void);
void profiling_allocation_tracker_shutdown(void);

/* Lifecycle */
int profiling_allocation_tracker_create(profiling_allocation_tracker_handle_t* out_handle, const profiling_allocation_tracker_desc_t* desc);
void profiling_allocation_tracker_destroy(profiling_allocation_tracker_handle_t handle);

/* Operations */
int profiling_allocation_tracker_update(profiling_allocation_tracker_handle_t handle, const void* data, size_t size);
bool profiling_allocation_tracker_is_valid(profiling_allocation_tracker_handle_t handle);
int profiling_allocation_tracker_get_info(profiling_allocation_tracker_handle_t handle, profiling_allocation_tracker_info_t* out_info);
void profiling_allocation_tracker_mark_dirty(profiling_allocation_tracker_handle_t handle);
int profiling_allocation_tracker_process_pending(void);

/* Statistics */
uint32_t profiling_allocation_tracker_get_count(void);
size_t profiling_allocation_tracker_get_memory_usage(void);
void profiling_allocation_tracker_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_ALLOCATION_TRACKER_H */
