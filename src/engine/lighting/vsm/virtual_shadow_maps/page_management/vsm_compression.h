/*
 * vsm_compression.h
 * Page compression
 *
 * Part of the Virtual Shadow Maps subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VIRTUAL_SHADOW_MAPS_VSM_COMPRESSION_H
#define VIRTUAL_SHADOW_MAPS_VSM_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct virtual_shadow_maps_vsm_compression_handle {
    uint32_t id;
} virtual_shadow_maps_vsm_compression_handle_t;

typedef struct virtual_shadow_maps_vsm_compression_desc {
    uint32_t flags;
    void* user_data;
} virtual_shadow_maps_vsm_compression_desc_t;

typedef struct virtual_shadow_maps_vsm_compression_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} virtual_shadow_maps_vsm_compression_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int virtual_shadow_maps_vsm_compression_init(void);
void virtual_shadow_maps_vsm_compression_shutdown(void);

/* Lifecycle */
int virtual_shadow_maps_vsm_compression_create(virtual_shadow_maps_vsm_compression_handle_t* out_handle, const virtual_shadow_maps_vsm_compression_desc_t* desc);
void virtual_shadow_maps_vsm_compression_destroy(virtual_shadow_maps_vsm_compression_handle_t handle);

/* Operations */
int virtual_shadow_maps_vsm_compression_update(virtual_shadow_maps_vsm_compression_handle_t handle, const void* data, size_t size);
bool virtual_shadow_maps_vsm_compression_is_valid(virtual_shadow_maps_vsm_compression_handle_t handle);
int virtual_shadow_maps_vsm_compression_get_info(virtual_shadow_maps_vsm_compression_handle_t handle, virtual_shadow_maps_vsm_compression_info_t* out_info);
void virtual_shadow_maps_vsm_compression_mark_dirty(virtual_shadow_maps_vsm_compression_handle_t handle);
int virtual_shadow_maps_vsm_compression_process_pending(void);

/* Statistics */
uint32_t virtual_shadow_maps_vsm_compression_get_count(void);
size_t virtual_shadow_maps_vsm_compression_get_memory_usage(void);
void virtual_shadow_maps_vsm_compression_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VIRTUAL_SHADOW_MAPS_VSM_COMPRESSION_H */
