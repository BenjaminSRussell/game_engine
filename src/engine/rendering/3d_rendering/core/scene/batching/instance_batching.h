/*
 * instance_batching.h
 * Instance batching
 *
 * Part of the Scene Management subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SCENE_MANAGEMENT_INSTANCE_BATCHING_H
#define SCENE_MANAGEMENT_INSTANCE_BATCHING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct scene_management_instance_batching_handle {
    uint32_t id;
} scene_management_instance_batching_handle_t;

typedef struct scene_management_instance_batching_desc {
    uint32_t flags;
    void* user_data;
} scene_management_instance_batching_desc_t;

typedef struct scene_management_instance_batching_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} scene_management_instance_batching_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int scene_management_instance_batching_init(void);
void scene_management_instance_batching_shutdown(void);

/* Lifecycle */
int scene_management_instance_batching_create(scene_management_instance_batching_handle_t* out_handle, const scene_management_instance_batching_desc_t* desc);
void scene_management_instance_batching_destroy(scene_management_instance_batching_handle_t handle);

/* Operations */
int scene_management_instance_batching_update(scene_management_instance_batching_handle_t handle, const void* data, size_t size);
bool scene_management_instance_batching_is_valid(scene_management_instance_batching_handle_t handle);
int scene_management_instance_batching_get_info(scene_management_instance_batching_handle_t handle, scene_management_instance_batching_info_t* out_info);
void scene_management_instance_batching_mark_dirty(scene_management_instance_batching_handle_t handle);
int scene_management_instance_batching_process_pending(void);

/* Statistics */
uint32_t scene_management_instance_batching_get_count(void);
size_t scene_management_instance_batching_get_memory_usage(void);
void scene_management_instance_batching_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_MANAGEMENT_INSTANCE_BATCHING_H */
