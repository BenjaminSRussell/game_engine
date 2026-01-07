/*
 * bandwidth_counter.h
 * Bandwidth measurement
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_BANDWIDTH_COUNTER_H
#define PROFILING_BANDWIDTH_COUNTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_bandwidth_counter_handle {
    uint32_t id;
} profiling_bandwidth_counter_handle_t;

typedef struct profiling_bandwidth_counter_desc {
    uint32_t flags;
    void* user_data;
} profiling_bandwidth_counter_desc_t;

typedef struct profiling_bandwidth_counter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_bandwidth_counter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_bandwidth_counter_init(void);
void profiling_bandwidth_counter_shutdown(void);

/* Lifecycle */
int profiling_bandwidth_counter_create(profiling_bandwidth_counter_handle_t* out_handle, const profiling_bandwidth_counter_desc_t* desc);
void profiling_bandwidth_counter_destroy(profiling_bandwidth_counter_handle_t handle);

/* Operations */
int profiling_bandwidth_counter_update(profiling_bandwidth_counter_handle_t handle, const void* data, size_t size);
bool profiling_bandwidth_counter_is_valid(profiling_bandwidth_counter_handle_t handle);
int profiling_bandwidth_counter_get_info(profiling_bandwidth_counter_handle_t handle, profiling_bandwidth_counter_info_t* out_info);
void profiling_bandwidth_counter_mark_dirty(profiling_bandwidth_counter_handle_t handle);
int profiling_bandwidth_counter_process_pending(void);

/* Statistics */
uint32_t profiling_bandwidth_counter_get_count(void);
size_t profiling_bandwidth_counter_get_memory_usage(void);
void profiling_bandwidth_counter_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_BANDWIDTH_COUNTER_H */
