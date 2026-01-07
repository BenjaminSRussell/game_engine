/*
 * triplanar_projection.h
 * Triplanar texture projection
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_TRIPLANAR_PROJECTION_H
#define MATERIALS_TRIPLANAR_PROJECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_triplanar_projection_handle {
    uint32_t id;
} materials_triplanar_projection_handle_t;

typedef struct materials_triplanar_projection_desc {
    uint32_t flags;
    void* user_data;
} materials_triplanar_projection_desc_t;

typedef struct materials_triplanar_projection_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_triplanar_projection_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_triplanar_projection_init(void);
void materials_triplanar_projection_shutdown(void);

/* Lifecycle */
int materials_triplanar_projection_create(materials_triplanar_projection_handle_t* out_handle, const materials_triplanar_projection_desc_t* desc);
void materials_triplanar_projection_destroy(materials_triplanar_projection_handle_t handle);

/* Operations */
int materials_triplanar_projection_update(materials_triplanar_projection_handle_t handle, const void* data, size_t size);
bool materials_triplanar_projection_is_valid(materials_triplanar_projection_handle_t handle);
int materials_triplanar_projection_get_info(materials_triplanar_projection_handle_t handle, materials_triplanar_projection_info_t* out_info);
void materials_triplanar_projection_mark_dirty(materials_triplanar_projection_handle_t handle);
int materials_triplanar_projection_process_pending(void);

/* Statistics */
uint32_t materials_triplanar_projection_get_count(void);
size_t materials_triplanar_projection_get_memory_usage(void);
void materials_triplanar_projection_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_TRIPLANAR_PROJECTION_H */
