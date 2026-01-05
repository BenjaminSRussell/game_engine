/*
 * frame_timer.h
 * Frame time measurement
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_FRAME_TIMER_H
#define PROFILING_FRAME_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_frame_timer_handle {
    uint32_t id;
} profiling_frame_timer_handle_t;

typedef struct profiling_frame_timer_desc {
    uint32_t flags;
    void* user_data;
} profiling_frame_timer_desc_t;

typedef struct profiling_frame_timer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_frame_timer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_frame_timer_init(void);
void profiling_frame_timer_shutdown(void);

/* Lifecycle */
int profiling_frame_timer_create(profiling_frame_timer_handle_t* out_handle, const profiling_frame_timer_desc_t* desc);
void profiling_frame_timer_destroy(profiling_frame_timer_handle_t handle);

/* Operations */
int profiling_frame_timer_update(profiling_frame_timer_handle_t handle, const void* data, size_t size);
bool profiling_frame_timer_is_valid(profiling_frame_timer_handle_t handle);
int profiling_frame_timer_get_info(profiling_frame_timer_handle_t handle, profiling_frame_timer_info_t* out_info);
void profiling_frame_timer_mark_dirty(profiling_frame_timer_handle_t handle);
int profiling_frame_timer_process_pending(void);

/* Statistics */
uint32_t profiling_frame_timer_get_count(void);
size_t profiling_frame_timer_get_memory_usage(void);
void profiling_frame_timer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_FRAME_TIMER_H */
