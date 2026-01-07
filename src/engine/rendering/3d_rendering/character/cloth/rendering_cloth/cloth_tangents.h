/*
 * cloth_tangents.h
 * Cloth tangent space
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SYSTEM_CLOTH_TANGENTS_H
#define CLOTH_SYSTEM_CLOTH_TANGENTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_system_cloth_tangents_handle {
    uint32_t id;
} cloth_system_cloth_tangents_handle_t;

typedef struct cloth_system_cloth_tangents_desc {
    uint32_t flags;
    void* user_data;
} cloth_system_cloth_tangents_desc_t;

typedef struct cloth_system_cloth_tangents_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} cloth_system_cloth_tangents_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int cloth_system_cloth_tangents_init(void);
void cloth_system_cloth_tangents_shutdown(void);

/* Lifecycle */
int cloth_system_cloth_tangents_create(cloth_system_cloth_tangents_handle_t* out_handle, const cloth_system_cloth_tangents_desc_t* desc);
void cloth_system_cloth_tangents_destroy(cloth_system_cloth_tangents_handle_t handle);

/* Operations */
int cloth_system_cloth_tangents_update(cloth_system_cloth_tangents_handle_t handle, const void* data, size_t size);
bool cloth_system_cloth_tangents_is_valid(cloth_system_cloth_tangents_handle_t handle);
int cloth_system_cloth_tangents_get_info(cloth_system_cloth_tangents_handle_t handle, cloth_system_cloth_tangents_info_t* out_info);
void cloth_system_cloth_tangents_mark_dirty(cloth_system_cloth_tangents_handle_t handle);
int cloth_system_cloth_tangents_process_pending(void);

/* Statistics */
uint32_t cloth_system_cloth_tangents_get_count(void);
size_t cloth_system_cloth_tangents_get_memory_usage(void);
void cloth_system_cloth_tangents_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SYSTEM_CLOTH_TANGENTS_H */
