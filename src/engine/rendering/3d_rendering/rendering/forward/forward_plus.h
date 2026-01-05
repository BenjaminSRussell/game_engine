/*
 * forward_plus.h
 * Forward+ light culling
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_PLUS_H
#define RENDERING_FORWARD_PLUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_forward_plus_handle {
    uint32_t id;
} rendering_forward_plus_handle_t;

typedef struct rendering_forward_plus_desc {
    uint32_t flags;
    void* user_data;
} rendering_forward_plus_desc_t;

typedef struct rendering_forward_plus_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_forward_plus_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_forward_plus_init(void);
void rendering_forward_plus_shutdown(void);

/* Lifecycle */
int rendering_forward_plus_create(rendering_forward_plus_handle_t* out_handle, const rendering_forward_plus_desc_t* desc);
void rendering_forward_plus_destroy(rendering_forward_plus_handle_t handle);

/* Operations */
int rendering_forward_plus_update(rendering_forward_plus_handle_t handle, const void* data, size_t size);
bool rendering_forward_plus_is_valid(rendering_forward_plus_handle_t handle);
int rendering_forward_plus_get_info(rendering_forward_plus_handle_t handle, rendering_forward_plus_info_t* out_info);
void rendering_forward_plus_mark_dirty(rendering_forward_plus_handle_t handle);
int rendering_forward_plus_process_pending(void);

/* Statistics */
uint32_t rendering_forward_plus_get_count(void);
size_t rendering_forward_plus_get_memory_usage(void);
void rendering_forward_plus_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FORWARD_PLUS_H */
