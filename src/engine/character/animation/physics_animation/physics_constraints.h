/*
 * physics_constraints.h
 * Animation physics constraints
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_PHYSICS_CONSTRAINTS_H
#define ANIMATION_PHYSICS_CONSTRAINTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_physics_constraints_handle {
    uint32_t id;
} animation_physics_constraints_handle_t;

typedef struct animation_physics_constraints_desc {
    uint32_t flags;
    void* user_data;
} animation_physics_constraints_desc_t;

typedef struct animation_physics_constraints_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_physics_constraints_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_physics_constraints_init(void);
void animation_physics_constraints_shutdown(void);

/* Lifecycle */
int animation_physics_constraints_create(animation_physics_constraints_handle_t* out_handle, const animation_physics_constraints_desc_t* desc);
void animation_physics_constraints_destroy(animation_physics_constraints_handle_t handle);

/* Operations */
int animation_physics_constraints_update(animation_physics_constraints_handle_t handle, const void* data, size_t size);
bool animation_physics_constraints_is_valid(animation_physics_constraints_handle_t handle);
int animation_physics_constraints_get_info(animation_physics_constraints_handle_t handle, animation_physics_constraints_info_t* out_info);
void animation_physics_constraints_mark_dirty(animation_physics_constraints_handle_t handle);
int animation_physics_constraints_process_pending(void);

/* Serialization */
size_t animation_physics_constraints_serialize(animation_physics_constraints_handle_t handle, void* buffer, size_t buffer_size);
int animation_physics_constraints_deserialize(animation_physics_constraints_handle_t* out_handle, const void* buffer, size_t buffer_size);

/* Statistics */
uint32_t animation_physics_constraints_get_count(void);
size_t animation_physics_constraints_get_memory_usage(void);
void animation_physics_constraints_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_PHYSICS_CONSTRAINTS_H */
