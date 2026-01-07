/*
 * deferred_decals.h
 * Deferred decal pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_DEFERRED_DECALS_H
#define RENDERING_DEFERRED_DECALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_deferred_decals_handle {
    uint32_t id;
} rendering_deferred_decals_handle_t;

typedef struct rendering_deferred_decals_desc {
    uint32_t flags;
    void* user_data;
} rendering_deferred_decals_desc_t;

typedef struct rendering_deferred_decals_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_deferred_decals_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_deferred_decals_init(void);
void rendering_deferred_decals_shutdown(void);

/* Lifecycle */
int rendering_deferred_decals_create(rendering_deferred_decals_handle_t* out_handle, const rendering_deferred_decals_desc_t* desc);
void rendering_deferred_decals_destroy(rendering_deferred_decals_handle_t handle);

/* Operations */
int rendering_deferred_decals_update(rendering_deferred_decals_handle_t handle, const void* data, size_t size);
bool rendering_deferred_decals_is_valid(rendering_deferred_decals_handle_t handle);
int rendering_deferred_decals_get_info(rendering_deferred_decals_handle_t handle, rendering_deferred_decals_info_t* out_info);
void rendering_deferred_decals_mark_dirty(rendering_deferred_decals_handle_t handle);
int rendering_deferred_decals_process_pending(void);

/* Statistics */
uint32_t rendering_deferred_decals_get_count(void);
size_t rendering_deferred_decals_get_memory_usage(void);
void rendering_deferred_decals_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_DEFERRED_DECALS_H */
