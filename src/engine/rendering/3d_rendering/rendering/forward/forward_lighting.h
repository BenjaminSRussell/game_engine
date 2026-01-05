/*
 * forward_lighting.h
 * Forward lighting loop
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_LIGHTING_H
#define RENDERING_FORWARD_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_forward_lighting_handle {
    uint32_t id;
} rendering_forward_lighting_handle_t;

typedef struct rendering_forward_lighting_desc {
    uint32_t flags;
    void* user_data;
} rendering_forward_lighting_desc_t;

typedef struct rendering_forward_lighting_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_forward_lighting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_forward_lighting_init(void);
void rendering_forward_lighting_shutdown(void);

/* Lifecycle */
int rendering_forward_lighting_create(rendering_forward_lighting_handle_t* out_handle, const rendering_forward_lighting_desc_t* desc);
void rendering_forward_lighting_destroy(rendering_forward_lighting_handle_t handle);

/* Operations */
int rendering_forward_lighting_update(rendering_forward_lighting_handle_t handle, const void* data, size_t size);
bool rendering_forward_lighting_is_valid(rendering_forward_lighting_handle_t handle);
int rendering_forward_lighting_get_info(rendering_forward_lighting_handle_t handle, rendering_forward_lighting_info_t* out_info);
void rendering_forward_lighting_mark_dirty(rendering_forward_lighting_handle_t handle);
int rendering_forward_lighting_process_pending(void);

/* Statistics */
uint32_t rendering_forward_lighting_get_count(void);
size_t rendering_forward_lighting_get_memory_usage(void);
void rendering_forward_lighting_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_LIGHTING_H */
