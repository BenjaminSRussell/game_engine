/*
 * resident_set.h
 * Resident mesh management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_RESIDENT_SET_H
#define GEOMETRY_RESIDENT_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_resident_set_handle {
    uint32_t id;
} geometry_resident_set_handle_t;

typedef struct geometry_resident_set_desc {
    uint32_t flags;
    void* user_data;
} geometry_resident_set_desc_t;

typedef struct geometry_resident_set_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_resident_set_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_resident_set_init(void);
void geometry_resident_set_shutdown(void);

/* Lifecycle */
int geometry_resident_set_create(geometry_resident_set_handle_t* out_handle, const geometry_resident_set_desc_t* desc);
void geometry_resident_set_destroy(geometry_resident_set_handle_t handle);

/* Operations */
int geometry_resident_set_update(geometry_resident_set_handle_t handle, const void* data, size_t size);
bool geometry_resident_set_is_valid(geometry_resident_set_handle_t handle);
int geometry_resident_set_get_info(geometry_resident_set_handle_t handle, geometry_resident_set_info_t* out_info);
void geometry_resident_set_mark_dirty(geometry_resident_set_handle_t handle);
int geometry_resident_set_process_pending(void);

/* Statistics */
uint32_t geometry_resident_set_get_count(void);
size_t geometry_resident_set_get_memory_usage(void);
void geometry_resident_set_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_RESIDENT_SET_H */
