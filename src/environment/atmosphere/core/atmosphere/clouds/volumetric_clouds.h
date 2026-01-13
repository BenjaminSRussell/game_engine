/*
 * volumetric_clouds.h
 * Volumetric cloud rendering
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_VOLUMETRIC_CLOUDS_H
#define ATMOSPHERE_VOLUMETRIC_CLOUDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_volumetric_clouds_handle {
    uint32_t id;
} atmosphere_volumetric_clouds_handle_t;

typedef struct atmosphere_volumetric_clouds_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_volumetric_clouds_desc_t;

typedef struct atmosphere_volumetric_clouds_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_volumetric_clouds_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_volumetric_clouds_init(void);
void atmosphere_volumetric_clouds_shutdown(void);

/* Lifecycle */
int atmosphere_volumetric_clouds_create(atmosphere_volumetric_clouds_handle_t* out_handle, const atmosphere_volumetric_clouds_desc_t* desc);
void atmosphere_volumetric_clouds_destroy(atmosphere_volumetric_clouds_handle_t handle);

/* Operations */
int atmosphere_volumetric_clouds_update(atmosphere_volumetric_clouds_handle_t handle, const void* data, size_t size);
bool atmosphere_volumetric_clouds_is_valid(atmosphere_volumetric_clouds_handle_t handle);
int atmosphere_volumetric_clouds_get_info(atmosphere_volumetric_clouds_handle_t handle, atmosphere_volumetric_clouds_info_t* out_info);
void atmosphere_volumetric_clouds_mark_dirty(atmosphere_volumetric_clouds_handle_t handle);
int atmosphere_volumetric_clouds_process_pending(void);

/* Statistics */
uint32_t atmosphere_volumetric_clouds_get_count(void);
size_t atmosphere_volumetric_clouds_get_memory_usage(void);
void atmosphere_volumetric_clouds_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_VOLUMETRIC_CLOUDS_H */
