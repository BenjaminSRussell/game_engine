/*
 * surface_lod.h
 * Surface cache LOD
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_SURFACE_LOD_H
#define LUMEN_SURFACE_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_surface_lod_handle {
    uint32_t id;
} lumen_surface_lod_handle_t;

typedef struct lumen_surface_lod_desc {
    uint32_t flags;
    void* user_data;
} lumen_surface_lod_desc_t;

typedef struct lumen_surface_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_surface_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_surface_lod_init(void);
void lumen_surface_lod_shutdown(void);

/* Lifecycle */
int lumen_surface_lod_create(lumen_surface_lod_handle_t* out_handle, const lumen_surface_lod_desc_t* desc);
void lumen_surface_lod_destroy(lumen_surface_lod_handle_t handle);

/* Operations */
int lumen_surface_lod_update(lumen_surface_lod_handle_t handle, const void* data, size_t size);
bool lumen_surface_lod_is_valid(lumen_surface_lod_handle_t handle);
int lumen_surface_lod_get_info(lumen_surface_lod_handle_t handle, lumen_surface_lod_info_t* out_info);
void lumen_surface_lod_mark_dirty(lumen_surface_lod_handle_t handle);
int lumen_surface_lod_process_pending(void);

/* Statistics */
uint32_t lumen_surface_lod_get_count(void);
size_t lumen_surface_lod_get_memory_usage(void);
void lumen_surface_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_SURFACE_LOD_H */
