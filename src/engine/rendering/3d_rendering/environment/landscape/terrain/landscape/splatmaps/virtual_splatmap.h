/*
 * virtual_splatmap.h
 * Virtual splat maps
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_VIRTUAL_SPLATMAP_H
#define LANDSCAPE_VIRTUAL_SPLATMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_virtual_splatmap_handle {
    uint32_t id;
} landscape_virtual_splatmap_handle_t;

typedef struct landscape_virtual_splatmap_desc {
    uint32_t flags;
    void* user_data;
} landscape_virtual_splatmap_desc_t;

typedef struct landscape_virtual_splatmap_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_virtual_splatmap_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_virtual_splatmap_init(void);
void landscape_virtual_splatmap_shutdown(void);

/* Lifecycle */
int landscape_virtual_splatmap_create(landscape_virtual_splatmap_handle_t* out_handle, const landscape_virtual_splatmap_desc_t* desc);
void landscape_virtual_splatmap_destroy(landscape_virtual_splatmap_handle_t handle);

/* Operations */
int landscape_virtual_splatmap_update(landscape_virtual_splatmap_handle_t handle, const void* data, size_t size);
bool landscape_virtual_splatmap_is_valid(landscape_virtual_splatmap_handle_t handle);
int landscape_virtual_splatmap_get_info(landscape_virtual_splatmap_handle_t handle, landscape_virtual_splatmap_info_t* out_info);
void landscape_virtual_splatmap_mark_dirty(landscape_virtual_splatmap_handle_t handle);
int landscape_virtual_splatmap_process_pending(void);

/* Statistics */
uint32_t landscape_virtual_splatmap_get_count(void);
size_t landscape_virtual_splatmap_get_memory_usage(void);
void landscape_virtual_splatmap_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_VIRTUAL_SPLATMAP_H */
