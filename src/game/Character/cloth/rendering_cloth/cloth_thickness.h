/*
 * cloth_thickness.h
 * Cloth thickness
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SYSTEM_CLOTH_THICKNESS_H
#define CLOTH_SYSTEM_CLOTH_THICKNESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_system_cloth_thickness_handle {
    uint32_t id;
} cloth_system_cloth_thickness_handle_t;

typedef struct cloth_system_cloth_thickness_desc {
    uint32_t flags;
    void* user_data;
} cloth_system_cloth_thickness_desc_t;

typedef struct cloth_system_cloth_thickness_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} cloth_system_cloth_thickness_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int cloth_system_cloth_thickness_init(void);
void cloth_system_cloth_thickness_shutdown(void);

/* Lifecycle */
int cloth_system_cloth_thickness_create(cloth_system_cloth_thickness_handle_t* out_handle, const cloth_system_cloth_thickness_desc_t* desc);
void cloth_system_cloth_thickness_destroy(cloth_system_cloth_thickness_handle_t handle);

/* Operations */
int cloth_system_cloth_thickness_update(cloth_system_cloth_thickness_handle_t handle, const void* data, size_t size);
bool cloth_system_cloth_thickness_is_valid(cloth_system_cloth_thickness_handle_t handle);
int cloth_system_cloth_thickness_get_info(cloth_system_cloth_thickness_handle_t handle, cloth_system_cloth_thickness_info_t* out_info);
void cloth_system_cloth_thickness_mark_dirty(cloth_system_cloth_thickness_handle_t handle);
int cloth_system_cloth_thickness_process_pending(void);

/* Statistics */
uint32_t cloth_system_cloth_thickness_get_count(void);
size_t cloth_system_cloth_thickness_get_memory_usage(void);
void cloth_system_cloth_thickness_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SYSTEM_CLOTH_THICKNESS_H */
