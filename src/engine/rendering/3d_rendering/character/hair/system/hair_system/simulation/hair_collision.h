/*
 * hair_collision.h
 * Hair collision
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef HAIR_SYSTEM_HAIR_COLLISION_H
#define HAIR_SYSTEM_HAIR_COLLISION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hair_system_hair_collision_handle {
    uint32_t id;
} hair_system_hair_collision_handle_t;

typedef struct hair_system_hair_collision_desc {
    uint32_t flags;
    void* user_data;
} hair_system_hair_collision_desc_t;

typedef struct hair_system_hair_collision_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} hair_system_hair_collision_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int hair_system_hair_collision_init(void);
void hair_system_hair_collision_shutdown(void);

/* Lifecycle */
int hair_system_hair_collision_create(hair_system_hair_collision_handle_t* out_handle, const hair_system_hair_collision_desc_t* desc);
void hair_system_hair_collision_destroy(hair_system_hair_collision_handle_t handle);

/* Operations */
int hair_system_hair_collision_update(hair_system_hair_collision_handle_t handle, const void* data, size_t size);
bool hair_system_hair_collision_is_valid(hair_system_hair_collision_handle_t handle);
int hair_system_hair_collision_get_info(hair_system_hair_collision_handle_t handle, hair_system_hair_collision_info_t* out_info);
void hair_system_hair_collision_mark_dirty(hair_system_hair_collision_handle_t handle);
int hair_system_hair_collision_process_pending(void);

/* Statistics */
uint32_t hair_system_hair_collision_get_count(void);
size_t hair_system_hair_collision_get_memory_usage(void);
void hair_system_hair_collision_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* HAIR_SYSTEM_HAIR_COLLISION_H */
