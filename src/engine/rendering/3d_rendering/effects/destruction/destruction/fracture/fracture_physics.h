/*
 * fracture_physics.h
 * Fracture physics
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_FRACTURE_PHYSICS_H
#define DESTRUCTION_FRACTURE_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_fracture_physics_handle {
    uint32_t id;
} destruction_fracture_physics_handle_t;

typedef struct destruction_fracture_physics_desc {
    uint32_t flags;
    void* user_data;
} destruction_fracture_physics_desc_t;

typedef struct destruction_fracture_physics_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_fracture_physics_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_fracture_physics_init(void);
void destruction_fracture_physics_shutdown(void);

/* Lifecycle */
int destruction_fracture_physics_create(destruction_fracture_physics_handle_t* out_handle, const destruction_fracture_physics_desc_t* desc);
void destruction_fracture_physics_destroy(destruction_fracture_physics_handle_t handle);

/* Operations */
int destruction_fracture_physics_update(destruction_fracture_physics_handle_t handle, const void* data, size_t size);
bool destruction_fracture_physics_is_valid(destruction_fracture_physics_handle_t handle);
int destruction_fracture_physics_get_info(destruction_fracture_physics_handle_t handle, destruction_fracture_physics_info_t* out_info);
void destruction_fracture_physics_mark_dirty(destruction_fracture_physics_handle_t handle);
int destruction_fracture_physics_process_pending(void);

/* Statistics */
uint32_t destruction_fracture_physics_get_count(void);
size_t destruction_fracture_physics_get_memory_usage(void);
void destruction_fracture_physics_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_FRACTURE_PHYSICS_H */
