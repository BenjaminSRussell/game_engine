/*
 * hair_forces.h
 * Hair force application
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef HAIR_SYSTEM_HAIR_FORCES_H
#define HAIR_SYSTEM_HAIR_FORCES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hair_system_hair_forces_handle {
    uint32_t id;
} hair_system_hair_forces_handle_t;

typedef struct hair_system_hair_forces_desc {
    uint32_t flags;
    void* user_data;
} hair_system_hair_forces_desc_t;

typedef struct hair_system_hair_forces_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} hair_system_hair_forces_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int hair_system_hair_forces_init(void);
void hair_system_hair_forces_shutdown(void);

/* Lifecycle */
int hair_system_hair_forces_create(hair_system_hair_forces_handle_t* out_handle, const hair_system_hair_forces_desc_t* desc);
void hair_system_hair_forces_destroy(hair_system_hair_forces_handle_t handle);

/* Operations */
int hair_system_hair_forces_update(hair_system_hair_forces_handle_t handle, const void* data, size_t size);
bool hair_system_hair_forces_is_valid(hair_system_hair_forces_handle_t handle);
int hair_system_hair_forces_get_info(hair_system_hair_forces_handle_t handle, hair_system_hair_forces_info_t* out_info);
void hair_system_hair_forces_mark_dirty(hair_system_hair_forces_handle_t handle);
int hair_system_hair_forces_process_pending(void);

/* Statistics */
uint32_t hair_system_hair_forces_get_count(void);
size_t hair_system_hair_forces_get_memory_usage(void);
void hair_system_hair_forces_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* HAIR_SYSTEM_HAIR_FORCES_H */
