/*
 * vsm_page_pool.h
 * VSM page pool
 *
 * Part of the Virtual Shadow Maps subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VIRTUAL_SHADOW_MAPS_VSM_PAGE_POOL_H
#define VIRTUAL_SHADOW_MAPS_VSM_PAGE_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct virtual_shadow_maps_vsm_page_pool_handle {
    uint32_t id;
} virtual_shadow_maps_vsm_page_pool_handle_t;

typedef struct virtual_shadow_maps_vsm_page_pool_desc {
    uint32_t flags;
    void* user_data;
} virtual_shadow_maps_vsm_page_pool_desc_t;

typedef struct virtual_shadow_maps_vsm_page_pool_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} virtual_shadow_maps_vsm_page_pool_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int virtual_shadow_maps_vsm_page_pool_init(void);
void virtual_shadow_maps_vsm_page_pool_shutdown(void);

/* Lifecycle */
int virtual_shadow_maps_vsm_page_pool_create(virtual_shadow_maps_vsm_page_pool_handle_t* out_handle, const virtual_shadow_maps_vsm_page_pool_desc_t* desc);
void virtual_shadow_maps_vsm_page_pool_destroy(virtual_shadow_maps_vsm_page_pool_handle_t handle);

/* Operations */
int virtual_shadow_maps_vsm_page_pool_update(virtual_shadow_maps_vsm_page_pool_handle_t handle, const void* data, size_t size);
bool virtual_shadow_maps_vsm_page_pool_is_valid(virtual_shadow_maps_vsm_page_pool_handle_t handle);
int virtual_shadow_maps_vsm_page_pool_get_info(virtual_shadow_maps_vsm_page_pool_handle_t handle, virtual_shadow_maps_vsm_page_pool_info_t* out_info);
void virtual_shadow_maps_vsm_page_pool_mark_dirty(virtual_shadow_maps_vsm_page_pool_handle_t handle);
int virtual_shadow_maps_vsm_page_pool_process_pending(void);

/* Statistics */
uint32_t virtual_shadow_maps_vsm_page_pool_get_count(void);
size_t virtual_shadow_maps_vsm_page_pool_get_memory_usage(void);
void virtual_shadow_maps_vsm_page_pool_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VIRTUAL_SHADOW_MAPS_VSM_PAGE_POOL_H */
