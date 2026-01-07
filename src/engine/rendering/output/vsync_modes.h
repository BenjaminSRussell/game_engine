/*
 * vsync_modes.h
 * VSync configuration
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_VSYNC_MODES_H
#define RENDERING_VSYNC_MODES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_vsync_modes_handle {
    uint32_t id;
} rendering_vsync_modes_handle_t;

typedef struct rendering_vsync_modes_desc {
    uint32_t flags;
    void* user_data;
} rendering_vsync_modes_desc_t;

typedef struct rendering_vsync_modes_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_vsync_modes_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_vsync_modes_init(void);
void rendering_vsync_modes_shutdown(void);

/* Lifecycle */
int rendering_vsync_modes_create(rendering_vsync_modes_handle_t* out_handle, const rendering_vsync_modes_desc_t* desc);
void rendering_vsync_modes_destroy(rendering_vsync_modes_handle_t handle);

/* Operations */
int rendering_vsync_modes_update(rendering_vsync_modes_handle_t handle, const void* data, size_t size);
bool rendering_vsync_modes_is_valid(rendering_vsync_modes_handle_t handle);
int rendering_vsync_modes_get_info(rendering_vsync_modes_handle_t handle, rendering_vsync_modes_info_t* out_info);
void rendering_vsync_modes_mark_dirty(rendering_vsync_modes_handle_t handle);
int rendering_vsync_modes_process_pending(void);

/* Statistics */
uint32_t rendering_vsync_modes_get_count(void);
size_t rendering_vsync_modes_get_memory_usage(void);
void rendering_vsync_modes_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_VSYNC_MODES_H */
