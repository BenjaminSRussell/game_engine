/*
 * vsm_invalidation.h
 * Page invalidation
 *
 * Part of the Virtual Shadow Maps subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VIRTUAL_SHADOW_MAPS_VSM_INVALIDATION_H
#define VIRTUAL_SHADOW_MAPS_VSM_INVALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_page_table.h"
#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_page_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct virtual_shadow_maps_vsm_invalidation_handle {
    uint32_t id;
} virtual_shadow_maps_vsm_invalidation_handle_t;

typedef struct virtual_shadow_maps_vsm_invalidation_desc {
    uint32_t flags;
    void* user_data;
} virtual_shadow_maps_vsm_invalidation_desc_t;

typedef struct virtual_shadow_maps_vsm_invalidation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} virtual_shadow_maps_vsm_invalidation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int virtual_shadow_maps_vsm_invalidation_init(void);
void virtual_shadow_maps_vsm_invalidation_shutdown(void);

/* Lifecycle */
int virtual_shadow_maps_vsm_invalidation_create(virtual_shadow_maps_vsm_invalidation_handle_t* out_handle, const virtual_shadow_maps_vsm_invalidation_desc_t* desc);
void virtual_shadow_maps_vsm_invalidation_destroy(virtual_shadow_maps_vsm_invalidation_handle_t handle);

/* Operations */
int virtual_shadow_maps_vsm_invalidation_update(virtual_shadow_maps_vsm_invalidation_handle_t handle, const void* data, size_t size);
bool virtual_shadow_maps_vsm_invalidation_is_valid(virtual_shadow_maps_vsm_invalidation_handle_t handle);
int virtual_shadow_maps_vsm_invalidation_get_info(virtual_shadow_maps_vsm_invalidation_handle_t handle, virtual_shadow_maps_vsm_invalidation_info_t* out_info);
void virtual_shadow_maps_vsm_invalidation_mark_dirty(virtual_shadow_maps_vsm_invalidation_handle_t handle);
int virtual_shadow_maps_vsm_invalidation_process_pending(void);

void vsm_invalidate_page(virtual_shadow_maps_vsm_page_table_handle_t table, virtual_shadow_maps_vsm_page_pool_handle_t pool, uint32_t v_x, uint32_t v_y);

/* Statistics */
uint32_t virtual_shadow_maps_vsm_invalidation_get_count(void);
size_t virtual_shadow_maps_vsm_invalidation_get_memory_usage(void);
void virtual_shadow_maps_vsm_invalidation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VIRTUAL_SHADOW_MAPS_VSM_INVALIDATION_H */
