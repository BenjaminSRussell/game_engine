/*
 * trail_points.h
 * Trail point generation
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_TRAIL_POINTS_H
#define EFFECTS_TRAIL_POINTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_trail_points_handle {
    uint32_t id;
} effects_trail_points_handle_t;

typedef struct effects_trail_points_desc {
    uint32_t flags;
    void* user_data;
} effects_trail_points_desc_t;

typedef struct effects_trail_points_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_trail_points_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_trail_points_init(void);
void effects_trail_points_shutdown(void);

/* Lifecycle */
int effects_trail_points_create(effects_trail_points_handle_t* out_handle, const effects_trail_points_desc_t* desc);
void effects_trail_points_destroy(effects_trail_points_handle_t handle);

/* Operations */
int effects_trail_points_update(effects_trail_points_handle_t handle, const void* data, size_t size);
bool effects_trail_points_is_valid(effects_trail_points_handle_t handle);
int effects_trail_points_get_info(effects_trail_points_handle_t handle, effects_trail_points_info_t* out_info);
void effects_trail_points_mark_dirty(effects_trail_points_handle_t handle);
int effects_trail_points_process_pending(void);

/* Statistics */
uint32_t effects_trail_points_get_count(void);
size_t effects_trail_points_get_memory_usage(void);
void effects_trail_points_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_TRAIL_POINTS_H */
