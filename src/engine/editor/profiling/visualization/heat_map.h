/*
 * heat_map.h
 * Performance heat map
 *
 * Part of the Profiling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PROFILING_HEAT_MAP_H
#define PROFILING_HEAT_MAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct profiling_heat_map_handle {
    uint32_t id;
} profiling_heat_map_handle_t;

typedef struct profiling_heat_map_desc {
    uint32_t flags;
    void* user_data;
} profiling_heat_map_desc_t;

typedef struct profiling_heat_map_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} profiling_heat_map_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int profiling_heat_map_init(void);
void profiling_heat_map_shutdown(void);

/* Lifecycle */
int profiling_heat_map_create(profiling_heat_map_handle_t* out_handle, const profiling_heat_map_desc_t* desc);
void profiling_heat_map_destroy(profiling_heat_map_handle_t handle);

/* Operations */
int profiling_heat_map_update(profiling_heat_map_handle_t handle, const void* data, size_t size);
bool profiling_heat_map_is_valid(profiling_heat_map_handle_t handle);
int profiling_heat_map_get_info(profiling_heat_map_handle_t handle, profiling_heat_map_info_t* out_info);
void profiling_heat_map_mark_dirty(profiling_heat_map_handle_t handle);
int profiling_heat_map_process_pending(void);

/* Statistics */
uint32_t profiling_heat_map_get_count(void);
size_t profiling_heat_map_get_memory_usage(void);
void profiling_heat_map_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILING_HEAT_MAP_H */
