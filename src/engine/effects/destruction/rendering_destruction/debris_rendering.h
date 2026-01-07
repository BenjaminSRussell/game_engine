/*
 * debris_rendering.h
 * Debris rendering
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_DEBRIS_RENDERING_H
#define DESTRUCTION_DEBRIS_RENDERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_debris_rendering_handle {
    uint32_t id;
} destruction_debris_rendering_handle_t;

typedef struct destruction_debris_rendering_desc {
    uint32_t flags;
    void* user_data;
} destruction_debris_rendering_desc_t;

typedef struct destruction_debris_rendering_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_debris_rendering_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_debris_rendering_init(void);
void destruction_debris_rendering_shutdown(void);

/* Lifecycle */
int destruction_debris_rendering_create(destruction_debris_rendering_handle_t* out_handle, const destruction_debris_rendering_desc_t* desc);
void destruction_debris_rendering_destroy(destruction_debris_rendering_handle_t handle);

/* Operations */
int destruction_debris_rendering_update(destruction_debris_rendering_handle_t handle, const void* data, size_t size);
bool destruction_debris_rendering_is_valid(destruction_debris_rendering_handle_t handle);
int destruction_debris_rendering_get_info(destruction_debris_rendering_handle_t handle, destruction_debris_rendering_info_t* out_info);
void destruction_debris_rendering_mark_dirty(destruction_debris_rendering_handle_t handle);
int destruction_debris_rendering_process_pending(void);

/* Statistics */
uint32_t destruction_debris_rendering_get_count(void);
size_t destruction_debris_rendering_get_memory_usage(void);
void destruction_debris_rendering_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_DEBRIS_RENDERING_H */
