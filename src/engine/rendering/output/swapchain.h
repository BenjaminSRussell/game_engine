/*
 * swapchain.h
 * Swapchain management
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_SWAPCHAIN_H
#define RENDERING_SWAPCHAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_swapchain_handle {
    uint32_t id;
} rendering_swapchain_handle_t;

typedef struct rendering_swapchain_desc {
    uint32_t flags;
    void* user_data;
} rendering_swapchain_desc_t;

typedef struct rendering_swapchain_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_swapchain_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_swapchain_init(void);
void rendering_swapchain_shutdown(void);

/* Lifecycle */
int rendering_swapchain_create(rendering_swapchain_handle_t* out_handle, const rendering_swapchain_desc_t* desc);
void rendering_swapchain_destroy(rendering_swapchain_handle_t handle);

/* Operations */
int rendering_swapchain_update(rendering_swapchain_handle_t handle, const void* data, size_t size);
bool rendering_swapchain_is_valid(rendering_swapchain_handle_t handle);
int rendering_swapchain_get_info(rendering_swapchain_handle_t handle, rendering_swapchain_info_t* out_info);
void rendering_swapchain_mark_dirty(rendering_swapchain_handle_t handle);
int rendering_swapchain_process_pending(void);

/* Statistics */
uint32_t rendering_swapchain_get_count(void);
size_t rendering_swapchain_get_memory_usage(void);
void rendering_swapchain_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_SWAPCHAIN_H */
