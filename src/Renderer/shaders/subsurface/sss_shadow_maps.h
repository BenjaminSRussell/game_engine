/*
 * sss_shadow_maps.h
 * SSS shadow maps
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_SSS_SHADOW_MAPS_H
#define SHADING_SSS_SHADOW_MAPS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_sss_shadow_maps_handle {
    uint32_t id;
} shading_sss_shadow_maps_handle_t;

typedef struct shading_sss_shadow_maps_desc {
    uint32_t flags;
    void* user_data;
} shading_sss_shadow_maps_desc_t;

typedef struct shading_sss_shadow_maps_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_sss_shadow_maps_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_sss_shadow_maps_init(void);
void shading_sss_shadow_maps_shutdown(void);

/* Lifecycle */
int shading_sss_shadow_maps_create(shading_sss_shadow_maps_handle_t* out_handle, const shading_sss_shadow_maps_desc_t* desc);
void shading_sss_shadow_maps_destroy(shading_sss_shadow_maps_handle_t handle);

/* Operations */
int shading_sss_shadow_maps_update(shading_sss_shadow_maps_handle_t handle, const void* data, size_t size);
bool shading_sss_shadow_maps_is_valid(shading_sss_shadow_maps_handle_t handle);
int shading_sss_shadow_maps_get_info(shading_sss_shadow_maps_handle_t handle, shading_sss_shadow_maps_info_t* out_info);
void shading_sss_shadow_maps_mark_dirty(shading_sss_shadow_maps_handle_t handle);
int shading_sss_shadow_maps_process_pending(void);

/* Statistics */
uint32_t shading_sss_shadow_maps_get_count(void);
size_t shading_sss_shadow_maps_get_memory_usage(void);
void shading_sss_shadow_maps_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SSS_SHADOW_MAPS_H */
