/*
 * destruction_lod.h
 * Destruction LOD
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_DESTRUCTION_LOD_H
#define DESTRUCTION_DESTRUCTION_LOD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_destruction_lod_handle {
    uint32_t id;
} destruction_destruction_lod_handle_t;

typedef struct destruction_destruction_lod_desc {
    uint32_t flags;
    void* user_data;
} destruction_destruction_lod_desc_t;

typedef struct destruction_destruction_lod_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_destruction_lod_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_destruction_lod_init(void);
void destruction_destruction_lod_shutdown(void);

/* Lifecycle */
int destruction_destruction_lod_create(destruction_destruction_lod_handle_t* out_handle, const destruction_destruction_lod_desc_t* desc);
void destruction_destruction_lod_destroy(destruction_destruction_lod_handle_t handle);

/* Operations */
int destruction_destruction_lod_update(destruction_destruction_lod_handle_t handle, const void* data, size_t size);
bool destruction_destruction_lod_is_valid(destruction_destruction_lod_handle_t handle);
int destruction_destruction_lod_get_info(destruction_destruction_lod_handle_t handle, destruction_destruction_lod_info_t* out_info);
void destruction_destruction_lod_mark_dirty(destruction_destruction_lod_handle_t handle);
int destruction_destruction_lod_process_pending(void);

/* Statistics */
uint32_t destruction_destruction_lod_get_count(void);
size_t destruction_destruction_lod_get_memory_usage(void);
void destruction_destruction_lod_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_DESTRUCTION_LOD_H */
