/*
 * resolution_scaling.h
 * Dynamic resolution
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_RESOLUTION_SCALING_H
#define RENDERING_RESOLUTION_SCALING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_resolution_scaling_handle {
    uint32_t id;
} rendering_resolution_scaling_handle_t;

typedef struct rendering_resolution_scaling_desc {
    uint32_t flags;
    void* user_data;
} rendering_resolution_scaling_desc_t;

typedef struct rendering_resolution_scaling_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_resolution_scaling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_resolution_scaling_init(void);
void rendering_resolution_scaling_shutdown(void);

/* Lifecycle */
int rendering_resolution_scaling_create(rendering_resolution_scaling_handle_t* out_handle, const rendering_resolution_scaling_desc_t* desc);
void rendering_resolution_scaling_destroy(rendering_resolution_scaling_handle_t handle);

/* Operations */
int rendering_resolution_scaling_update(rendering_resolution_scaling_handle_t handle, const void* data, size_t size);
bool rendering_resolution_scaling_is_valid(rendering_resolution_scaling_handle_t handle);
int rendering_resolution_scaling_get_info(rendering_resolution_scaling_handle_t handle, rendering_resolution_scaling_info_t* out_info);
void rendering_resolution_scaling_mark_dirty(rendering_resolution_scaling_handle_t handle);
int rendering_resolution_scaling_process_pending(void);

/* Statistics */
uint32_t rendering_resolution_scaling_get_count(void);
size_t rendering_resolution_scaling_get_memory_usage(void);
void rendering_resolution_scaling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_RESOLUTION_SCALING_H */
