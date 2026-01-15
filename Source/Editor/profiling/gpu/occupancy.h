/*
 * occupancy.h
 * Shader occupancy
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_OCCUPANCY_H
#define PROFILING_OCCUPANCY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_occupancy_handle {
    uint32_t id;
} profiling_occupancy_handle_t;

typedef struct profiling_occupancy_desc {
    uint32_t flags;
    void* user_data;
} profiling_occupancy_desc_t;

typedef struct profiling_occupancy_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_occupancy_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_occupancy_init(void);
void profiling_occupancy_shutdown(void);

/* Lifecycle */
int profiling_occupancy_create(profiling_occupancy_handle_t* out_handle, const profiling_occupancy_desc_t* desc);
void profiling_occupancy_destroy(profiling_occupancy_handle_t handle);

/* Operations */
int profiling_occupancy_update(profiling_occupancy_handle_t handle, const void* data, size_t size);
bool profiling_occupancy_is_valid(profiling_occupancy_handle_t handle);
int profiling_occupancy_get_info(profiling_occupancy_handle_t handle, profiling_occupancy_info_t* out_info);
void profiling_occupancy_mark_dirty(profiling_occupancy_handle_t handle);
int profiling_occupancy_process_pending(void);

/* Statistics */
uint32_t profiling_occupancy_get_count(void);
size_t profiling_occupancy_get_memory_usage(void);
void profiling_occupancy_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_OCCUPANCY_H */
