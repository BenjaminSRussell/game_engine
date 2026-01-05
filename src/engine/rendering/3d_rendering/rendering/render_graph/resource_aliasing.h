/*
 * resource_aliasing.h
 * Transient resource aliasing
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_RESOURCE_ALIASING_H
#define RENDERING_RESOURCE_ALIASING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_resource_aliasing_handle {
    uint32_t id;
} rendering_resource_aliasing_handle_t;

typedef struct rendering_resource_aliasing_desc {
    uint32_t flags;
    void* user_data;
} rendering_resource_aliasing_desc_t;

typedef struct rendering_resource_aliasing_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_resource_aliasing_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_resource_aliasing_init(void);
void rendering_resource_aliasing_shutdown(void);

/* Lifecycle */
int rendering_resource_aliasing_create(rendering_resource_aliasing_handle_t* out_handle, const rendering_resource_aliasing_desc_t* desc);
void rendering_resource_aliasing_destroy(rendering_resource_aliasing_handle_t handle);

/* Operations */
int rendering_resource_aliasing_update(rendering_resource_aliasing_handle_t handle, const void* data, size_t size);
bool rendering_resource_aliasing_is_valid(rendering_resource_aliasing_handle_t handle);
int rendering_resource_aliasing_get_info(rendering_resource_aliasing_handle_t handle, rendering_resource_aliasing_info_t* out_info);
void rendering_resource_aliasing_mark_dirty(rendering_resource_aliasing_handle_t handle);
int rendering_resource_aliasing_process_pending(void);

/* Statistics */
uint32_t rendering_resource_aliasing_get_count(void);
size_t rendering_resource_aliasing_get_memory_usage(void);
void rendering_resource_aliasing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_RESOURCE_ALIASING_H */
