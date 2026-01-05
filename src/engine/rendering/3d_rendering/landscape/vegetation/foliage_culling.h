/*
 * foliage_culling.h
 * Foliage GPU culling
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_FOLIAGE_CULLING_H
#define LANDSCAPE_FOLIAGE_CULLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_culling_handle {
    uint32_t id;
} landscape_foliage_culling_handle_t;

typedef struct landscape_foliage_culling_desc {
    uint32_t flags;
    void* user_data;
} landscape_foliage_culling_desc_t;

typedef struct landscape_foliage_culling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_foliage_culling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_foliage_culling_init(void);
void landscape_foliage_culling_shutdown(void);

/* Lifecycle */
int landscape_foliage_culling_create(landscape_foliage_culling_handle_t* out_handle, const landscape_foliage_culling_desc_t* desc);
void landscape_foliage_culling_destroy(landscape_foliage_culling_handle_t handle);

/* Operations */
int landscape_foliage_culling_update(landscape_foliage_culling_handle_t handle, const void* data, size_t size);
bool landscape_foliage_culling_is_valid(landscape_foliage_culling_handle_t handle);
int landscape_foliage_culling_get_info(landscape_foliage_culling_handle_t handle, landscape_foliage_culling_info_t* out_info);
void landscape_foliage_culling_mark_dirty(landscape_foliage_culling_handle_t handle);
int landscape_foliage_culling_process_pending(void);

/* Statistics */
uint32_t landscape_foliage_culling_get_count(void);
size_t landscape_foliage_culling_get_memory_usage(void);
void landscape_foliage_culling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_CULLING_H */
