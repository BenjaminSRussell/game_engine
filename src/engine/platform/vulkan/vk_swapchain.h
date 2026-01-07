/*
 * vk_swapchain.h
 * Vulkan swapchain
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_VK_SWAPCHAIN_H
#define PLATFORM_VK_SWAPCHAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_vk_swapchain_handle {
    uint32_t id;
} platform_vk_swapchain_handle_t;

typedef struct platform_vk_swapchain_desc {
    uint32_t flags;
    void* user_data;
} platform_vk_swapchain_desc_t;

typedef struct platform_vk_swapchain_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} platform_vk_swapchain_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int platform_vk_swapchain_init(void);
void platform_vk_swapchain_shutdown(void);

/* Lifecycle */
int platform_vk_swapchain_create(platform_vk_swapchain_handle_t* out_handle, const platform_vk_swapchain_desc_t* desc);
void platform_vk_swapchain_destroy(platform_vk_swapchain_handle_t handle);

/* Operations */
int platform_vk_swapchain_update(platform_vk_swapchain_handle_t handle, const void* data, size_t size);
bool platform_vk_swapchain_is_valid(platform_vk_swapchain_handle_t handle);
int platform_vk_swapchain_get_info(platform_vk_swapchain_handle_t handle, platform_vk_swapchain_info_t* out_info);
void platform_vk_swapchain_mark_dirty(platform_vk_swapchain_handle_t handle);
int platform_vk_swapchain_process_pending(void);

/* Statistics */
uint32_t platform_vk_swapchain_get_count(void);
size_t platform_vk_swapchain_get_memory_usage(void);
void platform_vk_swapchain_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_VK_SWAPCHAIN_H */
