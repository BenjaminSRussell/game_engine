/*
 * level_bounds.h
 * Level bounds
 *
 * Part of the Scene Management subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SCENE_MANAGEMENT_LEVEL_BOUNDS_H
#define SCENE_MANAGEMENT_LEVEL_BOUNDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct scene_management_level_bounds_handle {
    uint32_t id;
} scene_management_level_bounds_handle_t;

typedef struct scene_management_level_bounds_desc {
    uint32_t flags;
    void* user_data;
} scene_management_level_bounds_desc_t;

typedef struct scene_management_level_bounds_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} scene_management_level_bounds_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int scene_management_level_bounds_init(void);
void scene_management_level_bounds_shutdown(void);

/* Lifecycle */
int scene_management_level_bounds_create(scene_management_level_bounds_handle_t* out_handle, const scene_management_level_bounds_desc_t* desc);
void scene_management_level_bounds_destroy(scene_management_level_bounds_handle_t handle);

/* Operations */
int scene_management_level_bounds_update(scene_management_level_bounds_handle_t handle, const void* data, size_t size);
bool scene_management_level_bounds_is_valid(scene_management_level_bounds_handle_t handle);
int scene_management_level_bounds_get_info(scene_management_level_bounds_handle_t handle, scene_management_level_bounds_info_t* out_info);
void scene_management_level_bounds_mark_dirty(scene_management_level_bounds_handle_t handle);
int scene_management_level_bounds_process_pending(void);

/* Statistics */
uint32_t scene_management_level_bounds_get_count(void);
size_t scene_management_level_bounds_get_memory_usage(void);
void scene_management_level_bounds_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_MANAGEMENT_LEVEL_BOUNDS_H */
