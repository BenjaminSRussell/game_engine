/*
 * cloud_lod.h
 * Cloud detail LOD
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_CLOUD_LOD_H
#define ATMOSPHERE_CLOUD_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_cloud_lod_handle {
    uint32_t id;
} atmosphere_cloud_lod_handle_t;

typedef struct atmosphere_cloud_lod_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_cloud_lod_desc_t;

typedef struct atmosphere_cloud_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_cloud_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_cloud_lod_init(void);
void atmosphere_cloud_lod_shutdown(void);

/* Lifecycle */
int atmosphere_cloud_lod_create(atmosphere_cloud_lod_handle_t* out_handle, const atmosphere_cloud_lod_desc_t* desc);
void atmosphere_cloud_lod_destroy(atmosphere_cloud_lod_handle_t handle);

/* Operations */
int atmosphere_cloud_lod_update(atmosphere_cloud_lod_handle_t handle, const void* data, size_t size);
bool atmosphere_cloud_lod_is_valid(atmosphere_cloud_lod_handle_t handle);
int atmosphere_cloud_lod_get_info(atmosphere_cloud_lod_handle_t handle, atmosphere_cloud_lod_info_t* out_info);
void atmosphere_cloud_lod_mark_dirty(atmosphere_cloud_lod_handle_t handle);
int atmosphere_cloud_lod_process_pending(void);

/* Statistics */
uint32_t atmosphere_cloud_lod_get_count(void);
size_t atmosphere_cloud_lod_get_memory_usage(void);
void atmosphere_cloud_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_CLOUD_LOD_H */
