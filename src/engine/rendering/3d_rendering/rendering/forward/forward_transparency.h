/*
 * forward_transparency.h
 * Forward transparent pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_TRANSPARENCY_H
#define RENDERING_FORWARD_TRANSPARENCY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_forward_transparency_handle {
    uint32_t id;
} rendering_forward_transparency_handle_t;

typedef struct rendering_forward_transparency_desc {
    uint32_t flags;
    void* user_data;
} rendering_forward_transparency_desc_t;

typedef struct rendering_forward_transparency_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_forward_transparency_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_forward_transparency_init(void);
void rendering_forward_transparency_shutdown(void);

/* Lifecycle */
int rendering_forward_transparency_create(rendering_forward_transparency_handle_t* out_handle, const rendering_forward_transparency_desc_t* desc);
void rendering_forward_transparency_destroy(rendering_forward_transparency_handle_t handle);

/* Operations */
int rendering_forward_transparency_update(rendering_forward_transparency_handle_t handle, const void* data, size_t size);
bool rendering_forward_transparency_is_valid(rendering_forward_transparency_handle_t handle);
int rendering_forward_transparency_get_info(rendering_forward_transparency_handle_t handle, rendering_forward_transparency_info_t* out_info);
void rendering_forward_transparency_mark_dirty(rendering_forward_transparency_handle_t handle);
int rendering_forward_transparency_process_pending(void);

/* Statistics */
uint32_t rendering_forward_transparency_get_count(void);
size_t rendering_forward_transparency_get_memory_usage(void);
void rendering_forward_transparency_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_TRANSPARENCY_H */
