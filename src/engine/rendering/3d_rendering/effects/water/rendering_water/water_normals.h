/*
 * water_normals.h
 * Water normal maps
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_WATER_NORMALS_H
#define WATER_WATER_NORMALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_water_normals_handle {
    uint32_t id;
} water_water_normals_handle_t;

typedef struct water_water_normals_desc {
    uint32_t flags;
    void* user_data;
} water_water_normals_desc_t;

typedef struct water_water_normals_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_water_normals_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_water_normals_init(void);
void water_water_normals_shutdown(void);

/* Lifecycle */
int water_water_normals_create(water_water_normals_handle_t* out_handle, const water_water_normals_desc_t* desc);
void water_water_normals_destroy(water_water_normals_handle_t handle);

/* Operations */
int water_water_normals_update(water_water_normals_handle_t handle, const void* data, size_t size);
bool water_water_normals_is_valid(water_water_normals_handle_t handle);
int water_water_normals_get_info(water_water_normals_handle_t handle, water_water_normals_info_t* out_info);
void water_water_normals_mark_dirty(water_water_normals_handle_t handle);
int water_water_normals_process_pending(void);

/* Statistics */
uint32_t water_water_normals_get_count(void);
size_t water_water_normals_get_memory_usage(void);
void water_water_normals_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WATER_NORMALS_H */
