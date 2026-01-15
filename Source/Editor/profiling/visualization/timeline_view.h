/*
 * timeline_view.h
 * Timeline visualization
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_TIMELINE_VIEW_H
#define PROFILING_TIMELINE_VIEW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_timeline_view_handle {
    uint32_t id;
} profiling_timeline_view_handle_t;

typedef struct profiling_timeline_view_desc {
    uint32_t flags;
    void* user_data;
} profiling_timeline_view_desc_t;

typedef struct profiling_timeline_view_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_timeline_view_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_timeline_view_init(void);
void profiling_timeline_view_shutdown(void);

/* Lifecycle */
int profiling_timeline_view_create(profiling_timeline_view_handle_t* out_handle, const profiling_timeline_view_desc_t* desc);
void profiling_timeline_view_destroy(profiling_timeline_view_handle_t handle);

/* Operations */
int profiling_timeline_view_update(profiling_timeline_view_handle_t handle, const void* data, size_t size);
bool profiling_timeline_view_is_valid(profiling_timeline_view_handle_t handle);
int profiling_timeline_view_get_info(profiling_timeline_view_handle_t handle, profiling_timeline_view_info_t* out_info);
void profiling_timeline_view_mark_dirty(profiling_timeline_view_handle_t handle);
int profiling_timeline_view_process_pending(void);

/* Statistics */
uint32_t profiling_timeline_view_get_count(void);
size_t profiling_timeline_view_get_memory_usage(void);
void profiling_timeline_view_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_TIMELINE_VIEW_H */
