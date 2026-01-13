/*
 * cloth_collision.h
 * Cloth collision
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SYSTEM_CLOTH_COLLISION_H
#define CLOTH_SYSTEM_CLOTH_COLLISION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_system_cloth_collision_handle {
    uint32_t id;
} cloth_system_cloth_collision_handle_t;

typedef struct cloth_system_cloth_collision_desc {
    uint32_t flags;
    void* user_data;
} cloth_system_cloth_collision_desc_t;

typedef struct cloth_system_cloth_collision_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} cloth_system_cloth_collision_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int cloth_system_cloth_collision_init(void);
void cloth_system_cloth_collision_shutdown(void);

/* Lifecycle */
int cloth_system_cloth_collision_create(cloth_system_cloth_collision_handle_t* out_handle, const cloth_system_cloth_collision_desc_t* desc);
void cloth_system_cloth_collision_destroy(cloth_system_cloth_collision_handle_t handle);

/* Operations */
int cloth_system_cloth_collision_update(cloth_system_cloth_collision_handle_t handle, const void* data, size_t size);
bool cloth_system_cloth_collision_is_valid(cloth_system_cloth_collision_handle_t handle);
int cloth_system_cloth_collision_get_info(cloth_system_cloth_collision_handle_t handle, cloth_system_cloth_collision_info_t* out_info);
void cloth_system_cloth_collision_mark_dirty(cloth_system_cloth_collision_handle_t handle);
int cloth_system_cloth_collision_process_pending(void);

/* Statistics */
uint32_t cloth_system_cloth_collision_get_count(void);
size_t cloth_system_cloth_collision_get_memory_usage(void);
void cloth_system_cloth_collision_debug_print(void);

/* Serialization */
int cloth_system_cloth_collision_serialize(cloth_system_cloth_collision_handle_t handle, void** out_data, size_t* out_size);
int cloth_system_cloth_collision_deserialize(const void* data, size_t size, cloth_system_cloth_collision_handle_t* out_handle);

/* Async Operations */
int cloth_system_cloth_collision_update_async(cloth_system_cloth_collision_handle_t handle, const void* data, size_t size, void (*callback)(cloth_system_cloth_collision_handle_t, int));

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SYSTEM_CLOTH_COLLISION_H */
